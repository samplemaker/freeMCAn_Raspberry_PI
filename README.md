# FreeMCAn geiger counter on Raspberry PI (B+)

FreeMCAn-gc is a geiger pulse counter running on Raspberry Pi (B+). To get optimal performance the software is split into a hostware and a firmware part. The hostware is an user console program to collect and record the counting data. A gnuplot program is able to print the record. The firmware is a native linux kernel module driver which aquires all hardware related stuff. It sets up a periodic timer ISR and counts the geiger events via a hardware IRQ line (gpio).

## Software and system requirements

Prerequisites:

  * [pidora][pidora]

[pidora]:    http://pidora.ca/
             "Pidora Linux"

You have to post install the make environment and the kernel development tools:

yum install raspberrypi-kernel-devel*  
yum install raspberrypi-kernel-headers*


## Building and installing

Go into the hostware and firmware folder and type `make`. The kernel module can be loaded via `insmod firmware_geiger_ts.ko` (must be root!). Then start the hostware (must be root!).

To create access rights for specific users (run the hostware without beeing root) proceed as follows:

  * Adjust the file `50-udev.rules` according to the required user name (default is `fmca`)
  * Copy the file to `/etc/udev/rules.d/`
  * Issue a `udevadm control --reload-rules`

To auto load the firmware during boot proceed as follows:

  * Copy `firmware_geiger_ts.ko` to `/lib/modules/$(uname -r)/`
  * Issue a `depmod -a`
  * You may try to load the firmware via `modprobe firmware_geiger_ts`
  * Check with `lsmod` or have a look at `/proc/interrupts` or check wheather `/dev/freeMCAnPI` exists
  * Copy `firmware_geiger.conf` to `/etc/modules-load.d/`

## Configuration

In a first step you may wish to configure the peripherals:

  * The timebase signaling LED
    * See `#define GPIO_TIMEBASE_LED 35` (default is the Raspberry Pi B+ PWR LED which is a conflict but will help you to start up with the software)
  * The GPIO pin counting signal
    * See `#define GPIO_INTERRUPT_PIN 23`

## The License

LGPLv2.1+



