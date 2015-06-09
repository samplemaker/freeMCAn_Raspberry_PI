/** \file qchardev.cpp
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

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QtCore/QDebug>
#include <QtCore/QReadLocker>
#include <QtCore/QWriteLocker>
#include <QSocketNotifier>
#include "qchardev.h"
#include "common_defs.h"


QcharDev::QcharDev(QObject *parent)
: QIODevice(parent)
{
    fd = 0;
    readNotifier = 0;
}


QcharDev::~QcharDev()
{
    if (isOpen())
        close();
}


qint64 QcharDev::startMsrmnt(void)
{
    int retVal = -1;

    if (isOpen()) {
        retVal = ::ioctl(fd, IOCTL_START_MEASUREMENT, NULL);
    }

    return retVal;
}


qint64 QcharDev::stopMsrmnt(void)
{
    int retVal = -1;

    if (isOpen()) {
        retVal = ::ioctl(fd, IOCTL_STOP_MEASUREMENT, NULL);
    }

    return retVal;
}


qint64 QcharDev::setTimerCountsPerSample(unsigned int *cps)
{
    int retVal = ::ioctl(fd, IOCTL_SET_TCNTSPERSAMPLE, cps);

    return retVal;
}


void QcharDev::_q_canRead()
{
    //qWarning() << "emit readyread() ";
    Q_EMIT this->readyRead();
}


bool QcharDev::open(OpenMode mode)
{
    if ((mode & QIODevice::ReadOnly) && !isOpen()) {
        if ((fd = ::open(QString("/dev/" DEVICE_NAME).toLatin1() ,O_RDONLY)) != -1) {
            setOpenMode(mode);

            readNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
            connect(readNotifier, SIGNAL(activated(int)), this, SLOT(_q_canRead()));
            return true;
        }
    }
    return false;
}


void QcharDev::close()
{
    if (isOpen()) {
        QIODevice::close(); // mark ourselves as closed
        ::close(fd);
        if (readNotifier) {
            delete readNotifier;
            readNotifier = 0;
        }
    }
}


qint64 QcharDev::readData(char *data, qint64 maxSize)
{    
    int retVal = -1;

    //qWarning() << "read() with maxSize" << maxSize;
    if (maxSize)
        retVal = ::read(fd, data, maxSize);

    return retVal;
}


qint64 QcharDev::writeData(const char *data, qint64 maxSize)
{
    Q_UNUSED(data)
    Q_UNUSED(maxSize)
    return 0;
}


qint64 QcharDev::bytesAvailable() const
{
    int retVal = -1;
    qint64 len = 0;

    if (isOpen()) {
        retVal = ::ioctl(fd, IOCTL_GET_FIFO_LEN, len);
    }

    return (retVal > 0) ? retVal : len;
}


QByteArray QcharDev::readAll()
{
    int avail = this->bytesAvailable();
    return (avail > 0) ? this->read(avail) : QByteArray();
}
