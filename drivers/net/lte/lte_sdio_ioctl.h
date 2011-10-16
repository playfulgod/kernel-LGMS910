/*
 * LGE L2000 LTE driver for SDIO
 * 
 * Jae-gyu Lee <jaegyu.lee@lge.com>
 * Daeok Kim <daeok.kim@lge.com>, IOCTL member modified 
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

#ifndef _LTESDIOIOCTL_H_
#define _LTESDIOIOCTL_H_

typedef struct
{
	unsigned int offset;
	unsigned int size;	
	unsigned int him_rx_cnt;
	unsigned int him_tx_cnt;
	unsigned int packet_size;
	unsigned int packet_count;
	unsigned char buff[4];
	unsigned int result;
} __attribute__ ((packed)) lte_sdio_ioctl_info;

#define IOCTL_MAGIC 's'

#define IOCTL_WRITE_H_VAL	_IOW(IOCTL_MAGIC, 0, lte_sdio_ioctl_info)
#define IOCTL_READ_L_VAL	_IOR(IOCTL_MAGIC, 1, lte_sdio_ioctl_info)
#define IOCTL_READ_HIM_TX_CNT _IOR(IOCTL_MAGIC, 2, lte_sdio_ioctl_info)
#define IOCTL_READ_HIM_RX_CNT _IOR(IOCTL_MAGIC, 3, lte_sdio_ioctl_info)
#define	IOCTL_LTE_PWR_ON _IOR(IOCTL_MAGIC, 4, lte_sdio_ioctl_info)
#define	IOCTL_LTE_PWR_OFF _IOR(IOCTL_MAGIC, 5, lte_sdio_ioctl_info)
/*BEGIN: 0015454 daeok.kim@lge.com 2011-02-06 */
/*MOD 0015454: [LTE] LTE_GPIO_PWR_OFF IOCTL is added, because of case of LTE SDIO rescan fail */
#define	IOCTL_LTE_GPIO_PWR_OFF _IOR(IOCTL_MAGIC, 6, lte_sdio_ioctl_info)
#define	IOCTL_LTE_SDIO_RESCAN _IOR(IOCTL_MAGIC, 7, lte_sdio_ioctl_info)
#define IOCTL_LTE_SDIO_BOOT _IOR(IOCTL_MAGIC, 8, lte_sdio_ioctl_info)
#define IOCTL_LTE_TEST _IOWR(IOCTL_MAGIC, 9, lte_sdio_ioctl_info)
#ifdef CONFIG_LGE_USB_GADGET_LLDM_DRIVER
#define IOCTL_LTE_LLDM_SDIO _IOWR(IOCTL_MAGIC, 10, lte_sdio_ioctl_info)
#endif

#ifdef CONFIG_LGE_USB_GADGET_LLDM_DRIVER
#define IOIOCTL_MAXNR	11
#else
#define IOIOCTL_MAXNR	10
#endif
/*END: 0015454 daeok.kim@lge.com 2011-02-06 */
#endif //_LTESDIOIOCTL_H_
