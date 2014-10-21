/** \file firmware/kernel_fifo.c
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
 *
 *  Customized 2^n ringbuffer which holds customized typedefs
 *  (fifo_buffer_t).
 *
 *  NOTE: The code handles only one ringbuffer instance (use a static
 *        define)!
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include "kernel_fifo.h"

/* can write maximum 2^n - 1 elements */
#define FIFO_SIZE (1 << 8)
#define FIFO_MASK (FIFO_SIZE - 1)


/** fifo rw-pointers + copy + payload data */
typedef struct {
  fifo_data_t data[FIFO_SIZE];
  unsigned int read;
  unsigned int write;
  unsigned int read_cpy;
  unsigned int write_cpy;
  spinlock_t my_lock;
} fifo_buffer_t;


/* WARNING! can only handly one instance! */
static fifo_buffer_t fifo_buffer = {{}, 0, 0, 0, 0, {}};


void
fifo_init(void)
{
  spin_lock_init(&fifo_buffer.my_lock);
}


/** Register at ring buffer for read event
 *
 * Fetch a snapshot of the ring buffer rw-pointers to read from
 * the tail while the IRQ handler continues to write at the
 * ringbuffer head. Returns the number of elements allowed to be
 * read.
 */
ssize_t
fifo_request_for_read(void)
{
  unsigned long irq_flags;
  /* shut down the interrupts to protect the critical code section
     and restore the IRQ flags after */
  spin_lock_irqsave(&fifo_buffer.my_lock, irq_flags);
  fifo_buffer.read_cpy = fifo_buffer.read;
  fifo_buffer.write_cpy = fifo_buffer.write;
  spin_unlock_irqrestore(&fifo_buffer.my_lock, irq_flags);
  const ssize_t empty = ((fifo_buffer.read_cpy -
                          fifo_buffer.write_cpy - 1) & FIFO_MASK) + 1;
  const ssize_t to_read = FIFO_SIZE - empty;
  return to_read;
}


/** Remove data from the ringbuffer
 *
 * Remove up to the element when fifo_request_for_read() was called
 */
void
fifo_data_remove(void)
{
  unsigned long irq_flags;
  spin_lock_irqsave(&fifo_buffer.my_lock, irq_flags);
  fifo_buffer.read = fifo_buffer.read_cpy;
  spin_unlock_irqrestore(&fifo_buffer.my_lock, irq_flags);
}


/** Write into the ring buffer (IRQ handler only)
 *
 * The character device may read from the tail
 */
int
fifo_put(fifo_data_t data)
{
  /* the function is indented to be called from within the
     timer ISR. hence we don't expect to have two running ISRs
     on the data at the same time we dont shut off interrupts */
  spin_lock(&fifo_buffer.my_lock);
  unsigned int next = ((fifo_buffer.write + 1) & FIFO_MASK);
  if (fifo_buffer.read == next){
    spin_unlock(&fifo_buffer.my_lock);
    return -1;
  }
  fifo_buffer.data[fifo_buffer.write & FIFO_MASK] = data;
  fifo_buffer.write = next;
  spin_unlock(&fifo_buffer.my_lock);
  return 0;
}


/** Read from the buffer (character device only)
 *
 *  whereas the IRQ may continue to write at the same time at
 *  the head
 */
int
fifo_get(fifo_data_t *data)
{
  if (fifo_buffer.read_cpy == fifo_buffer.write_cpy)
    return -1;
  *data = fifo_buffer.data[fifo_buffer.read_cpy];
  fifo_buffer.read_cpy = (fifo_buffer.read_cpy + 1) & FIFO_MASK;
  return 0;
}
