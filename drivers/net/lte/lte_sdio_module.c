/*
 * LGE L2000 LTE driver for SDIO
 * 
 * Jae-gyu Lee <jaegyu.lee@lge.com>
 * Daeok Kim <daeok.kim@lge.com>, Built-in build added & tty IOCTL modified
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
*/

// 100630 DAEOKKIM [BRYCE] Kernel Built-in -start
//#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
// 100630 DAEOKKIM [BRYCE] Kernel Built-in -end

#include "lte_debug.h"
#include "lte_sdio.h"
#include "lte_sdio_ioctl.h"

#include "lte_boot.h"

#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/gpio.h>
#include <linux/slab.h>

#ifdef LTE_ROUTING
#include "lte_tty_lldm.h"
#endif	/*LTE_ROUTING*/
/*BEGIN: 0011460 daeok.kim@lge.com 2010-11-27 */
/*MOD 0011460: [LTE] Since Rev.D, revision check method is modified by using PMIC voltage level check */
extern unsigned char CheckHWRev();
/*END: 0011460 daeok.kim@lge.com 2010-11-27 */

/*BEGIN: 0014627 daeok.kim@lge.com 2011-01-23 */
/*MOD 0014627: [LTE] LTE Power off sequence is added on kernel in AP */
extern int gIrq_lte_hw_wdt;
/*END: 0014627 daeok.kim@lge.com 2011-01-23 */

#define LTE_SDIO_VENDOR_ID    0x0296
#define LTE_SDIO_DEVICE_ID    0x5347

static const struct sdio_device_id lte_sdio_ids[] = {
    { SDIO_DEVICE(LTE_SDIO_VENDOR_ID, LTE_SDIO_DEVICE_ID) },
//    { SDIO_DEVICE_CLASS(SDIO_CLASS_NONE)        },
    { /* end: all zeroes */             },
};

PLTE_SDIO_INFO gLte_sdio_info = NULL;

static struct tty_driver *lte_sdio_tty_driver = NULL;
#ifdef LTE_ROUTING
static struct tty_driver *lte_sdio_tty_lldm_driver = NULL;
static struct device *lte_sdio_tty_lldm_dev;
#endif	/*LTE_ROUTING*/
static struct device *lte_sdio_tty_dev;
static int g_is_sdio_registered = 0;
#ifdef LGE_NO_HARDWARE
static int g_is_no_hardware = 0;
#endif

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
static int g_power_state; // g_power_state=0 indicates L2000 boot state
int irq_tx, irq_rx;
int g_tx_int_enable;	
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

MODULE_DEVICE_TABLE(sdio, lte_sdio_ids);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
//static irqreturn_t L2K_WAKE_UP_TX(int irq, void *dev_id)
static irqreturn_t lte_wake_up_tx(int irq, void *dev_id)
{
	int ret;
	
	//printk(" ************* ISR_tx  **************\n"); //gpio_get_value(GPIO_L2K_LTE_STATUS) );
	
	if(g_power_state && g_tx_int_enable) {
            //disable_irq(irq_tx);		

	    ret = gpio_get_value(GPIO_L2K_LTE_STATUS);			//READ LTE_ST            
	    if(ret) {
                g_tx_int_enable = 0;
	        complete(&gLte_sdio_info->comp);
	        printk(" ************* ISR_tx Start **************\n");	    
	    }
            //printk(" ************* ISR_tx End **************\n");	    	    
	}
	return IRQ_HANDLED;
}

static irqreturn_t lte_wake_up_rx(int irq, void *dev_id)
{

	/* Wake lock for 3 sec */
	wake_lock_timeout(&gLte_sdio_info->lte_wake_lock, HZ / 2);

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-30  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
	//printk(" ************* ISR_rx  **************\n"); //gpio_get_value(GPIO_L2K_HOST_WAKEUP) );
	if(g_power_state) {	
	    gpio_set_value(GPIO_L2K_HOST_STATUS,TRUE);			//Set HOST_ST
	    //printk(" ************* ISR_rx Start **************\n");	    	    
	}
/* END: 0013584: jihyun.park@lge.com 2011-01-30 */   			
		
	return IRQ_HANDLED;
}
#endif
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			


static int lte_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
    int error = 0;
	struct device *dev;

    FUNC_ENTER();
    
    if ((func->vendor != LTE_SDIO_VENDOR_ID) || (func->device != LTE_SDIO_DEVICE_ID))
    {
    	return 0; /*Confirm of Vendor ID and Device ID*/
    }

	LTE_INFO("LTE_DRV_V_1_0_0\n");

    LTE_INFO("vendor ID = 0x%x\n", func->vendor);
    LTE_INFO("device ID = 0x%x\n", func->device);
    LTE_INFO("number of functions = 0x%x\n", func->num);

#if 0
	gLte_sdio_info = kzalloc(sizeof(LTE_SDIO_INFO), GFP_KERNEL);
	if (!gLte_sdio_info)
	{	
        LTE_ERROR("Failed to allocate memory for SDIO\n");
		return -ENOMEM;
	}
#endif

	gLte_sdio_info->func = func;
	
	lte_sdio_drv_init();

	sdio_set_drvdata(func, gLte_sdio_info);

#if 0
	dev = tty_register_device(lte_sdio_tty_driver, 0, &func->dev);
	if(IS_ERR(dev))
	{
        LTE_ERROR("Failed to register TTY device\n");		
		return PTR_ERR(dev);
	}
#endif

	lte_sdio_start_thread();

	lte_sdio_enable_function();

#if 0
	mdelay(500);

	sdio_claim_host(gLte_sdio_info->func);		
	sdio_writeb(gLte_sdio_info->func, 0xCF, 0x48, &error);
	sdio_release_host(gLte_sdio_info->func);
    if(error != 0)
    {
        LTE_ERROR("Failed to write GPR 0xCF\n");
    }
#endif

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
{
    //	define local variables
	int ret_tx=0, ret_rx=0;
	
	gpio_set_value(GPIO_L2K_LTE_WAKEUP, 0); //FALSE);			// LTE_WU = 0 DEFAULT IS LOW
	gpio_set_value(GPIO_L2K_HOST_STATUS, 1); //TRUE);			// HOST_ST = 1 DEFAULT IS HIGH	

//------------------------------ TX (MSM -> L2K) ------------------------------
	irq_tx = gpio_to_irq(GPIO_L2K_LTE_STATUS);
	ret_tx = request_irq(irq_tx, lte_wake_up_tx, IRQF_TRIGGER_RISING, "WAKEUP_TX", NULL);
	if(ret_tx < 0) {
	    LTE_ERROR("Failed to request tx_interrupt handler\n");
	    return -1;  // error occurs when initializing
	}
	printk(" ************* tx irq register **************\n");
	
//------------------------------ RX (L2K -> MSM) ------------------------------
//        gpio_set_value(GPIO_L2K_HOST_STATUS,TRUE);			//HOST_ST = 1 DEFAULT IS HIGH

        irq_rx = gpio_to_irq(GPIO_L2K_HOST_WAKEUP);
        ret_rx = request_irq(irq_rx, lte_wake_up_rx, IRQF_TRIGGER_RISING, "WAKEUP_RX", NULL);
	if(ret_rx < 0) {
	    LTE_ERROR("Failed to request rx_interrupt handler\n");
	    return -1; // error occurs when initializing
	}
		
        ret_rx = set_irq_wake(irq_rx, 1);
	if(ret_rx < 0) {
	    LTE_ERROR("Failed to wakeup rx_interrupt handler\n");
	    return -1;  // error occurs when initializing
	}        
	printk(" ************* rx irq register **************\n");
		
//------------------------------ DISABLE IRQ()s ------------------------------
//        disable_irq(irq_rx); //GPIO_L2K_HOST_WAKEUP);
        disable_irq(irq_tx); //GPIO_L2K_LTE_STATUS);

}
#endif
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

    FUNC_EXIT();

    return error;
}
    
static void lte_sdio_remove(struct sdio_func *func)
{
    FUNC_ENTER();

/*BEGIN: 0014627 daeok.kim@lge.com 2011-01-23 */
/*MOD 0014627: [LTE] LTE Power off sequence is added on kernel in AP */
	LTE_INFO("Remove FCN of LTE SDIO driver by LTE power off\n");
	lte_sdio_drv_deinit();

	/* Commentted out for LTE power off sequence*/
	//lte_sdio_disable_function();

#if 0
	tty_unregister_device(lte_sdio_tty_driver, 0);
#endif

//	lte_sdio_end_thread();
/*END: 0014627 daeok.kim@lge.com 2011-01-23 */

#if 0
	kfree(gLte_sdio_info);
#endif

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */
#ifdef LTE_WAKE_UP
	free_irq(irq_tx, FALSE);	
	free_irq(irq_rx, FALSE);	
#endif
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

    FUNC_EXIT();
}

static struct sdio_driver sdio_lte_driver = {
    .probe      = lte_sdio_probe,
    .remove     = lte_sdio_remove,
    .name       = "lte_sdio",
    .id_table   = lte_sdio_ids,
};

static int lte_sdio_tty_open(struct tty_struct *tty, struct file *filep)
{
    FUNC_ENTER();

	if(gLte_sdio_info == NULL)
	{
		FUNC_EXIT();
		return -ENXIO;
	}
	
	LTE_INFO("Opened TTY driver for LTE!\n");

	tty->driver_data = gLte_sdio_info;
	gLte_sdio_info->tty = tty;

    FUNC_EXIT();
    return 0;
}

static int lte_sdio_tty_close(struct tty_struct *tty, struct file *filep)
{
    FUNC_ENTER();

	LTE_INFO("Closed TTY driver for LTE! Bye Mommy!\n");	

    FUNC_EXIT();
    return 0;
}

static ssize_t lte_sdio_tty_write(struct tty_struct * tty, const unsigned char *buf, int count)
{
	int error = 0;

/* BEGIN: 0018446 jaegyu.lee@lge.com 2011-03-23 */
/* MOD 0018446: [LTE] Delete FUNC_ENTER & FUNC_EXIT in lte_sdio_tty_write()*/   
//    FUNC_ENTER();
/* END: 0018446 jaegyu.lee@lge.com 2011-03-23 */

/* BEGIN: 0018522 jaegyu.lee@lge.com 2011-03-24 */
/* MOD 0018522: [LTE] Kernel crash happen, When the SDIO TX & RX fail in 5sec */
	if(gLte_sdio_info->flag_gpio_l2k_host_status == FALSE){
		/* BEGIN: 0018684 seungyeol.seo@lge.com 2011-03-30 */
		/* MOD 0018684: [LTE] Remove a debug code for tracking the unwoken host */
		// LTE_INFO(" *********** MSM SDCC doesn't resume *********** \n");
		/* END: 0018684 seungyeol.seo@lge.com 2011-03-30 */
		return 0;
	}
/* END: 0018522 jaegyu.lee@lge.com 2011-03-24 */


/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	mutex_lock(&gLte_sdio_info->tx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

	wake_lock(&gLte_sdio_info->tx_control_wake_lock);
/*BEGIN: 0017497 daeok.kim@lge.com 2011-03-05 */
/*MOD 0017497: [LTE] LTE SW Assert stability is update: blocking of tx operation */
	if (gLte_sdio_info->flag_tx_blocking == TRUE)
	{
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */
		LTE_ERROR("[LTE_ASSERT] SDIO Tx(CTRL message) blocking, return 0 \n");
		return 0;
	}
/*END: 0017497 daeok.kim@lge.com 2011-03-05 */

	error = lte_him_write_control_packet(buf, count);
	if(error != 0)
	{
		LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
		wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

		return 0;
	}
/* BEGIN: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* MOD 0017997: [LTE] Wake-lock period rearrange in LTE SDIO driver */
	wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
/* END: 0017997 jaegyu.lee@lge.com 2011-03-15 */
/* BEGIN: 0018385 seungyeol.seo@lge.com 2011-03-22 */
/* MOD 0018385 : [LTE] Rearrangement of 'Wake Lock' section */
	mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
/* END: 0018385 seungyeol.seo@lge.com 2011-03-22 */

/* BEGIN: 0018446 jaegyu.lee@lge.com 2011-03-23 */
/* MOD 0018446: [LTE] Delete FUNC_ENTER & FUNC_EXIT in lte_sdio_tty_write()*/   
//    FUNC_EXIT();
/* END: 0018446 jaegyu.lee@lge.com 2011-03-23 */


	return count;
}

static ssize_t lte_sdio_tty_write_room(struct tty_struct *tty)
{
    FUNC_ENTER();
    FUNC_EXIT();
    return (ssize_t)2048;
}

static int lte_sdio_tty_chars_in_buffer(struct tty_struct *tty)
{
    FUNC_ENTER();
    FUNC_EXIT();
    return 0;
}

static void lte_sdio_tty_send_xchar(struct tty_struct *tty, char ch)
{
    FUNC_ENTER();
    FUNC_EXIT();
}

static void lte_sdio_tty_throttle(struct tty_struct *tty)
{
    FUNC_ENTER();
    FUNC_EXIT();
}

static void lte_sdio_tty_unthrottle(struct tty_struct *tty)
{
    FUNC_ENTER();
    FUNC_EXIT();
}

#define RELEVANT_IFLAG(iflag) ((iflag) & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

static void lte_sdio_tty_set_termios(struct tty_struct *tty, struct ktermios *old_termios)
{
    FUNC_ENTER();
    unsigned int cflag;

    cflag = tty->termios->c_cflag;

    /* check that they really want us to change something */
    if (old_termios) {
            if ((cflag == old_termios->c_cflag) &&
                (RELEVANT_IFLAG(tty->termios->c_iflag) == 
                 RELEVANT_IFLAG(old_termios->c_iflag))) {
                    printk(KERN_DEBUG " - nothing to change...\n");
                    return;
            }
    }

    /* get the byte size */
    switch (cflag & CSIZE) {
            case CS5:
                    printk(KERN_DEBUG " - data bits = 5\n");
                    break;
            case CS6:
                    printk(KERN_DEBUG " - data bits = 6\n");
                    break;
            case CS7:
                    printk(KERN_DEBUG " - data bits = 7\n");
                    break;
            default:
            case CS8:
                    printk(KERN_DEBUG " - data bits = 8\n");
                    break;
    }
    
    /* determine the parity */
    if (cflag & PARENB)
            if (cflag & PARODD)
                    printk(KERN_DEBUG " - parity = odd\n");
            else
                    printk(KERN_DEBUG " - parity = even\n");
    else
            printk(KERN_DEBUG " - parity = none\n");

    /* figure out the stop bits requested */
    if (cflag & CSTOPB)
            printk(KERN_DEBUG " - stop bits = 2\n");
    else
            printk(KERN_DEBUG " - stop bits = 1\n");

    /* figure out the hardware flow control settings */
    if (cflag & CRTSCTS)
            printk(KERN_DEBUG " - RTS/CTS is enabled\n");
    else
            printk(KERN_DEBUG " - RTS/CTS is disabled\n");
    
    /* determine software flow control */
    /* if we are implementing XON/XOFF, set the start and 
     * stop character in the device */
    if (I_IXOFF(tty) || I_IXON(tty)) {
            unsigned char stop_char  = STOP_CHAR(tty);
            unsigned char start_char = START_CHAR(tty);

            /* if we are implementing INBOUND XON/XOFF */
            if (I_IXOFF(tty))
                    printk(KERN_DEBUG " - INBOUND XON/XOFF is enabled, "
                            "XON = %2x, XOFF = %2x", start_char, stop_char);
            else
                    printk(KERN_DEBUG" - INBOUND XON/XOFF is disabled");

            /* if we are implementing OUTBOUND XON/XOFF */
            if (I_IXON(tty))
                    printk(KERN_DEBUG" - OUTBOUND XON/XOFF is enabled, "
                            "XON = %2x, XOFF = %2x", start_char, stop_char);
            else
                    printk(KERN_DEBUG" - OUTBOUND XON/XOFF is disabled");
    }

    /* get the baud rate wanted */
    printk(KERN_DEBUG " - baud rate = %d", tty_get_baud_rate(tty));

    FUNC_EXIT();
}

static int lte_sdio_tty_break_ctl(struct tty_struct *tty, int break_state)
{
    FUNC_ENTER();
    FUNC_EXIT();
	return 0;
}

/* fake UART values */
#define MCR_DTR         0x01
#define MCR_RTS         0x02
#define MCR_LOOP        0x04
#define MSR_CTS         0x08
#define MSR_CD          0x10
#define MSR_RI          0x20
#define MSR_DSR         0x40

static int lte_sdio_tty_tiocmget(struct tty_struct *tty, struct file *file)
{
    unsigned int result = 0;
    unsigned int msr = gLte_sdio_info->msr;
    unsigned int mcr = gLte_sdio_info->mcr;

    FUNC_ENTER();

    result = ((mcr & MCR_DTR)  ? TIOCM_DTR  : 0) |  /* DTR is set */
         ((mcr & MCR_RTS)  ? TIOCM_RTS  : 0) |      /* RTS is set */
         ((mcr & MCR_LOOP) ? TIOCM_LOOP : 0) |      /* LOOP is set */
         ((msr & MSR_CTS)  ? TIOCM_CTS  : 0) |      /* CTS is set */
         ((msr & MSR_CD)   ? TIOCM_CAR  : 0) |      /* Carrier detect is set*/
         ((msr & MSR_RI)   ? TIOCM_RI   : 0) |      /* Ring Indicator is set */
         ((msr & MSR_DSR)  ? TIOCM_DSR  : 0);       /* DSR is set */

    FUNC_EXIT();

    return result;	
}

static int lte_sdio_tty_tiocmset(struct tty_struct *tty, struct file *file,
			      unsigned int set, unsigned int clear)
{
    unsigned int mcr = gLte_sdio_info->mcr;

    FUNC_ENTER();

    if (set & TIOCM_RTS)
            mcr |= MCR_RTS;
    if (set & TIOCM_DTR)
            mcr |= MCR_RTS;

    if (clear & TIOCM_RTS)
            mcr &= ~MCR_RTS;
    if (clear & TIOCM_DTR)
            mcr &= ~MCR_RTS;

    /* set the new MCR value in the device */
    gLte_sdio_info->mcr = mcr;
	
    FUNC_EXIT();
	return 0;
}

static int lte_sdio_tty_ioctl(struct tty_struct *tty, struct file *file,
		 unsigned int cmd, unsigned long arg)
{
	lte_sdio_ioctl_info *ctrl_info;
	int error = 1, i = 0;
	unsigned int size, offset;
	void __user *argp = (void __user *)arg;

	unsigned int pwr_value =100;
	unsigned int core_en_value =100;
	unsigned int rst_value =100;
	
/*BEGIN: 0014925 daeok.kim@lge.com 2011-01-27 */
/*ADD 0014925: [LTE] HWRev_val is added due to overloading of RPC in the calling CheckHWRev() */
	unsigned char HWRev_Val =0; // For calling once CheckHWRev FCN
/*END: 0014925 daeok.kim@lge.com 2011-01-27 */
//    FUNC_ENTER();

	if(_IOC_TYPE(cmd) != IOCTL_MAGIC) return -EINVAL;
	if(_IOC_NR(cmd) >= IOIOCTL_MAXNR) return -EINVAL;

#if 0
	size = _IOC_SIZE(cmd);

	if(size)
	{
		error = 0;
		if(_IOC_DIR(cmd) & _IOC_READ)
			error = verify_area(VERIFY_WRITE, (void*) arg, size);
		else if(_IOC_DIR(cmd) & _IOC_WRITE)
			error = verify_area(VERIFY_READ, (void*) arg, size);

		if(error)
			return error;
	}
#endif

	ctrl_info = kmalloc(sizeof(lte_sdio_ioctl_info), GFP_KERNEL);
	if(!ctrl_info)
		return -ENOMEM;

	if(copy_from_user(ctrl_info, argp, sizeof(lte_sdio_ioctl_info)))
	{
		kfree(ctrl_info);
		return -ENOMEM;
	}

	size = ctrl_info->size;
	offset = ctrl_info->offset;
	
	switch(cmd)
	{
		case IOCTL_WRITE_H_VAL :
			sdio_claim_host(gLte_sdio_info->func);			
			sdio_writeb(gLte_sdio_info->func, ctrl_info->buff[0], offset, &error);
			sdio_release_host(gLte_sdio_info->func);			
			break;

		case IOCTL_READ_L_VAL :
			sdio_claim_host(gLte_sdio_info->func);			
			ctrl_info->buff[0] = sdio_readb(gLte_sdio_info->func, offset, &error);
			sdio_release_host(gLte_sdio_info->func);			
			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;			

		case IOCTL_READ_HIM_TX_CNT :
			ctrl_info->him_tx_cnt = gLte_sdio_info->him_tx_cnt;
			LTE_INFO("\n");
			LTE_INFO("TX count = %d\n", ctrl_info->him_tx_cnt);
			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;			

		case IOCTL_READ_HIM_RX_CNT :
			ctrl_info->him_rx_cnt = gLte_sdio_info->him_rx_cnt;
			LTE_INFO("RX count = %d\n", ctrl_info->him_rx_cnt);
			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;	
		case IOCTL_LTE_PWR_ON :
			/*Initial setting of Power on GPIO*/
			gpio_tlmm_config(GPIO_CFG(154, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/* kwangdo.yi@lge.com S 2010.09.04
	replaced with gpio_direction_output to fix build err
*/
#if 0			
			gpio_configure(154, GPIOF_DRIVE_OUTPUT);
#else
			gpio_direction_output(154, 0);
#endif

#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#else
/*BEGIN: 0011460 daeok.kim@lge.com 2010-11-27 */
/*MOD 0011460: [LTE] Since Rev.D, revision check method is modified by using PMIC voltage level check */
			/*Initial setting of 1.2V CORE_EN GPIO*/
#if defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) /*Update when there is HW revision*/
				gpio_tlmm_config(GPIO_CFG(83, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/* kwangdo.yi@lge.com S 2010.09.04
	replaced with gpio_direction_output to fix build err
*/
#if 0			
			gpio_configure(83, GPIOF_DRIVE_OUTPUT);
#else
			gpio_direction_output(83, 0);
#endif
#endif
#endif // defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)

#if defined (LGE_HW_MS910_REV2) || defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#elif defined (LGE_HW_MS910_REV1)
	gpio_tlmm_config(GPIO_CFG(80, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(80, 0);
#else
/*BEGIN: 0014925 daeok.kim@lge.com 2011-01-27 */
/*ADD 0014925: [LTE] HWRev_val is added due to overloading of RPC in the calling CheckHWRev() */
			HWRev_Val = CheckHWRev();
/*END: 0014925 daeok.kim@lge.com 2011-01-27 */

			if (HWRev_Val == 'D')
			{
				gpio_tlmm_config(GPIO_CFG(83, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
				gpio_direction_output(83, 0);
				printk("[LTE_SDIO] Rev.D , CheckHWRev: rev.%c \n", HWRev_Val);
			}
/*BEGIN: 0013863 daeok.kim@lge.com 2011-01-11 */
/*ADD 0013863: [LTE] For Rev.I, HW revision check for L2K_1V2_core_en is added  */	 
			else if ((HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
/*END: 0013310 daeok.kim@lge.com 2011-01-11 */
			{
				gpio_tlmm_config(GPIO_CFG(80, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
				gpio_direction_output(80, 0);
				printk("[LTE_SDIO] Since Rev.D, CheckHWRev: Rev.%c \n", HWRev_Val);
			}
			else
			{
				printk("[LTE_SDIO] If your build option have been Rev.D, this is wrong!! \n");
			}
#endif /* LGE_HW_MS910_REV1 */

/*BEGIN: 0010961 daeok.kim@lge.com 2010-11-16 */
/*MOD 0010961: [LTE] GPIO (L2K_1V2_CORE_EN) is changed: 83 ->80 */ 
//#if defined(LG_HW_REV7)
			//gpio_tlmm_config(GPIO_CFG(80, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			//gpio_direction_output(80, 0);
//#endif
/*END: 0010961 daeok.kim@lge.com 2010-11-16 */
/*END: 0011460 daeok.kim@lge.com 2010-11-27 */
			/*Initial setting of Reset & SDIO slot rescan GPIO*/
			gpio_tlmm_config(GPIO_CFG(157, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
/* kwangdo.yi@lge.com S 2010.09.04
	replaced with gpio_direction_output to fix build err
*/
#if 0			
			gpio_configure(157, GPIOF_DRIVE_OUTPUT);
#else
			gpio_direction_output(157, 0);
#endif

			gpio_set_value(154,0);
/*BEGIN: 0011460 daeok.kim@lge.com 2010-11-27 */
/*MOD 0011460: [LTE] Since Rev.D, revision check method is modified by using PMIC voltage level check */
#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2) || defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#else
#if defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) /*Update when there is HW revision*/
				gpio_set_value(83,0);
#endif
#endif // #if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)

#if defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#elif defined (LGE_HW_MS910_REV1)
	gpio_set_value(80,0);
#else
			if (HWRev_Val == 'D')
			{
				gpio_set_value(83,0);
			}
/*BEGIN: 0013863 daeok.kim@lge.com 2011-01-11 */
/*ADD 0013863: [LTE] For Rev.I, HW revision check for L2K_1V2_core_en is added  */
			else if ((HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
/*END: 0013863 daeok.kim@lge.com 2011-01-11 */
			{
				gpio_set_value(80,0);
			}
			else
			{
				printk("[LTE_SDIO] If your build option have been Rev.D, this is wrong!! \n");
			}
#endif /* LGE_HW_MS910_REV1 */
/*BEGIN: 0010961 daeok.kim@lge.com 2010-11-16 */
/*MOD 0010961: [LTE] GPIO (L2K_1V2_CORE_EN) is changed: 83 ->80 */ 
//#if defined(LG_HW_REV7)
			//gpio_set_value(80,0);
//#endif
/*END: 0010961 daeok.kim@lge.com 2010-11-16 */
			gpio_set_value(157,0);
			udelay(100);

#if defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) /*Update when there is HW revision*/
	gpio_set_value(154,1);
#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#else
	gpio_set_value(83,1);
#endif //#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#endif

#if defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
	gpio_set_value(154,1);
#elif defined (LGE_HW_MS910_REV1)
	gpio_set_value(154,1);
	gpio_set_value(80,1);
#else
			if (HWRev_Val == 'D')
			{
				gpio_set_value(154,1);
				gpio_set_value(83,1);
			}
/*BEGIN: 0013863 daeok.kim@lge.com 2011-01-11 */
/*ADD 0013863: [LTE] For Rev.I, HW revision check for L2K_1V2_core_en is added  */
			else if ((HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
/*END: 0013863 daeok.kim@lge.com 2011-01-11 */
			{
				gpio_set_value(154,1);
				gpio_set_value(80,1);
			}
			else
			{
				printk("[LTE_SDIO] If your build option have been Rev.D, this is wrong!! \n");
			}
#endif /* LGE_HW_MS910_REV1 */
/*BEGIN: 0010961 daeok.kim@lge.com 2010-11-16 */
/*MOD 0010961: [LTE] GPIO (L2K_1V2_CORE_EN) is changed: 83 ->80 */ 
//#if defined(LG_HW_REV7)
			//gpio_set_value(80,1);
//#endif
/*END: 0010961 daeok.kim@lge.com 2010-11-16 */
			mdelay(60); // For power sequence
			gpio_set_value(157,1);
			
			pwr_value = gpio_get_value(154);
#if defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) /*Update when there is HW revision*/
#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#else
				core_en_value = gpio_get_value(83);
#endif //#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#endif

#if defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#elif defined (LGE_HW_MS910_REV1)
	core_en_value = gpio_get_value(80);
#else
			if (HWRev_Val == 'D')
			{
				core_en_value = gpio_get_value(83);
			}
/*BEGIN: 0013863 daeok.kim@lge.com 2011-01-11 */
/*ADD 0013863: [LTE] For Rev.I, HW revision check for L2K_1V2_core_en is added  */
			else if ((HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
/*END: 0013863 daeok.kim@lge.com 2011-01-11 */
			{
				core_en_value = gpio_get_value(80);
			}
			else
			{
				printk("[LTE_SDIO] If your build option have been Rev.D, this is wrong!! \n");
			}
#endif /* LGE_HW_MS910_REV1 */
/*BEGIN: 0010961 daeok.kim@lge.com 2010-11-16 */
/*MOD 0010961: [LTE] GPIO (L2K_1V2_CORE_EN) is changed: 83 ->80 */ 
//#if defined(LG_HW_REV7)
			//core_en_value = gpio_get_value(80);
//#endif
/*END: 0010961 daeok.kim@lge.com 2010-11-16 */
			rst_value = gpio_get_value(157);
			mdelay(20);
#if defined(LG_HW_REV3) || defined(LG_HW_REV4) || defined(LG_HW_REV5) /*Update when there is HW revision*/
				printk("[LTE_SDIO] pwr[%d] core_en[%d] rst[%d]\n", pwr_value, core_en_value, rst_value);
#endif
#if defined (LGE_HW_MS910_REV1) || defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#else
/*BEGIN: 0013863 daeok.kim@lge.com 2011-01-11 */
/*ADD 0013863: [LTE] For Rev.I, HW revision check for L2K_1V2_core_en is added  */
			if ((HWRev_Val == 'D') || (HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
/*END: 0013863 daeok.kim@lge.com 2011-01-11 */
#endif /* LGE_HW_MS910_REV1 */
			{
				printk("[LTE_SDIO] Since Rev.D, pwr[%d] core_en[%d] rst[%d]\n", pwr_value, core_en_value, rst_value);
			}
/*END: 0011460 daeok.kim@lge.com 2010-11-27 */			

			/*Try to check SDIO probing during 4second(80x50ms)*/
			for (i=0; i<80; i++)
			{
				if (gLte_sdio_info->func == NULL)
				{
					mdelay(50);
				}
				else
				{
					printk("[LTE_SDIO_RESCAN] Probing time: %d ms \n",(i+1)*50);
					break;
				}
			}
			//gpio_set_value(3,0); //rescan MMC, Slot Rescan
			//gpio_set_value(3,1); //GPIO_LTE_SDIO_DETECT =3, Active low
			
			if (gLte_sdio_info->func == NULL)
			{
				printk("[LTE_SDIO_RESCAN] HW problem : SDIO Probing FAILURE \n");
				ctrl_info->result = 0; /*LTE PWR on & 1st Rescan Failure return*/
			        	
			}	
			else
			{
				printk("[LTE_SDIO_RESCAN] 1st TRY is Probing SUCCESS\n");
				ctrl_info->result = 1; /*LTE PWR on &1st Rescan Success return*/	
			}
			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;
		
		case IOCTL_LTE_PWR_OFF :
#if defined (CONFIG_MACH_LGE_BRYCE_MS910)
#else
/*BEGIN: 0014925 daeok.kim@lge.com 2011-01-27 */
/*ADD 0014925: [LTE] HWRev_val is added due to overloading of RPC in the calling CheckHWRev() */
			HWRev_Val = CheckHWRev();
/*END: 0014925 daeok.kim@lge.com 2011-01-27 */
#endif /* MS910 */			

/*BEGIN: 0014627 daeok.kim@lge.com 2011-01-23 */
/*MOD 0014627: [LTE] LTE Power off sequence is added on kernel in AP */
			printk("[LTE_SDIO_PWR_OFF] Start IOCTL_LTE_PWR_OFF \n");

			/* IRQ Disable for LTE HW WDT RST*/
			disable_irq(gIrq_lte_hw_wdt);
			free_irq(gIrq_lte_hw_wdt, NULL);
			/* sdio_release_irq & sdio_disable_func*/ 
			lte_sdio_disable_function();
			/* IRQ Disable for LTE WAKE-UP*/
			disable_irq(irq_rx); //GPIO_L2K_HOST_WAKEUP
			disable_irq(irq_tx); //GPIO_L2K_LTE_STATUS
			/* Initialization of Global value*/
			g_power_state =0;
			g_tx_int_enable =0;
			
			/* GPIO control for LTE power off sequence*/
			gpio_set_value(154,0); // L2K Power GPIO
#if defined (LGE_HW_MS910_REV2)|| defined (LGE_HW_MS910_REV3) || defined (LGE_HW_MS910_REV4)|| defined (LGE_HW_MS910_REV5)
#elif defined (LGE_HW_MS910_REV1)
			gpio_set_value(80,0); // L2K_1V2_CORE_EN GPIO
#else
			if (HWRev_Val == 'D')
			{
				gpio_set_value(83,0); // L2K_1V2_CORE_EN GPIO
				printk("[LTE_SDIO_PWR_OFF] Rev.D , CheckHWRev: rev.%c \n", HWRev_Val);
			}
			else if ((HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
			{
				gpio_set_value(80,0); // L2K_1V2_CORE_EN GPIO
				printk("[LTE_SDIO_PWR_OFF] Since Rev.D, CheckHWRev: Rev.%c \n", HWRev_Val);
			}
			else
			{
				printk("[LTE_SDIO_PWR_OFF] If your build option have been Rev.D, this is wrong!! \n");
			}
#endif /* CONFIG_MACH_LGE_BRYCE_MS910 */
			mdelay(60); // For power sequence
			gpio_set_value(157,0); // L2K Reset GPIO
			mdelay(100);
/*END: 0014627 daeok.kim@lge.com 2011-01-23 */

			ctrl_info->result = 0;
 			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;

/*BEGIN: 0015454 daeok.kim@lge.com 2011-02-06 */
/*MOD 0015454: [LTE] LTE_GPIO_PWR_OFF IOCTL is added, because of case of LTE SDIO rescan fail */
		case IOCTL_LTE_GPIO_PWR_OFF:
			/* In case of HW problem, Only LTE GPIO power off*/
			/* GPIO control for LTE power off sequence*/
#if defined (CONFIG_MACH_LGE_BRYCE_MS910)
#else
			HWRev_Val = CheckHWRev();
			gpio_set_value(154,0); // L2K Power GPIO
			if (HWRev_Val == 'D')
			{
				gpio_set_value(83,0); // L2K_1V2_CORE_EN GPIO
				printk("[LTE_SDIO_GPIO_PWR_OFF] Rev.D , CheckHWRev: rev.%c \n", HWRev_Val);
			}
			else if ((HWRev_Val == 'E') || (HWRev_Val == 'F') || (HWRev_Val == 'G') 
				|| (HWRev_Val == 'H') || (HWRev_Val == 'I') || (HWRev_Val == '0') 
				|| (HWRev_Val == '1') || (HWRev_Val == '2') || (HWRev_Val == '3')) /*Update when there is HW revision*/
			{
				gpio_set_value(80,0); // L2K_1V2_CORE_EN GPIO
				printk("[LTE_SDIO_GPIO_PWR_OFF] Since Rev.D, CheckHWRev: Rev.%c \n", HWRev_Val);
			}
			else
			{
				printk("[LTE_SDIO_GPIO_PWR_OFF] If your build option have been Rev.D, this is wrong!! \n");
			}
#endif /* CONFIG_MACH_LGE_BRYCE_MS910 */
			mdelay(60); // For power sequence
			gpio_set_value(157,0); // L2K Reset GPIO
			mdelay(100);

			ctrl_info->result = 0;
			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;
/*END: 0015454 daeok.kim@lge.com 2011-02-06 */
		case IOCTL_LTE_SDIO_RESCAN:	
			/* Do not use this case in the Bryce*/
			if (gLte_sdio_info->func != NULL)
			{
				printk("[LTE_SDIO_RESCAN] 2nd No TRY \n");
				printk("[LTE_SDIO_RESCAN] 2nd TRY is Probing SUCCESS\n");
				ctrl_info->result = 1; /*2nd Rescan Success return*/
			}
			else
			{
				printk("[LTE_SDIO_RESCAN] 2nd TRY is Probing FAILURE\n");
				ctrl_info->result = 0; /*2nd Rescan Failure return*/
			}			
 			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;
			
		case IOCTL_LTE_SDIO_BOOT :
			/* LTE SDIO boot */
			ctrl_info->result = lte_sdio_boot();
 			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;

/* BEGIN: 0013584: jihyun.park@lge.com 2011-01-05  */
/* [LTE] Wakeup Scheme  for Power Saving Mode   */				
			if(!ctrl_info->result)
				g_power_state=1; // power control allowed, here!
/* END: 0013584: jihyun.park@lge.com 2011-01-05 */   			

			break;

		case IOCTL_LTE_TEST :
			LTE_INFO("Test SDIO/HIM driver\n");
#ifdef SDIO_HIM_TEST
			gLte_sdio_info->test_packet_size = ctrl_info->packet_size;
			gLte_sdio_info->test_packet_count = ctrl_info->packet_count;
			up(&gLte_sdio_info->test_sem);
#endif
			break;

#ifdef CONFIG_LGE_USB_GADGET_LLDM_DRIVER
		case IOCTL_LTE_LLDM_SDIO :
			LTE_INFO("[LTE_POEWR_STATE] = %d \n", g_power_state);
			ctrl_info->result = g_power_state;
 			if(copy_to_user(argp, ctrl_info, sizeof(lte_sdio_ioctl_info)))
				error = -EFAULT;
			break;
#endif

	}
	kfree(ctrl_info);	
	
//    FUNC_EXIT();

	return error;
}
		 
static const struct tty_operations lte_sdio_tty_ops = {
	.open			= lte_sdio_tty_open,
	.close			= lte_sdio_tty_close,
	.write			= lte_sdio_tty_write,
	.write_room		= lte_sdio_tty_write_room,
	.chars_in_buffer = lte_sdio_tty_chars_in_buffer,
	.send_xchar		= lte_sdio_tty_send_xchar,
	.throttle		= lte_sdio_tty_throttle,
	.unthrottle		= lte_sdio_tty_unthrottle,
	.set_termios	= lte_sdio_tty_set_termios,
	.break_ctl		= lte_sdio_tty_break_ctl,
	.tiocmget		= lte_sdio_tty_tiocmget,
	.tiocmset		= lte_sdio_tty_tiocmset,
	.ioctl			= lte_sdio_tty_ioctl,
};

static int lte_sdio_tty_init(void)
{
	int error = 0;
	struct tty_driver *tty_drv;
	struct device *dev;
	
    FUNC_ENTER();	

	lte_sdio_tty_driver = tty_drv = alloc_tty_driver(1);
	if(!tty_drv)
	{
		LTE_ERROR("Failed to allocate memory for TTY driver\n");
		return -ENOMEM;
	}
	tty_drv->owner = THIS_MODULE;
	tty_drv->driver_name = "ttyLTE";
	tty_drv->name = "ttyLTE";
	tty_drv->major = 0;  /* dynamically allocated */
	tty_drv->minor_start = 0;
	tty_drv->type = TTY_DRIVER_TYPE_SERIAL;
	tty_drv->subtype = SERIAL_TYPE_NORMAL;
	tty_drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty_drv->init_termios = tty_std_termios;
	/*BEGIN: 0011683 daeok.kim@lge.com 2010-12-01 */
	/*MOD 0011683: [LTE] Delete input flag which is ignore CR in TTY driver  */ 
	tty_drv->init_termios.c_iflag = IGNBRK | IGNPAR;
	/*END: 0011683 daeok.kim@lge.com 2010-12-01 */
	tty_drv->init_termios.c_oflag = 0;
	tty_drv->init_termios.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
	tty_drv->init_termios.c_lflag = 0;
	
	tty_set_operations(tty_drv, &lte_sdio_tty_ops);

	error = tty_register_driver(tty_drv);
	if (error)
	{
		LTE_ERROR("Failed to register TTY driver\n");		
	        lte_sdio_tty_driver = NULL;
		return error;
	}

	// 100630 DAEOKKIM [BRYCE] Kernel Built-in -start
	dev = tty_register_device(lte_sdio_tty_driver, 0, NULL);
	if(IS_ERR(dev))
	{
        LTE_ERROR("Failed to register TTY device\n");		
	        tty_unregister_driver(tty_drv);
	        lte_sdio_tty_driver = NULL;
		return PTR_ERR(dev);
	}
	lte_sdio_tty_dev = dev;
	// 100630 DAEOKKIM [BRYCE] Kernel Built-in -end

	FUNC_EXIT();
	
	return 0;
}

static int lte_sdio_tty_deinit(void)
{
	FUNC_ENTER();
	if(lte_sdio_tty_driver != NULL)
	{
		tty_unregister_device(lte_sdio_tty_driver,0);
		tty_unregister_driver(lte_sdio_tty_driver);
		lte_sdio_tty_driver = NULL;
	}
	FUNC_EXIT();
	return 0;
}
#ifdef LTE_SYSFS
/* show TX queue size */
static ssize_t sysfs_txqueue_show(struct device *d, struct device_attribute *attr, char *buf)
{
	unsigned long c = 0,p = 0;
	struct list_head *c_tiem, *c_temp;
	if(gLte_sdio_info)
	{
		list_for_each_safe(c_tiem, c_temp, &gLte_sdio_info->tx_packet_head->list)
		{
			c++;
		}
	}
	return sprintf(buf, "%lu\n",c);
}

static DEVICE_ATTR(txqueue, 0444, sysfs_txqueue_show, NULL);

static ssize_t sysfs_rxcnt_show(struct device *d, struct device_attribute *attr, char *buf)
{
	unsigned long c = 0;
	struct list_head *c_tiem, *c_temp;
	if(gLte_sdio_info)
	{
		c = gLte_sdio_info->him_rx_cnt;
	}
	return sprintf(buf, "%lu\n",c);
}

static DEVICE_ATTR(rxcnt, 0444, sysfs_rxcnt_show, NULL);


static ssize_t sysfs_txcnt_show(struct device *d, struct device_attribute *attr, char *buf)
{
	unsigned long c = 0;
	struct list_head *c_tiem, *c_temp;
	if(gLte_sdio_info)
	{
		c = gLte_sdio_info->him_tx_cnt;
	}
	return sprintf(buf, "%lu\n",c);
}

static DEVICE_ATTR(txcnt, 0444, sysfs_txcnt_show, NULL);

static int lte_sdio_sysfs_init(void)
{
	if(lte_sdio_tty_dev)
	{
	    device_create_file(lte_sdio_tty_dev, &dev_attr_txqueue);
	    device_create_file(lte_sdio_tty_dev, &dev_attr_rxcnt);
	    device_create_file(lte_sdio_tty_dev, &dev_attr_txcnt);
	}
#ifdef LTE_ROUTING
	else if(lte_sdio_tty_lldm_dev)
	{
	    device_create_file(lte_sdio_tty_lldm_dev, &dev_attr_txqueue);
	    device_create_file(lte_sdio_tty_lldm_dev, &dev_attr_rxcnt);
	    device_create_file(lte_sdio_tty_lldm_dev, &dev_attr_txcnt);
	}
#endif	/*LTE_ROUTING*/
}

static int lte_sdio_sysfs_deinit(void)
{
	if(lte_sdio_tty_dev)
	{
	    device_remove_file(lte_sdio_tty_dev, &dev_attr_txqueue);
	    device_remove_file(lte_sdio_tty_dev, &dev_attr_rxcnt);
	    device_remove_file(lte_sdio_tty_dev, &dev_attr_txcnt);
	}
#ifdef LTE_ROUTING
	else if(lte_sdio_tty_lldm_dev)
	{
	    device_remove_file(lte_sdio_tty_lldm_dev, &dev_attr_txqueue);
	    device_remove_file(lte_sdio_tty_lldm_dev, &dev_attr_rxcnt);
	    device_remove_file(lte_sdio_tty_lldm_dev, &dev_attr_txcnt);
	}
#endif	/*LTE_ROUTING*/
}


#endif

static int __init lte_sdio_init(void)
{
    int error = 0;
	
    FUNC_ENTER();

    gLte_sdio_info = NULL;

// 100630 DAEOKKIM [BRYCE] Kernel Built-in -start
	gLte_sdio_info = kzalloc(sizeof(LTE_SDIO_INFO), GFP_KERNEL);
	if (!gLte_sdio_info)
	{	
        LTE_ERROR("Failed to allocate memory for SDIO\n");
		return -ENOMEM;
	}
// 100630 DAEOKKIM [BRYCE] Kernel Built-in -end

	error = lte_sdio_tty_init();
    if(error != 0)
    {
        LTE_ERROR("Failed to initialize TTY driver. Error = %x\n",error);
		goto err;
    }

#ifdef LTE_ROUTING
	error = lte_sdio_tty_lldm_init();
	if(error != 0)
    {
        LTE_ERROR("Failed to initialize TTY_LLDM driver. Error = %x\n",error);
		goto err;
    }
#endif	/*LTE_ROUTING*/

	error = sdio_register_driver(&sdio_lte_driver);
    if(error != 0)
    {
        LTE_ERROR("Failed to register sdio driver. Error = %x\n",error);
		if(gLte_sdio_info != NULL)
			{
				kfree(gLte_sdio_info);		
				gLte_sdio_info = NULL;
		}
		if(lte_sdio_tty_driver != NULL)
		{
			tty_unregister_driver(lte_sdio_tty_driver);
			lte_sdio_tty_driver = NULL;
		}
#ifdef LTE_ROUTING
		if(lte_sdio_tty_lldm_driver != NULL)
		{
			tty_unregister_driver(lte_sdio_tty_lldm_driver);
			lte_sdio_tty_lldm_driver = NULL;
		}
#endif	/*LTE_ROUTING*/
		g_is_sdio_registered = 0;
		goto err;
	}
	else
	{
		g_is_sdio_registered = 1;
    }

#ifdef LGE_NO_HARDWARE
	/* this is case when we have no SDIO hardware, but */
	/* want to test driver                             */
	msleep_interruptible(500);
	if(gLte_sdio_info == NULL)
	{
		LTE_INFO("No SDIO LTE hardware, use emulation\n");
		gLte_sdio_info = kzalloc(sizeof(LTE_SDIO_INFO), GFP_KERNEL);
		if(!gLte_sdio_info)
		{	
   			LTE_ERROR("Failed to allocate memory for SDIO\n");
			error = -ENOMEM;
			goto err;
		}
		lte_sdio_drv_init();
		lte_sdio_start_thread();
		g_is_no_hardware = 1;
	}
	else
	{
		g_is_no_hardware = 0;
    }
#endif
#ifdef LTE_SYSFS
	lte_sdio_sysfs_init();
#endif

	error = 0;
err:
    FUNC_EXIT();
    
    return error;
}

static void __exit lte_sdio_exit(void)
{
    FUNC_ENTER();

#ifdef LTE_SYSFS
	lte_sdio_sysfs_deinit();
#endif

	lte_sdio_tty_deinit();
#ifdef LTE_ROUTING
	lte_sdio_tty_lldm_deinit();
#endif	/*LTE_ROUTING*/

#ifdef LGE_NO_HARDWARE
	if(g_is_no_hardware)
	{
	    lte_sdio_drv_deinit();
	    lte_sdio_end_thread();
	}
#endif
	if(g_is_sdio_registered)
	{
		sdio_unregister_driver(&sdio_lte_driver);
		g_is_sdio_registered = 0;
	}
	if(gLte_sdio_info != NULL)
	{
		kfree(gLte_sdio_info);
		gLte_sdio_info = NULL;
	}

    FUNC_EXIT();
}

#ifndef MODULE
__initcall (lte_sdio_init);
#else
module_init(lte_sdio_init);
module_exit(lte_sdio_exit);
#endif

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("LTE L2000 LTE driver for SDIO");
MODULE_LICENSE("GPL");
