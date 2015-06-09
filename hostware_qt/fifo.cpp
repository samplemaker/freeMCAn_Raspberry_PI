/** \file fifo.cpp
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


#include <QtCore/QDebug>
#include "fifo.h"


Fifo::Fifo(int size) {
    fb = new FifoBuffer();
    fb->size  = size;
    fb->start = 0;
    fb->count = 0;
    fb->payLoad = new payloadData [fb->size + 1]();
}


Fifo::~Fifo()
{
   delete[] fb;
}


/* set print elements 10
 * graph display fb->payLoad[0] @ 7 */

/** write at head */
void Fifo::writeHead(const payloadData *elem) {
    int end = (fb->start + fb->count) % fb->size;
    fb->payLoad[end] = *elem;
    if (fb->count == fb->size)
        fb->start = (fb->start + 1) % fb->size;
    else
        ++ fb->count;
}


/** copy the newest N (or max available) elements without removing from the FIFO */
int Fifo::copyLastN(int N, payloadData *elem) {
    int toCopy = (fb->count > N) ? N : fb->count;
    int strtPos = (fb->start - toCopy + fb->count ) % fb->size;
    for (int i = 0; i < toCopy; i++){
        elem[i] = fb->payLoad[strtPos];
        strtPos = (strtPos + 1) % fb->size;
    }
    return toCopy;
}


/** read at tail and remove */
void Fifo::readTail(payloadData *elem) {
    *elem = fb->payLoad[fb->start];
    fb->start = (fb->start + 1) % fb->size;
    -- fb->count;
}


int Fifo::isFull(void) {
    return fb->count == fb->size;
}


int Fifo::isEmpty(void) {
    return fb->count == 0;
}

void Fifo::reset(void) {
    fb->start = 0;
    fb->count = 0;
}
