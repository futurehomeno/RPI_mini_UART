obj-m += bcm2835_mini_uart.o

all:
	+make -C "/lib/modules/$(shell uname -r)/build" M=$(PWD) modules
install:
	cp bcm2835_mini_uart.ko "/lib/modules/$(shell uname -r)/kernel/arch/arm/bcm2835_mini_uart/"
clean:
	+make -C "/lib/modules/$(shell uname -r)/build" M=$(PWD) clean
