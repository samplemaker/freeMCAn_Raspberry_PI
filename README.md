# FreeMCAn geiger counter on Raspberry PI (B+) as a kernel module and QT client

FreeMCAn-gc is a geiger pulse counter running on Raspberry Pi (B+). To get optimal performance the software is split into a hostware and a firmware part. The hostware is an user console program to collect and record the counting data. Alternatively a GUI related QT version is available. The firmware is a native linux kernel module driver which aquires all hardware related stuff. It sets up a periodic timer ISR and counts the geiger events via a hardware IRQ line (gpio).

[Screenshot](https://github.com/samplemaker/freeMCAn_Raspberry_PI/blob/freeMCAn_Raspberry_PI_github/screenshots/freemcan_in_action.png)

## Software and system requirements

Prerequisites:

  * [pidora][pidora]
  * [Qt API (Version 5)][QtHomepage]
  * [g++/gcc - Gnu Compiler Collection][gcc]

[QtHomepage]:  http://qt-project.org/
             "Qt aplication programming interface"
[gcc]:       http://gcc.gnu.org/
             "GNU Compiler Collection"
[pidora]:    http://pidora.ca/
             "Pidora Linux"

You have to post install the make environment, QT5 devel and the kernel development tools:

yum install qt5-qtbase-devel*  
yum install qt5-qtbase-5*  
yum install qt5-qtbase-x11*  
yum install raspberrypi-kernel-devel*  
yum install raspberrypi-kernel-headers*


## Building and installing of kernel module (firmware)

Go into the firmware folder and type `make`. The kernel module can be loaded via `insmod firmware_geiger_ts.ko` (must be root!). Then start the hostware (must be root!).

To create access rights for specific users (run the hostware without beeing root) proceed as follows:

  * Adjust the file `50-udev.rules` according to the required user name and access rights
  * Copy the file to `/etc/udev/rules.d/`
  * Issue a `udevadm control --reload-rules`

To auto load the firmware during boot proceed as follows:

  * Copy `firmware_geiger_ts.ko` to `/lib/modules/$(uname -r)/`
  * Issue a `depmod -a`
  * You may try to load the firmware via `modprobe firmware_geiger_ts`
  * Check with `lsmod` or have a look at `/proc/interrupts` or check wheather `/dev/freeMCAnPI` exists
  * Copy `firmware_geiger.conf` to `/etc/modules-load.d/`


## Building the QT based hostware

  * Proceed to `hostware_qt` folder
  * Execute `/usr/bin/qmake-qt5 hostware_qt.pro` and then type `make`


## Configuration

In a first step you may wish to configure the peripherals:

  * The timebase signaling LED
    * See `#define GPIO_TIMEBASE_LED 35` (default is the Raspberry Pi B+ PWR LED which is a conflict but will help you to start up with the software)
  * The GPIO pin counting signal
    * See `#define GPIO_INTERRUPT_PIN 23`

## The License

LGPLv2.1+



