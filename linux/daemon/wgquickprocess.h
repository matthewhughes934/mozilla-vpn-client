/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WGQUICKPROCESS_H
#define WGQUICKPROCESS_H

#include <QObject>
#include <QTemporaryDir>

constexpr const char *WG_INTERFACE = "moz0";

class WgQuickProcess : public QObject
{
    Q_OBJECT
public:
    enum Op {
        Up,
        Down,
    };

    explicit WgQuickProcess(Op op);

    void run(const QString &privateKey,
             const QString &deviceIpv4Address,
             const QString &deviceIpv6Address,
             const QString &serverIpv4Gateway,
             const QString &serverPublicKey,
             const QString &serverIpv4AddrIn,
             const QString &serverIpv6AddrIn,
             int serverPort,
             bool ipv6Enabled,
             bool localNetworkAccess);

signals:
    void failed();
    void succeeded();

private:
    Op m_op;

    QTemporaryDir tmpDir;
};

#endif // WGQUICKPROCESS_H
