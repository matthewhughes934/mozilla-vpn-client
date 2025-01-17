/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macospingsender.h"
#include "leakdetector.h"
#include "logger.h"

#include <QSocketNotifier>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>

namespace {

Logger logger({LOG_MACOS, LOG_NETWORKING}, "MacOSPingSender");

int identifier() { return (getpid() & 0xFFFF); }

// From ping.c implementation:
u_short in_cksum(u_short* addr, int len) {
  int nleft, sum;
  u_short* w;
  union {
    u_short us;
    u_char uc[2];
  } last;
  u_short answer;

  nleft = len;
  sum = 0;
  w = addr;

  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    last.uc[0] = *(u_char*)w;
    last.uc[1] = 0;
    sum += last.us;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
  sum += (sum >> 16);                 /* add carry */
  answer = ~sum;                      /* truncate to 16 bits */
  return (answer);
}

};  // namespace

MacOSPingSender::MacOSPingSender(const QString& source, QObject* parent)
    : PingSender(parent) {
  MVPN_COUNT_CTOR(MacOSPingSender);

  if (getuid()) {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
  } else {
    m_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  }
  if (m_socket < 0) {
    logger.log() << "Socket creation failed";
    return;
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_len = sizeof(addr);

  if (inet_aton(source.toLocal8Bit().constData(), &addr.sin_addr) == 0) {
    logger.log() << "source address error";
    return;
  }

  if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
    logger.log() << "bind error:" << strerror(errno);
    return;
  }

  m_notifier = new QSocketNotifier(m_socket, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &MacOSPingSender::socketReady);
}

MacOSPingSender::~MacOSPingSender() {
  MVPN_COUNT_DTOR(MacOSPingSender);
  if (m_socket >= 0) {
    close(m_socket);
  }
}

void MacOSPingSender::sendPing(const QString& dest, quint16 sequence) {
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_len = sizeof(addr);

  if (inet_aton(dest.toLocal8Bit().constData(), &addr.sin_addr) == 0) {
    logger.log() << "DNS lookup failed";
    return;
  }

  struct icmp packet;
  bzero(&packet, sizeof packet);
  packet.icmp_type = ICMP_ECHO;
  packet.icmp_id = identifier();
  packet.icmp_seq = htons(sequence);
  packet.icmp_cksum = in_cksum((u_short*)&packet, sizeof(packet));

  if (sendto(m_socket, (char*)&packet, sizeof(packet), 0,
             (struct sockaddr*)&addr, sizeof(addr)) != sizeof(packet)) {
    logger.log() << "ping sending failed:" << strerror(errno);
    return;
  }
}

void MacOSPingSender::socketReady() {
  struct msghdr msg;
  bzero(&msg, sizeof(msg));

  struct sockaddr_in addr;
  msg.msg_name = (caddr_t)&addr;
  msg.msg_namelen = sizeof(addr);

  struct iovec iov;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  u_char packet[IP_MAXPACKET];
  iov.iov_base = packet;
  iov.iov_len = IP_MAXPACKET;

  ssize_t rc = recvmsg(m_socket, &msg, MSG_DONTWAIT);
  if (rc <= 0) {
    logger.log() << "Recvmsg failed";
    return;
  }

  struct ip* ip = (struct ip*)packet;
  int hlen = ip->ip_hl << 2;
  struct icmp* icmp = (struct icmp*)(((char*)packet) + hlen);

  if (icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == identifier()) {
    emit recvPing(htons(icmp->icmp_seq));
  }
}
