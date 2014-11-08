/** \file include/common_defs.h
 * \brief Common defines for user and kernel source
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

#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <linux/ioctl.h>

/** the character device name (location is /dev/) */
#define DEVICE_NAME "freeMCAnPI"

/** specify a unique ioctl magic number to avoid conflicts */
/* [linux-3.12.23]$cat ./Documentation/ioctl/ioctl-number.txt */
#define IOC_MAGIC 0xe0

enum IOCTL_CMDS{
  IOCTL_GET_FIFO_LEN =  _IOR(IOC_MAGIC, 0, size_t),
  IOCTL_START_MEASUREMENT = _IO(IOC_MAGIC, 1),
  IOCTL_STOP_MEASUREMENT = _IO(IOC_MAGIC, 2),
  IOCTL_SET_SLOW = _IO(IOC_MAGIC, 3),
  IOCTL_SET_FAST = _IO(IOC_MAGIC, 4),
};

#endif
