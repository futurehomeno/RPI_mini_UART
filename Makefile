obj-m += bcm2835_mini_uart.o

all:
	make -C /usr/src/linux M=$(PWD) modules
install:
	cp bcm2835_mini_uart.ko /lib/modules/3.18.14+/kernel/arch/arm/bcm2835_mini_uart/
clean:
	make -C /usr/src/linux M=$(PWD) clean
