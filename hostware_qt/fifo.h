/** \file fifo.h
* \brief
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


#ifndef FIFO_H_
#define FIFO_H_

#include <QObject>
#include "parser.h"


/** payload data which is sent over the character device */
class FifoBuffer
{
  public:
    int size;
    int start;
    int count;
    payloadData *payLoad;
};


/* we need the QObject to implement signals and slots */
class Fifo : public QObject
{
    Q_OBJECT

public:
    explicit Fifo (int size);
    ~Fifo ();
    void writeHead(const payloadData *elem);
    void readTail(payloadData *elem);
    int copyLastN(int N, payloadData *elem);
    int isFull(void);
    int isEmpty(void);
    void reset(void);


signals:


public slots:


private:
    FifoBuffer *fb;

};

#endif
