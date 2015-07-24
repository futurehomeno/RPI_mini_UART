#ifndef _BCM2835_MINI_UART_H
#define _BCM2835_MINI_UART_H

#define AUTHOR "Future Home AS <arun@futurehome.no>"
#define DESCRIPTION "BCM285 Mini UArt Driver"
#define LICENSE "GPL"

#define CLOCK 250000000
#define FIFO_SIZE 8
#define IRQ 29
#define MAX_PORTS 1

#define PORT 110

/*
** Auxiliary peripherals registers
** https://www.raspberrypi.org/wp-content/uploads/2012/02/BCM2835-ARM-Peripherals.pdf Section (Auxiliaries: UART1 & SPI1, SPI2)
*/

#define AUX_BASEADDR 0x20215000
#define AUX_IRQ 0x0
#define AUX_ENABLES 0x04

#define AUX_MU_IO_OFFSET 0x40
#define AUX_REG_SIZE 48
#define AUX_MU_IO_REG_OFFSET 0x0
#define AUX_MU_IER_REG_OFFSET 0x4
#define AUX_MU_IIR_REG_OFFSET 0x8
#define AUX_MU_LCR_REG_OFFSET 0xC
#define AUX_MU_MCR_REG_OFFSET 0x10
#define AUX_MU_LSR_REG_OFFSET 0x14
#define AUX_MU_MSR_REG_OFFSET 0x18
#define AUX_MU_SCRATCH_OFFSET 0x1C
#define AUX_MU_CNTL_REG_OFFSET 0x20
#define AUX_MU_STAT_REG_OFFSET 0x24
#define AUX_MU_BAUD_REG_OFFSET 0x28

/* AUX_ENABLES */
#define ENABLE_UART1(x) (x | 0x1)
#define DISABLE_UART1(x) (x & 0x6)

/* AUX_MU_IER_REG */
#define ENABLE_INTERRUPT 0x7
#define DISABLE_INTERRUPT 0x5

/* AUX_MX_IIR_REG */
#define IS_TRANSMIT_INTERRUPT(x) (x & 0x2)
#define IS_RECEIVE_INTERRUPT(x) (x & 0x4)
#define FLUSH_UART 0xC6

/* AUX_MU_LCR_REH */
#define ENABLE_7BIT 0x2
#define ENABLE_8BIT 0x3

/* AUX_MU_LSR_REG */
#define IS_TRANSMITTER_EMPTY(x) (x & 0x10)
#define IS_TRANSMITTER_IDEL(x)  (x & 0x20)
#define IS_DATA_READY(x) (x & 0x1)
#define IS_RECEIVER_OVEERUN(x) (x & 0x2)

/* AUX_MU_CNTL_REG */
#define IS_RECEIVER_ENABLED(x) (x & 0x1)
#define IS_TRANSMITTER_ENABLED(x) (x & 0x2)
#define ENABLE_UART_FUNCTION 0x3
#define DISABLE_UART_FUNCTION 0x0

/* AUX_MU_STAT_REG */
#define IS_TRANSMIT_FIFO_FULL(x) ( x & 0x20 )

#endif /* _BCM2835_MINI_UART_H */