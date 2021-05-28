/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DAEMON_H
#define DAEMON_H

#include "interfaceconfig.h"
#include "iputils.h"
#include "wireguardutils.h"

#include <QDateTime>

class Daemon : public QObject {
  Q_OBJECT

 public:
  enum Op {
    Up,
    Down,
  };

  struct Config {
    QString m_privateKey;
    QString m_deviceIpv4Address;
    QString m_deviceIpv6Address;
    QString m_serverIpv4Gateway;
    QString m_serverIpv6Gateway;
    QString m_serverPublicKey;
    QString m_serverIpv4AddrIn;
    QString m_serverIpv6AddrIn;
    int m_serverPort = 0;
    bool m_ipv6Enabled = false;
    QList<IPAddressRange> m_allowedIPAddressRanges;
    QList<QString> m_vpnDisabledApps;
  };

  explicit Daemon(QObject* parent);
  ~Daemon();

  static Daemon* instance();

  static bool parseConfig(const QJsonObject& obj, InterfaceConfig& config);

  virtual bool activate(const InterfaceConfig& config);
  virtual bool deactivate(bool emitSignals = true);

  // Explose a JSON object with the daemon status.
  virtual QByteArray getStatus() = 0;

  QString logs();
  void cleanLogs();

 signals:
  void connected();
  void disconnected();
  void backendFailure();

 protected:
  virtual bool run(Op op, const InterfaceConfig& config) = 0;
  virtual bool supportServerSwitching(const InterfaceConfig& config) const {
    Q_UNUSED(config);
    return false;
  }
  virtual bool switchServer(const InterfaceConfig& config);
  virtual bool supportWGUtils() const { return false; }
  virtual WireguardUtils* wgutils() { return nullptr; }
  virtual bool supportIPUtils() const { return false; }
  virtual IPUtils* iputils() { return nullptr; }

  bool m_connected = false;
  QDateTime m_connectionDate;
  InterfaceConfig m_lastConfig;
};

#endif  // DAEMON_H
