/** \file firmware/kernel_firmware.c
 * \brief Freemcan linux kernel core module
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/atomic.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kfifo.h>
#include <asm/uaccess.h>
#include "common_defs.h"


// #define TEST_ON_X86


#define DRIVER_VERSION "v0.1"
#define DRIVER_AUTHOR  "samplemaker"
#define DRIVER_DESC    "freemcan-gc"

#define SUCCESS 0


#ifndef TEST_ON_X86
/* Raspberry PI B+
   PWR LED --> GPIO 35
*/
//#define GPIO_TIMEBASE_LED 24
#define GPIO_TIMEBASE_LED 35
#define GPIO_INTERRUPT_PIN 23
#endif

/** character device related stuff */
static dev_t device_number;
static struct cdev *driver_object;
static struct class *device_class;
static struct device *the_device;

/** high resolution timer for timebase */
static struct hrtimer hrt_timebase;
static ktime_t kt_period, kt_start;
/** timer event counter */
static atomic_t timer_counts = ATOMIC_INIT(1);

static unsigned int timercnts_per_sample = 1;

/** character device single open policy */
static atomic_t dev_use_count = ATOMIC_INIT(-1);

#ifndef TEST_ON_X86
/** interrupt occurence counter */
static atomic_t accu_counts = ATOMIC_INIT(0);
#endif

/** wait queue for blocking/nonblocking read */
static DECLARE_WAIT_QUEUE_HEAD(wq_read);
static atomic_t wup_flag = ATOMIC_INIT(0);
#define UNSET_READABLE_FLAG ( atomic_set( &wup_flag, 0 ) )
#define SET_READABLE_FLAG ( atomic_set( &wup_flag, 1 ) )
#define READABLE_FLAG ( atomic_read( &wup_flag ) != 0 )

/** number of characters in the character ring buffer */
/* must be a power of 2. the maximum number of elements which
   can be stored is FIFO_SIZE_BYTES/[sizeof(fifo_data_t)
   + number of extra characters per line] */
#define FIFO_SIZE_BYTES (1 << 8)

static DECLARE_KFIFO(char_fifo, unsigned char, FIFO_SIZE_BYTES);

/** payload data which is sent over the character device */
typedef struct {
  int timer_counts;
  int kernel_time;
  int accu_counts;
} fifo_data_t;

#ifndef TEST_ON_X86
/** holds the assigned irq line */
static int gpio_irq_in_number;
#endif

/* prototypes */
static enum hrtimer_restart timer_callback(struct hrtimer * unused);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static long device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static unsigned int device_poll (struct file *file, poll_table *wait);
static int __init firmware_init(void);
static void __exit firmware_exit(void);


static struct file_operations device_fops = {
  .owner = THIS_MODULE,
  .read = device_read,
  .unlocked_ioctl = device_ioctl,
  .open = device_open,
  .release = device_release,
  .poll = device_poll
};


#ifndef TEST_ON_X86
static inline
void gpio_toggle(unsigned int gpio_number){
  unsigned int state =  gpio_get_value(gpio_number);
  state = !state;
  gpio_set_value(gpio_number, state);
}


static irqreturn_t
my_interrupt_handler(int irq, void* dev_id)
{
   atomic_inc(&accu_counts);
   return IRQ_HANDLED;
}
#endif


/** Timer callback function for timebase
 *
 * Collect timestamp and accu_counts in the ring buffer.
 * Wake up reader tasks.
 */
static enum hrtimer_restart timer_callback(struct hrtimer * unused)
{
  unsigned char a_line[256];
  fifo_data_t fifo_data;

  /* get the current time stamp */
  ktime_t kt_now = hrtimer_cb_get_time(&hrt_timebase);
  /* periodic timer */
  int overruns = hrtimer_forward(&hrt_timebase, kt_now, kt_period);
  if (overruns > 1)
    printk(KERN_ALERT "freemcan: timer events are missing\n");

  /* the timer softirq does not interrupt itself. timer_counts not
     necessarily atomic? */
  const int act_timer_counts = atomic_read(&timer_counts);
  atomic_inc(&timer_counts);

#ifndef TEST_ON_X86
    gpio_toggle(GPIO_TIMEBASE_LED);
#endif

  /* create each expired timercnts_per_sample a new ringbuffer element */
  if (!(act_timer_counts % timercnts_per_sample)){

    fifo_data.timer_counts = act_timer_counts;

    /* absolute time since start of measurement */
    ktime_t kt_diff = ktime_sub(kt_now, kt_start);
    fifo_data.kernel_time = ktime_to_ms(kt_diff);

#ifndef TEST_ON_X86
    /* function can be interrupted by the accu_count ISR because this
       code is running inside a softirq */
    fifo_data.accu_counts = atomic_xchg(&accu_counts, 0);
#else
    fifo_data.accu_counts = 1234;
#endif

    /* from now on treat the data simply as a stupid character stream */
    /* create a csv style output */
    int len = sprintf(a_line,
                      "event/time/count: ; %d ; %d ; %d\n",
                      fifo_data.timer_counts,
                      fifo_data.kernel_time,
                      fifo_data.accu_counts);

    kfifo_in(&char_fifo, a_line, len);
    //kfifo_in(&char_fifo,(unsigned char *)(&fifo_data),sizeof(fifo_data_t));

    /* data ready to read. wake up reader task if it does sleep and
       inform poll() */
    SET_READABLE_FLAG;
    wake_up_interruptible(&wq_read);
  }

  return HRTIMER_RESTART;
}


/** Device open - service function
 *
 * Collect timestamp and accu_counts in the ring buffer.
 * Wake up reader tasks.
 */
static int
device_open(struct inode *inode, struct file *file)
{
  /* device is read only */
  if (((file->f_flags & O_ACCMODE) == O_WRONLY)
     || ((file->f_flags & O_ACCMODE) == O_RDWR))
    return -EACCES;

  /* brut force single-open policy. possibly that two different tasks
     try to open the device at the same time, therefore atomic */
  if (atomic_inc_and_test(&dev_use_count)){
    /* success, only one instance */
    return SUCCESS;
  }
  /* already opened - reject */
  atomic_dec(&dev_use_count);

  return -EBUSY;
}


/** Device close - service function
 */
static int
device_release(struct inode *inode, struct file *file)
{
  atomic_dec(&dev_use_count);

  return SUCCESS;
}


/** Device read - service function
 *
 * Send ringbuffer payload to user space
 */
static ssize_t
device_read(struct file *filp, char *buffer,
            size_t length, loff_t *offset)
{
  /* no data + nonblocking mode = return without any action */
  if ( (!READABLE_FLAG) &&
       (filp->f_flags & O_NONBLOCK) )
    return -EAGAIN;
  /* blocking mode and if no data available to send then put
     process to SLEEP otherwise continue */
  if (wait_event_interruptible(wq_read, READABLE_FLAG)) {
    /* can be interrupted by a signal (e.g. KILL) during sleep */
    return -ERESTARTSYS;
  }
  /* at this point data is available */

  /* resetting the flag could possibly interrupted by an ISR
     therefore it is atomic */
  UNSET_READABLE_FLAG;

  /* With only one concurrent reader and one concurrent writer,
     we don't need extra locking. direct copy_to_user space
     from the fifo. remove the data copied automatically from the
     ringbuffer */
  int ret_val;
  unsigned int copied;
  ret_val = kfifo_to_user(&char_fifo, buffer, length, &copied);
  // ret_val = kfifo_out(&char_fifo, buffer, length);
  /* -EFAULT or number of bytes copied respectively */
  return ret_val ? ret_val : copied;
}


static inline void
stop_firmware(void){
  /* \TODO what happens if a not running timer is canceled? */
  /* cancel the timer and wait until the ISR executes */
  hrtimer_cancel(&hrt_timebase);
  wake_up_interruptible_all(&wq_read);
}


static inline void
start_firmware(void){
  hrtimer_start(&hrt_timebase, kt_period, HRTIMER_MODE_REL);
  kt_start = hrtimer_cb_get_time(&hrt_timebase);
  atomic_set(&timer_counts, 1);
#ifndef TEST_ON_X86
  disable_irq(gpio_irq_in_number);
  atomic_set(&accu_counts, 0);
  enable_irq(gpio_irq_in_number);
#endif
}


/** Device ioctl - service function
 *
 * Implements the firmware state machine
 */
static long
device_ioctl(struct file *file,
             unsigned int ioctl_num,
             unsigned long ioctl_param)
{
  switch (ioctl_num) {

    case IOCTL_GET_FIFO_LEN:
       /* get the minimum number of readable characters for lets say a
          readAll() implementation in user space */
       { const ssize_t len = kfifo_len(&char_fifo);
       return len; }
    break;
    case IOCTL_START_MEASUREMENT:
       stop_firmware();
       start_firmware();
    break;
    case IOCTL_STOP_MEASUREMENT:
       stop_firmware();
    break;
    case IOCTL_SET_TCNTSPERSAMPLE:
       stop_firmware();
       if (copy_from_user(&timercnts_per_sample,
                         (unsigned int *)ioctl_param,
                         sizeof(unsigned int)) )
         return -EACCES;
    break;
    default:
       printk(KERN_INFO "freemcan: unknown IOCTL:%d\n",ioctl_num);
       return -EBADF;
    break;
  }
  /* printk(KERN_INFO "IOCTL:%d\n",ioctl_num); */
  return SUCCESS;
}


/** Device select and poll - service function
 *
 */
static unsigned int
device_poll (struct file *file, poll_table *wait)
{
  unsigned int mask = 0;

  poll_wait(file, &wq_read, wait);

  /* data ready to read? */
  if (READABLE_FLAG) {
    mask |= POLLIN | POLLRDNORM;
  }
  return mask;
}


/** Firmware initialization
 *
 * Called when insmod is issued
 */
static int
__init firmware_init(void)
{
#ifndef TEST_ON_X86
  int ret_val;
  /* configure the input ourput peripherals and set the directions */
  ret_val = gpio_request_one(GPIO_TIMEBASE_LED, GPIOF_OUT_INIT_LOW, "Timebase LED");
  if (ret_val < 0)
    goto gpio_timebase_exit;

  ret_val = gpio_request_one(GPIO_INTERRUPT_PIN, GPIOF_IN, "IRQ Input Pin");
  if (ret_val < 0)
    goto gpio_irq_exit;

  gpio_irq_in_number = gpio_to_irq(GPIO_INTERRUPT_PIN);
  if (gpio_irq_in_number < 0)
    goto err_irq_return;

  /* NOTE 1:
   * our gpio pin / IRQ is assigned to one driver only (non shared IRQ).
   * therefore we will not pass a uniqe ID but only NULL.
   * NOTE 2:
   * we do not use threaded interrupt processing since our handler is very
   * small. so everything is processed inside interrupt context.
   * NOTE 3: IRQF_DISABLED is not maintained. hence note that IRQs
   * remain enabled except the caller IRQ line(?).
   */
  ret_val = request_irq(gpio_irq_in_number,
                        my_interrupt_handler,
                        IRQF_TRIGGER_RISING |
                        IRQF_TRIGGER_FALLING |
                        IRQF_DISABLED,
                        "freemcan-gc",
                        NULL);
  if (ret_val < 0)
    goto err_irq_return;
#endif

  /* setup the character device */
  /* get device number automatically */
  if (alloc_chrdev_region(&device_number, 0, 1, "freemcan-gc") < 0)
    return -EIO;
  /* character device object to register */
  driver_object = cdev_alloc();
  if (driver_object == NULL)
    goto free_device_number;
  /* protect against module unloading - misuse */
  driver_object->owner = THIS_MODULE;
  driver_object->ops = &device_fops;
  /* register the driver */
  if (cdev_add(driver_object, device_number,1))
    goto free_cdev;
  /* insert into Sysfs to force udev */
  device_class = class_create(THIS_MODULE, "freemcan-gc" );
  if (IS_ERR(device_class)){
    pr_err("device test: no udev support\n");
    goto free_cdev;
  }
  the_device = device_create(device_class,
                             NULL, device_number,
                             NULL, "%s", DEVICE_NAME);

  /* setup the ringbuffer */
  INIT_KFIFO(char_fifo);

  /* setup the timer period (seconds,nanoseconds) */
  kt_period = ktime_set(1, 0);
  /* wanna be independant from systime */
  hrtimer_init (&hrt_timebase, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  hrt_timebase.function = timer_callback;
  if (hrtimer_is_hres_active(&hrt_timebase) > 0)
    printk(KERN_INFO "freemcan: timer has high resolution\n");

  printk(KERN_INFO "freemcan-gc succesfully loaded\n");
  return SUCCESS;

free_cdev:
  kobject_put(&driver_object->kobj);
free_device_number:
  unregister_chrdev_region(device_number, 1);
#ifndef TEST_ON_X86
err_irq_return:
  gpio_free(GPIO_INTERRUPT_PIN);
gpio_irq_exit:
  gpio_free(GPIO_TIMEBASE_LED);
gpio_timebase_exit:
#endif
  return -1;

}

module_init(firmware_init);


/** Firmware cleanup
 *
 * Called when rmmod is issued
 */
static void
__exit firmware_exit(void)
{
#ifndef TEST_ON_X86
  gpio_free(GPIO_INTERRUPT_PIN);
  gpio_free(GPIO_TIMEBASE_LED);
  free_irq(gpio_irq_in_number, NULL);
#endif
  hrtimer_cancel(&hrt_timebase);
  wake_up_interruptible(&wq_read);
  /* erase sysfs item and hence the device file */
  device_destroy(device_class, device_number);
  class_destroy(device_class);
  /* sign off the driver (cdev_del includes kobject_put) */
  cdev_del(driver_object);
  /* free range of reserved device number */
  unregister_chrdev_region( device_number, 1 );
  printk(KERN_INFO "freemcan-gc exit\n");
}

module_exit(firmware_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
