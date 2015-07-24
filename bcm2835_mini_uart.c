#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/console.h>
#include <linux/clk.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/sysrq.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/of.h>

#include "bcm2835_mini_uart.h"

static struct uart_port ports[MAX_PORTS];

static inline unsigned int bcm2835_uart_readl(struct uart_port *port, unsigned int offset)
{
    return __raw_readl(port->membase + offset);
}

static inline void bcm2835_uart_writel(struct uart_port *port, unsigned int value, unsigned int offset)
{
    __raw_writel(value, port->membase + offset);
}

static unsigned int bcm2835_uart_tx_empty(struct uart_port *port)
{
    unsigned int val;
    val = bcm2835_uart_readl(port, AUX_MU_LSR_REG_OFFSET);
    return IS_TRANSMITTER_EMPTY(val);
}

static void bcm2835_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
    bcm2835_uart_writel(port, 206, AUX_MU_CNTL_REG_OFFSET);
}

static unsigned int bcm2835_uart_get_mctrl(struct uart_port *port)
{
    int mctrl = 0, val;
    val = bcm2835_uart_readl(port, AUX_MU_STAT_REG_OFFSET);
    if(!IS_TRANSMIT_FIFO_FULL(val)) {
        mctrl |= TIOCM_CTS;
    }
    return mctrl;
}

static void bcm2835_uart_stop_tx(struct uart_port *port)
{
    // TODO: Implement
}

static void bcm2835_uart_start_tx(struct uart_port *port)
{
    bcm2835_uart_writel(port, ENABLE_INTERRUPT, AUX_MU_IER_REG_OFFSET);
    bcm2835_uart_writel(port, ENABLE_8BIT, AUX_MU_LCR_REG_OFFSET);
}

static void bcm2835_uart_stop_rx(struct uart_port *port)
{
    // TODO: Implement
}

static void bcm2835_uart_enable_ms(struct uart_port *port)
{
    // TODO: Implement
}

static void bcm2835_uart_break_ctl(struct uart_port *port, int ctl)
{

}

static const char *bcm2835_uart_type(struct uart_port *port)
{
    return (port->type == PORT) ? "BCM2835_uart" : NULL;
}

static void bcm2835_uart_do_tx(struct uart_port *port)
{
    struct circ_buf *xmit;
    unsigned int val, max_count;

    if (port->x_char) {
        bcm2835_uart_writel(port, port->x_char, AUX_MU_IO_REG_OFFSET);
        port->icount.tx++;
        port->x_char = 0;
        return;
    }

    if (uart_tx_stopped(port)) {
        bcm2835_uart_stop_tx(port);
        return;
    }

    xmit = &port->state->xmit;
    if (uart_circ_empty(xmit))
        goto txqueue_empty;

    val = bcm2835_uart_readl(port, AUX_MU_STAT_REG_OFFSET);
    val = (val & (0xF<<23)) >> 23;
    max_count = port->fifosize - val;

    while (max_count--) {
        unsigned int c;

        c = xmit->buf[xmit->tail];
        bcm2835_uart_writel(port, c, AUX_MU_IO_REG_OFFSET);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        port->icount.tx++;
        if (uart_circ_empty(xmit))
            break;
    }

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(port);

    if (uart_circ_empty(xmit))
        goto txqueue_empty;
    return;

txqueue_empty:
    bcm2835_uart_writel(port, DISABLE_INTERRUPT, AUX_MU_IER_REG_OFFSET);
    return;
}

static void bcm2835_uart_do_rx(struct uart_port *port)
{
    struct tty_port *tty_port = &port->state->port;
    unsigned int max_count;

    max_count = 8;
    do {
        unsigned int iestat, c, cstat;
        char flag;

        iestat = bcm2835_uart_readl(port, AUX_MU_STAT_REG_OFFSET);
        if (!(iestat & 0xF0000))
            break;

        cstat = c = bcm2835_uart_readl(port, AUX_MU_IO_REG_OFFSET);
        port->icount.rx++;
        flag = TTY_NORMAL;
        c &= 0xff;
        tty_insert_flip_char(tty_port, c, flag);

    } while (--max_count);

    spin_unlock(&port->lock);
    tty_flip_buffer_push(tty_port);
    spin_lock(&port->lock);
}

static irqreturn_t bcm2835_uart_interrupt(int irq, void *dev_id)
{
    struct uart_port *port;
    unsigned int irqstat;

    port = dev_id;
    spin_lock(&port->lock);

    irqstat = bcm2835_uart_readl(port, AUX_MU_IIR_REG_OFFSET);
    printk(KERN_INFO "Got irqstat: %d\n", irqstat);
    if(IS_RECEIVE_INTERRUPT(irqstat))
        bcm2835_uart_do_rx(port);
    if(IS_TRANSMIT_INTERRUPT(irqstat))
        bcm2835_uart_do_tx(port);

    spin_unlock(&port->lock);
    return IRQ_HANDLED;
}

static void bcm2835_uart_enable(struct uart_port *port)
{
    bcm2835_uart_writel(port, ENABLE_UART_FUNCTION, AUX_MU_CNTL_REG_OFFSET);
}

static void bcm2835_uart_disable(struct uart_port *port)
{
    bcm2835_uart_writel(port, DISABLE_UART_FUNCTION, AUX_MU_CNTL_REG_OFFSET);
}

static void bcm2835_uart_flush(struct uart_port *port)
{
    bcm2835_uart_writel(port, FLUSH_UART, AUX_MU_IIR_REG_OFFSET);
}

static int bcm2835_uart_startup(struct uart_port *port)
{
    int ret;
    /* mask all irq and flush port */
    bcm2835_uart_disable(port);
    bcm2835_uart_writel(port, 0, AUX_MU_IER_REG_OFFSET);
    bcm2835_uart_flush(port);

    ret = request_irq(port->irq, bcm2835_uart_interrupt, 0,
              bcm2835_uart_type(port), port);
    if (ret)
        return ret;
    bcm2835_uart_writel(port, ENABLE_INTERRUPT , AUX_MU_IER_REG_OFFSET);
    bcm2835_uart_enable(port);
    return 0;
}

static void bcm2835_uart_shutdown(struct uart_port *port)
{
    bcm2835_uart_disable(port);
    bcm2835_uart_flush(port);
    free_irq(port->irq, port);
}

static void bcm2835_uart_set_termios(struct uart_port *port,
                 struct ktermios *new_termios,
                 struct ktermios *old_termios)
{
    unsigned long flags;
    unsigned int baud, baudrate_reg;
    spin_lock_irqsave(&port->lock, flags);

    /* disable uart while changing speed */
    bcm2835_uart_disable(port);
    bcm2835_uart_flush(port);

    baud = tty_termios_baud_rate(new_termios);

    if(baud == 0)
    {
        baud = 9600;
    }

    baudrate_reg = port->uartclk / (8 * baud) - 1;

    bcm2835_uart_writel(port, ENABLE_8BIT, AUX_MU_LCR_REG_OFFSET);
    bcm2835_uart_writel(port, baudrate_reg, AUX_MU_BAUD_REG_OFFSET);

    bcm2835_uart_enable(port);

    spin_unlock_irqrestore(&port->lock, flags);
}

static int bcm2835_uart_request_port(struct uart_port *port)
{
    unsigned int size;

    size = AUX_REG_SIZE;
    if (!request_mem_region(port->mapbase, size, "BCM2835")) {
        dev_err(port->dev, "Memory region busy\n");
        return -EBUSY;
    }

    port->membase = ioremap(port->mapbase, size);
    if (!port->membase) {
        dev_err(port->dev, "Unable to map registers\n");
        release_mem_region(port->mapbase, size);
        return -EBUSY;
    }
    return 0;
}

static void bcm2835_uart_release_port(struct uart_port *port)
{
    release_mem_region(port->mapbase, AUX_REG_SIZE);
    iounmap(port->membase);
}

static void bcm2835_uart_config_port(struct uart_port *port, int flags)
{
    if (flags & UART_CONFIG_TYPE) {
        if (bcm2835_uart_request_port(port))
            return;
        port->type = PORT;
    }
}

static int bcm2835_uart_verify_port(struct uart_port *port,
                struct serial_struct *serinfo)
{
    if (port->type != PORT)
        return -EINVAL;
    if (port->irq != serinfo->irq)
        return -EINVAL;
    if (port->iotype != serinfo->io_type)
        return -EINVAL;
    if (port->mapbase != (unsigned long)serinfo->iomem_base)
        return -EINVAL;
    return 0;
}

static struct uart_ops bcm2835_uart_ops = {
    .tx_empty       = bcm2835_uart_tx_empty,
    .get_mctrl      = bcm2835_uart_get_mctrl,
    .set_mctrl      = bcm2835_uart_set_mctrl,
    .start_tx       = bcm2835_uart_start_tx,
    .stop_tx        = bcm2835_uart_stop_tx,
    .stop_rx        = bcm2835_uart_stop_rx,
    .enable_ms      = bcm2835_uart_enable_ms,
    .break_ctl      = bcm2835_uart_break_ctl,
    .startup        = bcm2835_uart_startup,
    .shutdown       = bcm2835_uart_shutdown,
    .set_termios    = bcm2835_uart_set_termios,
    .type           = bcm2835_uart_type,
    .release_port   = bcm2835_uart_release_port,
    .request_port   = bcm2835_uart_request_port,
    .config_port    = bcm2835_uart_config_port,
    .verify_port    = bcm2835_uart_verify_port,
};

static void bcm2835_console_write(struct console *co, const char *s, unsigned int count)
{
    printk(KERN_INFO "Writing via console");
}

static int __init bcm2835_console_setup(struct console *co, char *options)
{
    printk(KERN_INFO "Setting up console");
    return -ENODEV;
}

/*
* Rip off from serial.c
 */
struct tty_driver *bcm2835_console_device(struct console *co, int *index)
{
         struct uart_driver *p = co->data;
         *index = co->index;
         return p->tty_driver;
}

static struct uart_driver bcm2835_uart_driver;
static struct console bcm2835_uart_console = {
    .name       = "ttyS",
    .write      = bcm2835_console_write,
    .device     = bcm2835_console_device,
    .setup      = bcm2835_console_setup,
    .flags      = CON_PRINTBUFFER,
    .index      = -1,
    .data       = &bcm2835_uart_driver,
};

#define BCM2835_UART_CONSOLE (&bcm2835_uart_console)

static struct uart_driver bcm2835_uart_driver = {
    .owner          = THIS_MODULE,
    .driver_name    = "ttyS",
    .dev_name       = "ttyS",
    .major          = 205,
    .minor          = 65,
    .nr             = MAX_PORTS,
    .cons           = BCM2835_UART_CONSOLE
};

static int bcm2835_uart_probe(struct platform_device *pdev)
{
    struct uart_port *port;
    int ret;

    __raw_writel(1,(uint32_t *)__io_address(AUX_BASEADDR + AUX_ENABLES));

    if (pdev->dev.of_node)
        pdev->id = of_alias_get_id(pdev->dev.of_node, "uart");
    if (pdev->id < 0 || pdev->id >= MAX_PORTS)
        return -EINVAL;
    if (ports[pdev->id].membase)
        return -EBUSY;

    port = &ports[pdev->id];
    memset(port, 0, sizeof(*port));
    port->iotype = UPIO_MEM;
    port->mapbase = AUX_BASEADDR + AUX_MU_IO_OFFSET;

    port->irq = IRQ;
    port->ops = &bcm2835_uart_ops;
    port->flags = UPF_BOOT_AUTOCONF;
    port->dev = &pdev->dev;
    port->fifosize = FIFO_SIZE;
    port->uartclk = CLOCK;
    port->line = pdev->id;

    ret = uart_add_one_port(&bcm2835_uart_driver, port);
    if (ret) {
        ports[pdev->id].membase = 0;
        return ret;
    }
    platform_set_drvdata(pdev, port);

    return 0;
}

static int bcm2835_uart_remove(struct platform_device *pdev)
{
    struct uart_port *port;

    port = platform_get_drvdata(pdev);
    uart_remove_one_port(&bcm2835_uart_driver, port);
    ports[pdev->id].membase = 0;

    return 0;
}

static struct platform_driver bcm2835_uart_platform_driver = {
    .probe  = bcm2835_uart_probe,
    .remove = bcm2835_uart_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name  = "ttyS",
    },
};

static void probe_device_release(struct device *dev)
{
}

static struct platform_device bcm2835_probe_device = {
    .name = "ttyS",
    .id = 0,
    .dev = {
        .release = probe_device_release,
    },
};

static int __init bcm2835_uart_init(void)
{
    int ret;
    ret = uart_register_driver(&bcm2835_uart_driver);
    if (ret) {
        return ret;
    }
    platform_device_register(&bcm2835_probe_device);
    ret = platform_driver_register(&bcm2835_uart_platform_driver);
    if (ret) {
        uart_unregister_driver(&bcm2835_uart_driver);
    }

    return ret;
}

static void __exit bcm2835_uart_exit(void)
{
    platform_driver_unregister(&bcm2835_uart_platform_driver);
    platform_device_unregister(&bcm2835_probe_device);
    uart_unregister_driver(&bcm2835_uart_driver);
}

module_init(bcm2835_uart_init);
module_exit(bcm2835_uart_exit);

MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESCRIPTION);
MODULE_LICENSE(LICENSE);