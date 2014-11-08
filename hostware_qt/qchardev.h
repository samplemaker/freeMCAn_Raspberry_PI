/** \file qchardev.h
* \brief QIODevice to access and control the kernel firmware
*
* \author Copyright (C) 2014 samplemaker
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1
* of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA
*
* @{
*/


#ifndef _QCHARDEV_H_
#define _QCHARDEV_H_

#include <QtCore/QtGlobal>
#include <QtCore/QIODevice>

class QSocketNotifier;

class QcharDev: public QIODevice
{
    Q_OBJECT

public:
    QcharDev(QObject *parent = 0);
    ~QcharDev();

    qint64 stopMsrmnt(void);
    qint64 startMsrmnt(void);
    qint64 bytesAvailable() const;
    QByteArray readAll();
    bool open(OpenMode mode);
    void close();

private:
    int fd;
    QSocketNotifier *readNotifier;

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private slots:
    void _q_canRead(void);

};

#endif
