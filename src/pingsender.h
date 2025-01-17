/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef PINGSENDER_H
#define PINGSENDER_H

#include <QElapsedTimer>
#include <QObject>

class PingSender : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(PingSender)

 public:
  PingSender(QObject* parent = nullptr) : QObject(parent) {}

  virtual void sendPing(const QString& destination, quint16 sequence) = 0;

 signals:
  void recvPing(quint16 sequence);
};

#endif  // PINGSENDER_H
