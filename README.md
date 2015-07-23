# Raspberry PI BCM2835 mini UART Serial Driver
Check LICENSE on root folder for LICENSE RELATED INFORMATION

This driver is greatly inspired from 
http://brevera.in/blog/software/mini-uart-driver-for-raspberry-pi/

## Installation Instruction

### Preparing for installation

* First you would need to find your kernel version

`uname -r`

* Then clone the linux source (you can just we the source, but the one buldled with debian system doesn't seem to work) corresponding to your kernel version

`git clone https://github.com/raspberrypi/linux.git`

Then checkout the branch that corresponds to your linux version:

`git checkout rpi-3.18.y`

You might even have to upgrade gcc because some gcc version are flagged dangerous for compilations.

* Now create a symlink of the clone linux source

`ln -s /usr/src/linux /lib/modules/3.18.14+/build`

* Navigate to the recently created directory symlink, and run following commands:

```
make mrproper
gzip -dc /proc/config.gz > .config
make modules_prepare
```

* You will then need module version information stored in a file `Module.symvers`. You can compile the whole kernel in the rapi to obtain it, but it is not receommended. Fortunately, Module.symvers has already been generated for our ease.

It can be found in:

`https://github.com/raspberrypi/firmware/tree/master/extra`

The kernel version I am working on is `3.18.14+` so my file would be:

`https://raw.githubusercontent.com/raspberrypi/firmware/70b05985cf2b2b893bc6e55b12794f32857a78dd/extra/Module.symvers`

You can look through the commit history and decide which commit has you file.

* Run `depmod -a`

### Making the module

* Checkout the repo

```
git checkout https://github.com/arunpoudel/RPI_mini_UART.git
make
```
