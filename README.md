# Raspberry PI BCM2835 mini UART Serial Driver
Check LICENSE on root folder for LICENSE RELATED INFORMATION

This driver is greatly inspired from 
http://brevera.in/blog/software/mini-uart-driver-for-raspberry-pi/

## Installation Instruction

### Preparing for installation

* First you would need to find your kernel version

`uname -r`

* Then download the Linux source (you can just apt-get the source, but the one bundled with Debian system doesn't seem to work) corresponding to your kernel version

```
cd /usr/src
wget https://github.com/raspberrypi/linux/archive/rpi-3.18.y.zip
```

Then unzip it:

```
unzip rpi-3.18.y
mv linux-rpi-3.18.y linux
```

You might even have to upgrade gcc because some gcc version are flagged dangerous for compilations.

* Now create a symlink of the Linux source

`ln -s /usr/src/linux /lib/modules/3.18.14+/build`

* Navigate to the recently created directory symlink, and run following commands:

```
make mrproper
gzip -dc /proc/config.gz > .config
make modules_prepare
```

* You will then need module version information stored in a file `Module.symvers`. You can compile the whole kernel in the rapi to obtain it, but it is not recommended. Fortunately, Module.symvers has already been generated for our ease.

It can be found in:

`https://github.com/raspberrypi/firmware/tree/master/extra`

The kernel version I am working on is `3.18.14+` so my file would be:

`wget https://raw.githubusercontent.com/raspberrypi/firmware/70b05985cf2b2b893bc6e55b12794f32857a78dd/extra/Module.symvers`

You can look through the commit history and decide which commit has your file.

* Run `depmod -a`

### Making the module

* Clone this repo

```
git clone https://github.com/arunpoudel/RPI_mini_UART.git
make
```
