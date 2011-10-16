/*
 * TTY Driver for LLDM
 * 
 * seongmook Yim <seongmook.yim@lge.com>
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

#include "lte_debug.h"
#include "lte_sdio.h"

#include <linux/slab.h>

#include "lte_sdio_ioctl.h"

//seongmook.yim
#include "lte_him.h"
#include <asm/uaccess.h>
#include<asm/io.h>

extern PLTE_SDIO_INFO gLte_sdio_info;

//unsigned char tempbuff[2048];

static struct tty_driver *lte_sdio_tty_lldm_driver = NULL;
static struct device *lte_sdio_tty_lldm_dev;

//void write_temp_buff(char* input){
//	strcpy(temp_buff,input);
//}

int lte_sdio_tty_lldm_open(struct tty_struct *tty_lldm, struct file *filep)
{
    FUNC_ENTER();

	if(gLte_sdio_info == NULL)
	{
		FUNC_EXIT();
		return -ENXIO;
	}
	
	LTE_INFO("Opened TTY_LLDM driver for LTE!\n");

	gLte_sdio_info->lldm_flag = TRUE;
	LTE_INFO("lldm_flag = %d\n",gLte_sdio_info->lldm_flag);
	
	tty_lldm->driver_data = gLte_sdio_info;
	gLte_sdio_info->tty_lldm = tty_lldm;

	/*Don't Split, when data TX to SDIO*/
	set_bit(TTY_NO_WRITE_SPLIT, &tty_lldm->flags);
	
	
    FUNC_EXIT();
    return 0;
}

int lte_sdio_tty_lldm_close(struct tty_struct *tty_lldm, struct file *filep)
{
    FUNC_ENTER();

	LTE_INFO("Closed TTY_LLDM driver for LTE! Bye Mommy!\n");	

	/*Don't Split, when data TX to SDIO*/
	clear_bit(TTY_NO_WRITE_SPLIT,&tty_lldm->flags);
	
	/*Initialize lldm_flag*/
	gLte_sdio_info->lldm_flag = FALSE;
	LTE_INFO("lldm_flag = %d\n",gLte_sdio_info->lldm_flag);

	FUNC_EXIT();
    return 0;
}

ssize_t lte_sdio_tty_lldm_write(struct tty_struct *tty_lldm, const unsigned char *buf, int count)
{
	int error = 0;

	/* buffer pointer null check */
	
	if(buf == NULL)
	{
		LTE_ERROR("Buffer Pointer is NULL!!\n");
		return 0;
	}
	
	/* Checking HOST wake-up status */
	if(gLte_sdio_info->flag_gpio_l2k_host_status == FALSE){
		return 0;
	}

	mutex_lock(&gLte_sdio_info->tx_lock_mutex);
	wake_lock(&gLte_sdio_info->tx_control_wake_lock);

	/* Blocking of tx operation */
	if (gLte_sdio_info->flag_tx_blocking == TRUE)	
	{
		wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
		LTE_ERROR("[LTE_ASSERT] SDIO Tx(CTRL message) blocking, return 0 \n");
		return 0;
	}
	
	error = lte_him_write_control_packet_tty_lldm(buf, count);

	if(error != 0)
	{
		LTE_ERROR("Failed to wrtie data to LTE. Error = 0x%x\n", error);
		wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
		mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
		return 0;
	}
	wake_unlock(&gLte_sdio_info->tx_control_wake_lock);
	mutex_unlock(&gLte_sdio_info->tx_lock_mutex);
	return count;
}

ssize_t lte_sdio_tty_lldm_write_room(struct tty_struct *tty_lldm)
{
    FUNC_ENTER();
    FUNC_EXIT();
   // return (ssize_t)2048;
    return (ssize_t)6618;
   // return (ssize_t)8192;
}

int lte_sdio_tty_lldm_chars_in_buffer(struct tty_struct *tty_lldm)
{
    FUNC_ENTER();
    FUNC_EXIT();
    return 0;
}

void lte_sdio_tty_lldm_send_xchar(struct tty_struct *tty_lldm, char ch)
{
    FUNC_ENTER();
    FUNC_EXIT();
}

void lte_sdio_tty_lldm_throttle(struct tty_struct *tty_lldm)
{
    //FUNC_ENTER();
    //FUNC_EXIT();
}

void lte_sdio_tty_lldm_unthrottle(struct tty_struct *tty_lldm)
{
    //FUNC_ENTER();
    //FUNC_EXIT();
}

#define RELEVANT_IFLAG(iflag) ((iflag) & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

void lte_sdio_tty_lldm_set_termios(struct tty_struct *tty_lldm, struct ktermios *old_termios)
{
    FUNC_ENTER();
    unsigned int cflag;

    cflag = tty_lldm->termios->c_cflag;

    /* check that they really want us to change something */
    if (old_termios) {
            if ((cflag == old_termios->c_cflag) &&
                (RELEVANT_IFLAG(tty_lldm->termios->c_iflag) == 
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
    if (I_IXOFF(tty_lldm) || I_IXON(tty_lldm)) {
            unsigned char stop_char  = STOP_CHAR(tty_lldm);
            unsigned char start_char = START_CHAR(tty_lldm);

            /* if we are implementing INBOUND XON/XOFF */
            if (I_IXOFF(tty_lldm))
                    printk(KERN_DEBUG " - INBOUND XON/XOFF is enabled, "
                            "XON = %2x, XOFF = %2x", start_char, stop_char);
            else
                    printk(KERN_DEBUG" - INBOUND XON/XOFF is disabled");

            /* if we are implementing OUTBOUND XON/XOFF */
            if (I_IXON(tty_lldm))
                    printk(KERN_DEBUG" - OUTBOUND XON/XOFF is enabled, "
                            "XON = %2x, XOFF = %2x", start_char, stop_char);
            else
                    printk(KERN_DEBUG" - OUTBOUND XON/XOFF is disabled");
    }

    /* get the baud rate wanted */
    printk(KERN_DEBUG " - baud rate = %d", tty_get_baud_rate(tty_lldm));

    FUNC_EXIT();
}

int lte_sdio_tty_lldm_break_ctl(struct tty_struct *tty_lldm, int break_state)
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

int lte_sdio_tty_lldm_tiocmget(struct tty_struct *tty_lldm, struct file *file)
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

int lte_sdio_tty_lldm_tiocmset(struct tty_struct *tty_lldm, struct file *file,
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

int lte_sdio_tty_lldm_ioctl(struct tty_struct *tty_lldm, struct file *file,
		 unsigned int cmd, unsigned long arg)
{
	//seongmook.yim 1/31
	LTE_INFO(" [READ_LTE_TTY_LLDM] ***** Start *****\n");
	
	lte_sdio_ioctl_info *ctrl_info;
	
	unsigned char *tmp_him_blk;
	int error = 1, i = 0;
	unsigned int size, offset;
	void __user *argp = (void __user *)arg;

	//seongmook.yim
	//int tty_lldm_string=0;
	//int tty_lldm_ioctl=0;
	
	unsigned int pwr_value =100;
	unsigned int core_en_value =100;
	unsigned int rst_value =100;

  //  FUNC_ENTER();

	if(_IOC_TYPE(cmd) != IOCTL_MAGIC) return -EINVAL;		//Determine the CMD is vaild or not
	if(_IOC_NR(cmd) >= IOIOCTL_MAXNR) return -EINVAL;		//EINVAL = Can't processing 


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
		/*
		case IOCTL_LLDM_WRITE :

					LTE_INFO(" ***** Start IOCTL_LLDM_WRITE *****\n");
					tty_lldm_ioctl=copy_from_user(ctrl_info, argp, sizeof(ctrl_info));

					if(tty_lldm_ioctl > 0)
					{
						LTE_INFO("[LLDM_IOCTL] fail copy_from_user data = %d\n",tty_lldm_ioctl);
					}
															
					//memset(ctrl_info->test_buff,NULL,sizeof(ctrl_info->test_buff));
					LTE_INFO(" ***** End IOCTL_LLDM_WRITE *****\n");
					break;
		*/
	}
	return error;
}

const struct tty_operations lte_sdio_tty_lldm_ops = {
	.open			= lte_sdio_tty_lldm_open,
	.close			= lte_sdio_tty_lldm_close,
	.write			= lte_sdio_tty_lldm_write,
	.write_room		= lte_sdio_tty_lldm_write_room,
	.chars_in_buffer = lte_sdio_tty_lldm_chars_in_buffer,
	.send_xchar		= lte_sdio_tty_lldm_send_xchar,
	.throttle		= lte_sdio_tty_lldm_throttle,
	.unthrottle		= lte_sdio_tty_lldm_unthrottle,
	.set_termios	= lte_sdio_tty_lldm_set_termios,
	.break_ctl		= lte_sdio_tty_lldm_break_ctl,
	.tiocmget		= lte_sdio_tty_lldm_tiocmget,
	.tiocmset		= lte_sdio_tty_lldm_tiocmset,
	.ioctl			= lte_sdio_tty_lldm_ioctl,
};

int lte_sdio_tty_lldm_init(void)
{
	int error = 0;
	struct tty_driver *tty_lldm_drv;
	struct device *dev;
	
    FUNC_ENTER();	

	//seongmook.yim -0405
	/*Initialize lldm_flag*/
	gLte_sdio_info->lldm_flag = FALSE;
	
	lte_sdio_tty_lldm_driver = tty_lldm_drv = alloc_tty_driver(1);
	if(!tty_lldm_drv)
	{
		LTE_ERROR("Failed to allocate memory for TTY_LLDM driver\n");
		return -ENOMEM;
	}
	tty_lldm_drv->owner = THIS_MODULE;
	tty_lldm_drv->driver_name = "tty_lldm_LTE";
	tty_lldm_drv->name = "tty_lldm_LTE";
	tty_lldm_drv->major = 0;  /* dynamically allocated */
	tty_lldm_drv->minor_start = 0;
	tty_lldm_drv->type = TTY_DRIVER_TYPE_SERIAL;
	tty_lldm_drv->subtype = SERIAL_TYPE_NORMAL;
	tty_lldm_drv->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	tty_lldm_drv->init_termios = tty_std_termios;
	/*BEGIN: 0011683 daeok.kim@lge.com 2010-12-01 */
	/*MOD 0011683: [LTE] Delete input flag which is ignore CR in TTY driver  */ 
	tty_lldm_drv->init_termios.c_iflag = IGNBRK | IGNPAR;
	/*END: 0011683 daeok.kim@lge.com 2010-12-01 */
	tty_lldm_drv->init_termios.c_oflag = 0;
	tty_lldm_drv->init_termios.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
	tty_lldm_drv->init_termios.c_lflag = 0;
	
	tty_set_operations(tty_lldm_drv, &lte_sdio_tty_lldm_ops);

	error = tty_register_driver(tty_lldm_drv);
	if (error)
	{
		LTE_ERROR("Failed to register TTY_LLDM driver\n");		
	        lte_sdio_tty_lldm_driver = NULL;
		return error;
	}

	// 100630 DAEOKKIM [BRYCE] Kernel Built-in -start
	dev = tty_register_device(lte_sdio_tty_lldm_driver, 0, NULL);
	if(IS_ERR(dev))
	{
        LTE_ERROR("Failed to register TTY_LLDM device\n");		
	        tty_unregister_driver(tty_lldm_drv);
	        lte_sdio_tty_lldm_driver = NULL;
		return PTR_ERR(dev);
	}
	lte_sdio_tty_lldm_dev = dev;
	// 100630 DAEOKKIM [BRYCE] Kernel Built-in -end

	FUNC_EXIT();
	
	return 0;
}

int lte_sdio_tty_lldm_deinit(void)
{
	FUNC_ENTER();
	if(lte_sdio_tty_lldm_driver != NULL)
	{
		tty_unregister_device(lte_sdio_tty_lldm_driver,0);
		tty_unregister_driver(lte_sdio_tty_lldm_driver);
		lte_sdio_tty_lldm_driver = NULL;
	}
	FUNC_EXIT();
	return 0;
}
