/*
 * LGE L2000 LTE HIM layer
 * 
 * Jae-gyu Lee <jaegyu.lee@lge.com>
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

#ifndef LTE_HIM_H
#define LTE_HIM_H

#ifndef LTE_ROUTING
#define LTE_ROUTING
#endif
#include <linux/list.h>

#define	HIM_STRUCTURE_VER_3_0
#define LG_LTE_CHANGE_HIM_MDM_TYPE

typedef struct 
{
#ifdef	HIM_STRUCTURE_VER_2_0
	unsigned int	total_block_size;	/* Total Size */
	unsigned int	seq_num; 			/* Sequence Number */
	unsigned int	packet_num; 		/* Number of Packets */
	unsigned char	dst_core;			/* Destination Core */
	unsigned char	src_core;			/* Source Core */
	unsigned short	dummy_1;
	unsigned int	dummy_2;
	unsigned int	him_block_state;	 /* HIM Block State for LTE Chipset */
#else /* HIM_STRUCTURE_VER_3_0 */
	unsigned int	total_block_size;	 /* Total Size */
	unsigned int	seq_num; 			 /* Sequence Number */
	unsigned int	packet_num; 		 /* Number of Packets */
	unsigned short	dummy_1;				 /* Reserved for LTE */
	unsigned short	him_block_type;
	unsigned int	dummy_2;				 /* Reserved for LTE */
	unsigned int	him_block_state;	 /* HIM Block State for LTE Chipset */
#endif

}him_block_header;

typedef struct 
{
	unsigned int	guard1;
	unsigned int	guard2;
	unsigned short	payload_size; /* Packet Size */
	unsigned short	dummy;
	unsigned short	packet_type;	/* Packet Type */
}him_packet_header;

#ifdef	HIM_STRUCTURE_VER_2_0
#define	HIM_NIC_1						0x0008
#define	HIM_NIC_2						0x0009
#define	HIM_NIC_3						0x000A
#define	HIM_CONTROL_PACKET			0xF0FF
#define	HIM_MODEM						0xF011
#define	HIM_SERIAL_1					0xF021
#define	HIM_SERIAL_2					0xF022
#define	HIM_AUTO_INSTALL				0xF051
#define	HIM_NV							0xF051
#define	HIM_EM							0xF052
#define	HIM_RF_CAL						0xF053

#define	HIM_IP_PACKET_1				0xF054
#define	HIM_IP_PACKET_2				0xF055
#define	HIM_IP_PACKET_3				0xF056
#else /* HIM_STRUCTURE_VER_3_0 */
#define	HIM_NIC_1						0x0008
#define	HIM_NIC_2						0x0009
#define	HIM_NIC_3						0x000A
#ifdef	LG_LTE_CHANGE_HIM_MDM_TYPE
#define	HIM_MDM_ATC						0xF011
#define	HIM_MDM_PPP						0xF012
#define	HIM_MDM_TO_LTE					0xF013
#else
#define	HIM_MODEM						0xF011
#endif
#define	HIM_SERIAL_1					0xF021 //SDIO SER1 (CDMA USB Serial)
#define	HIM_SERIAL_2					0xF022 //SDIO SER2 (NMEA)
#define	HIM_SERIAL_3					0xF023 //SDIO SER2 (LBS) 
#define	HIM_SERIAL_4					0xF024 
#define	HIM_NV							0xF051
#define	HIM_RF_CAL						0xF052
#define	HIM_EM							0xF053
#define	HIM_CONTROL_PACKET			0xF058
#define	HIM_IP_PACKET_1				0x0058
#define	HIM_IP_PACKET_2				0x0059
#define	HIM_IP_PACKET_3				0x005A
#define	HIM_IP_PACKET_4				0x005B
#define	HIM_IP_PACKET_5				0x005C
#define	HIM_IP_PACKET_6				0x005D
#define	HIM_IP_PACKET_7				0x005E
#define	HIM_IP_PACKET_8				0x005F
#define	HIM_ARP_PACKET					0x0608
#define	HIM_DRV_NOTI					0x0609
#define	HIM_OID							0x060A
#ifdef	LG_LTE_SFN
#define	HIM_MDM_INT_EP					0xF018
#else
#define	HIM_MDM_INT_EP					0xF056
#endif
#define	HIM_LTE_EM						0xF0E1
#endif

#ifdef	HIM_STRUCTURE_VER_2_0
#define	HIM_BLOCK_HEADER_SIZE		24
#else /* HIM_STRUCTURE_VER_3_0 */
#define	HIM_BLOCK_HEADER_SIZE		24
#endif

#define	HIM_PACKET_HEADER_SIZE 		14

#ifdef	HIM_STRUCTURE_VER_2_0
#define	HIM_CORE_WCDMA			0x23
#define	HIM_CORE_LTE			0x04
#define	HIM_CORE_PC				0xC0
#endif

#ifdef	HIM_STRUCTURE_VER_3_0
#define	HIM_BLOCK_NIC			0x0000
#define	HIM_BLOCK_IP			0x0001
/* BEGIN: 0012603 jaegyu.lee@lge.com 2010-12-17 */
/* MOD 0012603: [LTE] HIM block parsing policy change */
#define	HIM_BLOCK_TOOL			0x0003
#define	HIM_BLOCK_MISC			0x000F
#define	HIM_BLOCK_ARP			0x0004
#define HIM_BLOCK_ERROR			0x000E
/* END: 0012603 jaegyu.lee@lge.com 2010-12-17 */
#ifdef LTE_ROUTING
#define HIM_LLDM_PACKET			0xF051
#endif /*LTE_ROUTING*/

#endif

//#define	HIM_BLOCK_TX_MAX_SIZE	6144
#define	HIM_BLOCK_TX_MAX_SIZE	6656
#define	HIM_BLOCK_RX_MAX_SIZE	12800

#define	HIM_BLOCK_BYTE_ALIGN	4

typedef struct _tx_packet_list
{
	struct list_head list;
	unsigned char *tx_packet;
	unsigned int size;
}tx_packet_list;

typedef int (*lte_him_cb_fn_t)(void *, unsigned int, int);

#ifdef LTE_ROUTING
int lte_him_write_control_packet_tty_lldm(const unsigned char *data, int size);
#endif /*LTE_ROUTING*/
int lte_him_parsing_blk(void);
int lte_him_register_cb_from_ved(lte_him_cb_fn_t cb);
int lte_him_write_control_packet(const unsigned char *data, int size);
int lte_him_enqueue_ip_packet(unsigned char *data, int size, int pdn);

#endif /*LTE_HIM_H*/
