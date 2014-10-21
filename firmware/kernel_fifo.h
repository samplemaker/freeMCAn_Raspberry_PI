/** \file firmware/kernel_fifo.h
 * \brief Implements a customized ring buffer
 *
 * \author Copyright (C) 2014 samplemaker
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 *
 */

#ifndef KERNEL_FIFO_H
#define KERNEL_FIFO_H

/** payload data which is sent over the character device */
typedef struct {
  int timer_counts;
  int kernel_time;
  int accu_counts;
} fifo_data_t;

ssize_t fifo_request_for_read(void);
void fifo_data_remove(void);
int fifo_put(fifo_data_t data);
int fifo_get(fifo_data_t *data);
void fifo_init(void);

#endif
