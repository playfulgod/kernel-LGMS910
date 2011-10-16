/* 
 * arch/arm/mach-msm/lge/lge_ers.c
 *
 * Copyright (C) 2009 LGE, Inc
 * Author: Jun-Yeong Han <j.y.han@lge.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/limits.h>
#include <mach/lge_mem_misc.h>
#ifdef CONFIG_LGE_LTE_ERS
#include <asm/setup.h> // for meminfo
#include <linux/slab.h> // for kzalloc
#include <linux/fs.h>
#include <asm/stat.h>
#endif

#include "../proc_comm.h"

/* neo.kang@lge.com	10.12.29
 * 0013365 : [kernel] add uevent for LTE ers */
#ifdef CONFIG_LGE_LTE_ERS
#include <linux/switch.h>
#endif

#define ERS_DRIVER_NAME "ers-kernel"

/* neo.kang@lge.com 11.01.05	
 * 0013574 : bug fix lte crash log and modified code */
#ifdef CONFIG_LGE_LTE_ERS
#define LTE_WDT_RESET	0x0101
#define LTE_WDT_RESET_MAGIC 0x0045544c
#define LTE_CRASH_RESET	0x0102
#define LTE_CRASH_RESET_MAGIC	0x0045544d
#endif

static atomic_t enable = ATOMIC_INIT(1);
static atomic_t report_num = ATOMIC_INIT(1);

struct ram_console_buffer {
	uint32_t    sig;
	uint32_t    start;
	uint32_t    size;
	uint8_t     data[0];
};

static struct ram_console_buffer *ram_console_buffer = 0;
extern struct ram_console_buffer *get_ram_console_buffer(void);

extern char *ram_console_old_log;
extern size_t ram_console_old_log_size;

#ifdef CONFIG_LGE_LTE_USB_SWITCH_REQUEST
extern void lte_usb_switch_request(int lte_usb_enabled);
#endif

/* neo.kang@lge.com	10.12.29. S
 * 0013365 : [kernel] add uevent for LTE ers */
#ifdef CONFIG_LGE_LTE_ERS
void msleep(unsigned int msecs);

static int ustate = 0;
static ssize_t print_switch_name(struct switch_dev *sdev, char *buf);
static ssize_t print_switch_state(struct switch_dev *sdev, char *buf);
static char lte_dump_file_name[50];

wait_queue_head_t lte_wait_q;
atomic_t lte_log_handled;
static struct switch_dev sdev_lte = {
	.name = "LTE_LOG",
	.print_name = print_switch_name,
	.print_state = print_switch_state,
};
#endif
/* neo.kang@lge.com	10.12.29. E */
#ifdef CONFIG_LGE_LTE_ERS
#include <linux/io.h>

/* neo.kang@lge.com	10.12.15.
 * 0012867 : add the hidden reset */

/* neo.kang@lge.com	10.12.13. S
 * 0012347 : [kernel] add the LTE ers
 * resize misc area to get the LTE log buffer.
 * the size should be lower than 128k because of LTE log buffer
*/
/* neo.kang@lge.com	10.12.13. E */
static struct misc_buffer * ram_misc_buffer = NULL;
#endif


/* RAM CONSOLE 1MB LAYOUT
 * |--------------------	|
 * | RESERVED  640KB   	|
 * |--------------------	|
 * | LTE logo     124KB   	|
 * |--------------------	|
 * | misc area       4KB		|
 * |--------------------	|
 * | panic_logo   128KB		|
 * |--------------------	|
 * | ram console 128KB		|
 * |--------------------	|
 */

#if defined(CONFIG_LGE_LTE_ERS)
/* neo.kang@lge.com	10.12.15.
 * 0012867 : add the hidden reset */

void *ram_lte_log_buf;
static atomic_t lte_enable = ATOMIC_INIT(1);
static char lte_dump_save_flag = 0;

void lte_panic_report(unsigned int magic_code)
{
	mm_segment_t oldfs;
	struct file *phMscd_Filp = NULL;

#if 0
	value = atomic_read(&lte_enable);

	if( value == 0 )
		return;
#endif

	oldfs = get_fs();
	set_fs(get_ds());

	if(magic_code == LTE_WDT_RESET_MAGIC)
	{
		phMscd_Filp = filp_open("/data/lte_ers_panic", O_WRONLY |O_CREAT | O_TRUNC |O_SYNC | O_LARGEFILE, 0777);
		printk(KERN_INFO "%s, create /data/lte_ers_panic\n", __func__);
	}
	else if(magic_code == LTE_CRASH_RESET_MAGIC)
	{
		phMscd_Filp = filp_open("/data/lte_ers_panic_prev", O_WRONLY |O_CREAT | O_TRUNC |O_SYNC | O_LARGEFILE, 0777);
		printk(KERN_INFO "%s, create /data/lte_ers_panic_prev\n", __func__);
	}
	phMscd_Filp->f_pos = 0;
	
	if(IS_ERR(phMscd_Filp)){
		printk(KERN_ERR "open failed\n");
	}

	printk("### __func__ : %08x\n", (unsigned int)ram_lte_log_buf);
	
	phMscd_Filp->f_op->write(phMscd_Filp, ram_lte_log_buf, LTE_LOG_SIZE, &phMscd_Filp->f_pos);
	filp_close(phMscd_Filp,NULL);
	
	set_fs(oldfs);
}
EXPORT_SYMBOL(lte_panic_report);

static ssize_t lte_ers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value = atomic_read(&lte_enable);
	if (value == 0) {
		printk("The LTE ers of kernel was disabled.\n");
	} else {
		printk("The LTE ers of kernel was enabled.\n");
	}
	return value;
}

static ssize_t lte_ers_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	atomic_set(&lte_enable, value);

	return size;
}
static DEVICE_ATTR(lte_ers, 0664 , lte_ers_show, lte_ers_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
//static DEVICE_ATTR(lte_ers, S_IRUGO | S_IWUGO, lte_ers_show, lte_ers_store);

static ssize_t lte_ers_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct misc_buffer *temp;

	temp = (struct misc_buffer *)ram_misc_buffer;

	printk("### lte_ers_panic_store \n");
	
	if(( temp->lte_magic == LTE_WDT_RESET_MAGIC ) || ( temp->lte_magic == LTE_CRASH_RESET_MAGIC )) {
		lte_panic_report(temp->lte_magic);
		temp->lte_magic = 0;
	} else {
		printk("\n### error : no valid lte log !!\n");
	}
	return size;
}
static DEVICE_ATTR(lte_ers_panic, 0664 , NULL, lte_ers_panic_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
//static DEVICE_ATTR(lte_ers_panic, S_IRUGO | S_IWUGO, NULL, lte_ers_panic_store);

int lte_crash_log_in(void *buffer, unsigned int size, unsigned int reserved)
{
	struct misc_buffer *temp;

	temp = (struct misc_buffer *)ram_misc_buffer;

	if(reserved == LTE_WDT_RESET)
		temp->lte_magic = LTE_WDT_RESET_MAGIC;
	else
		temp->lte_magic = LTE_CRASH_RESET_MAGIC;
	
	if( size > LTE_LOG_SIZE ) {
		printk("%s : buffer size %d is bigger than %d\n",
				__func__, size, LTE_LOG_SIZE);
		return -1;
	}

	memcpy(ram_lte_log_buf, buffer, size);
	printk("%08x, %08x \n", (unsigned int)ram_lte_log_buf, (unsigned int)buffer);
	
	/* save log to the buffer */
	printk("### generate kernel panic for LTE\n");

	//[START] 2011.06.16 previously crash log was saved after boot-up in the init.rc. change to save as soon as it happens as required.
	printk("### lte_ers_panic_store \n");
	lte_panic_report(temp->lte_magic);
	temp->lte_magic = 0;
	//[END] 2011.06.16

// This feature will be defined in case user variant
// Since lte_log_handled will not be set in the user-signed image ASSERT and WDT_RESET
// will be handled same.
#ifdef CONFIG_LGE_FEATURE_RELEASE
	BUG();
#else
	// 2011.06.16 panic reset only in case WDT
	if(reserved == LTE_WDT_RESET)
		BUG();
#endif

	return 0;
}

/* neo.kang@lge.com	10.12.29. S
 * 0013365 : [kernel] add uevent for LTE ers */
int lte_crash_log(void *buffer, unsigned int size, unsigned int reserved)
{
	struct misc_buffer *temp;
	
	printk("%s: , reserved : %s\n", __func__, reserved==LTE_WDT_RESET?"LTE_WDT":"LTE_CRASH");

	//[START] 2011.06.16 previously crash log was saved after boot-up in the init.rc. change to save as soon as it happens as required.
	lte_crash_log_in(buffer, size, reserved);

// This feature will be defined in case user variant
// Since lte_log_handled will not be set in the user-signed image ASSERT and WDT_RESET
// will be handled same.
#ifdef CONFIG_LGE_FEATURE_RELEASE
	return 0;
#else
	/* if watchDogReset is occured, jsut reset */
	if( reserved == LTE_WDT_RESET) {
		//lte_crash_log_in(buffer, size, reserved);
		return 0;
	}
#endif
	//[END] 2011.06.16

/* neo.kang@lge.com 11.01.05 E */

	/*
	if(ustate == 0) {
		ustate = 1;
		switch_set_state(&sdev_lte, ustate);
	} else {
		ustate = 0;
		switch_set_state(&sdev_lte, ustate);
	}
	*/
	ustate = S_L2K_RAM_DUMP_ATTR_PREV;
	switch_set_state(&sdev_lte, ustate);
	
	printk("%s: ustate = %d\n", __func__, ustate);

	// to recieve the crash dump, do not wait user selection
	/*
	wait_event_interruptible(lte_wait_q, atomic_read(&lte_log_handled));

	printk("%s: got lte_log_handled event interrupt\n", __func__, ustate);

	[START] 2011.06.16 dump the crash log
	if (atomic_read(&lte_log_handled))
		
	if (atomic_read(&lte_log_handled))
		lte_crash_log_in(buffer, size, reserved);
	[END] 2011.06.16
	*/
	return 0;
}
EXPORT_SYMBOL(lte_crash_log);

int lte_crash_log_dump(void *buffer, unsigned int size, unsigned int file_num, unsigned int file_seq, unsigned int file_attr)
{
	mm_segment_t oldfs;
	//struct statfs st_fs;
	struct stat f_st;
	static struct file *phMscd_Filp = NULL;
	//char lte_dump_file_name[50];
	static unsigned int file_position = 0;
	static unsigned int prev_file_seq = 0;
	ssize_t write_size = 0;

	oldfs = get_fs();
	set_fs(get_ds());

	memset(lte_dump_file_name, 0, sizeof(lte_dump_file_name));
	sprintf(lte_dump_file_name, "/data/lte_total_dump_%d", file_num);

	// new file, delete the file if exist
	if(file_attr & S_L2K_RAM_DUMP_ATTR_START)
	{
		if (sys_statfs(lte_dump_file_name, &f_st) < 0)
		{
			printk(KERN_INFO "file exists, unlink and re-create %s\n", lte_dump_file_name);
			sys_unlink(lte_dump_file_name);
		}
		else
		{
			printk(KERN_INFO "file does not exist, create %s\n", lte_dump_file_name);
		}
	
		//phMscd_Filp = filp_open(lte_dump_file_name, O_WRONLY | O_CREAT |O_SYNC |O_LARGEFILE, 0777);
		phMscd_Filp = filp_open(lte_dump_file_name, O_WRONLY | O_CREAT |O_LARGEFILE, 0777);

		file_position = 0;
		prev_file_seq = 1;

		// switch state to notifiy crash dump start
		ustate = S_L2K_RAM_DUMP_ATTR_START;
		switch_set_state(&sdev_lte, ustate);
	}
	else
	{
		//phMscd_Filp = filp_open(lte_dump_file_name, O_WRONLY |O_SYNC |O_LARGEFILE, 0);

		if(IS_ERR(phMscd_Filp))
		{
			// invalid packet type error handling - if the attribute did not started with 0x11, this case will be met
			printk(KERN_ERR "%s, open failed, ignore error and create the file\n", lte_dump_file_name);
			//phMscd_Filp = filp_open(lte_dump_file_name, O_WRONLY | O_CREAT |O_SYNC |O_LARGEFILE, 0777);
			phMscd_Filp = filp_open(lte_dump_file_name, O_WRONLY | O_CREAT |O_LARGEFILE, 0777);
			file_position = 0;
			prev_file_seq = 1;

			// switch state to notifiy crash dump start
			ustate = S_L2K_RAM_DUMP_ATTR_START;
			switch_set_state(&sdev_lte, ustate);
		}
		else
		{
			if((prev_file_seq+1) < file_seq)
			{
				printk(KERN_ERR "invalid file_seq, prev : %d, current : %d\n", prev_file_seq, file_seq);
				file_position += (file_seq - (prev_file_seq+1))*size;

				// reposition the file location in case recieved position is greater than expectation
				if(lte_dump_save_flag == 0)
				{
					phMscd_Filp->f_pos = file_position;
					phMscd_Filp->f_op->llseek(phMscd_Filp, &phMscd_Filp->f_pos, SEEK_SET);			
				}
				
			}
			prev_file_seq = file_seq;
		}
	}

	if(IS_ERR(phMscd_Filp)) {
		printk(KERN_ERR "%s, open failed\n", lte_dump_file_name);
		set_fs(oldfs);
		return -1;
	}
	
	//phMscd_Filp->f_pos = file_position;
	//printk(KERN_INFO "f_pos : 0x%X, file_position : 0x%X \n", phMscd_Filp->f_pos, file_position);

	// if user selects not to dump like switching to LLDM, do not save the logs.
	if(lte_dump_save_flag == 0)
	{
		//phMscd_Filp->f_op->llseek(phMscd_Filp, &phMscd_Filp->f_pos, SEEK_SET);	

		write_size = phMscd_Filp->f_op->write(phMscd_Filp, buffer, size, &phMscd_Filp->f_pos);
		if(write_size <= 0)
		{
			printk(KERN_ERR "%s write failed, write size : %d, expected size : %d\n", lte_dump_file_name, write_size, size);
		}
	}

	file_position+= size;
	//printk(KERN_INFO "file position : 0x%X, size : 0x%X\n", file_position, size);

	// switch state to notifiy the end of crash dump
	if((file_num == 0x0)&&(file_attr &S_L2K_RAM_DUMP_ATTR_END))
	{
		msleep(500); // wait some time to save all the logs and files
	
		ustate = S_L2K_RAM_DUMP_ATTR_END;
		switch_set_state(&sdev_lte, ustate);

		phMscd_Filp->f_op->fsync(phMscd_Filp, 0);
		filp_close(phMscd_Filp,NULL);
		phMscd_Filp = NULL;

		/*
		wait_event_interruptible(lte_wait_q, atomic_read(&lte_log_handled));

		printk("%s: got lte_log_handled event interrupt\n", __func__, ustate);
		atomic_read(&lte_log_handled);

		// panic reset if selected to reset
		if(lte_log_handled.counter == 1)
			BUG();
		*/
	}

	set_fs(oldfs);
	
}

EXPORT_SYMBOL(lte_crash_log_dump);

static ssize_t lte_cmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value = 0;
	sscanf(buf, "%d\n", &value);
	printk("[MC] %s: value = %d\n", __func__, value);

	/*
	if( value == 1)
		atomic_set(&lte_log_handled, 1);
	else
		atomic_set(&lte_log_handled, 0);
	*/

	if( value == 2)
	{
		printk("%s: value = %d, LTE DUMP saving disabled!\n", __func__, value);	
		lte_dump_save_flag = 1;
// request LTE usb mode if user selects NO
#ifdef CONFIG_LGE_LTE_USB_SWITCH_REQUEST
		lte_usb_switch_request(1);
#endif
	}
	
	atomic_set(&lte_log_handled, value);

	wake_up(&lte_wait_q);

	// activate kernel panic reset
	if( value == 1)
		BUG();
	
	return size;
}
static DEVICE_ATTR(lte_cmd, 0664 , NULL, lte_cmd_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
//static DEVICE_ATTR(lte_cmd, S_IRUGO | S_IWUGO, NULL, lte_cmd_store);
/* neo.kang@lge.com	10.12.29. E */

static ssize_t gen_lte_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	void *test_buf = NULL;

	test_buf = kzalloc(4096, GFP_KERNEL);
	printk("### generate kernel panic for LTE (sys file)\n");
	lte_crash_log(test_buf, 4096, 0);

	return size;
}
static DEVICE_ATTR(gen_lte_panic, 0664 , NULL, gen_lte_panic_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
// static DEVICE_ATTR(gen_lte_panic, S_IRUGO | S_IWUGO, NULL, gen_lte_panic_store);

/* neo.kang@lge.com 10.12.29. S
 * 0013237 : add the sys file of hidden reset */
#if defined(CONFIG_LGE_HIDDEN_RESET_PATCH)
static ssize_t is_hidden_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", on_hidden_reset);
}

static DEVICE_ATTR(is_hreset, 0664 , is_hidden_show, NULL); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
// static DEVICE_ATTR(is_hreset, S_IRUGO|S_IWUGO, is_hidden_show, NULL);
#endif
/* neo.kang@lge.com 10.12.29. E */
#endif // CONFIG_LGE_LTE_ERS
/* neo.kang@lge.com	10.12.08. E */

int get_lock_dependencies_report_start(uint32_t start, uint32_t size, uint8_t *data)
{
	int report_start;
	int i;	

	report_start = -1;

	for (i = start - 1; i > -1; --i) {
		if (data[i] == '-') {
			if (!strncmp(&data[i - 25], "-lock_dependencies_report-", 26)) {
				report_start = i + 1;
				break;
			}
		}
	}

	if (i > -1) {
		return report_start;		
	}

	for (i = size - 1; i >= start; --i) {
		if (data[i] == '-') {
			if (!strncmp(&data[i - 25], "-lock_dependencies_report-", 26)) {
				report_start = i + 1;
				break;
			}
		}
	}

	if (i < start) {
		return -1;
	}

	return report_start;
}

void lock_dependencies_report(void)
{
	char filename[NAME_MAX];
	int fd;
	int value;
	mm_segment_t oldfs;

	uint32_t start;
	uint32_t size;
	uint8_t *data;
	int report_start;

	value = atomic_read(&enable);
	if (value == 0) {
		return;
	}

	ram_console_buffer = get_ram_console_buffer();
	if (!ram_console_buffer) {
		return;
	}
	
	start = ram_console_buffer->start; 
	size = ram_console_buffer->size;
	data = ram_console_buffer->data;
	
	report_start = get_lock_dependencies_report_start(start, size, data);
	if(report_start == -1) {
		return;
	}

	sprintf(filename, "/data/ers_lock_dependencies_%d", atomic_read(&report_num));

	oldfs = get_fs();
	set_fs(get_ds());

	fd = sys_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0766);
	if (fd < 0) {
		return;
	}
	atomic_inc(&report_num);

	if (report_start < start) {
		sys_write(fd, &data[report_start], start - report_start);
	} else {
		sys_write(fd, &data[report_start], size - report_start);
		sys_write(fd, &data[0], start);
	}
	
	sys_close(fd);
	
	set_fs(oldfs);
}

EXPORT_SYMBOL(lock_dependencies_report);


static int get_panic_report_start(uint32_t start, uint32_t size, uint8_t *data)
{
	int report_start;
	int i;	

	report_start = -1;

	for (i = start - 1; i > -1; --i) {
		if (data[i] == 'C') {
			if (!strncmp(&data[i], "CPU:", 4)) {
				report_start = i;
				break;
			}
		}
	}

	if (i > -1) {
		return report_start;		
	}

	for (i = size - 1; i >= start; --i) {
		if (data[i] == 'C') {
			if (!strncmp(&data[i], "CPU:", 4)) {
				report_start = i;
				break;
			}
		}
	}

	if (i < start) {
		return -1;
	}

	return report_start;
}

static int panic_report(struct notifier_block *this, 
		unsigned long event, void *ptr)
{
	int fd;
	int value;
	mm_segment_t oldfs;

	uint32_t start;
	uint32_t size;
	uint8_t *data;
	int report_start;

	value = atomic_read(&enable);
	if (value == 0) {
		return NOTIFY_DONE;
	}

	ram_console_buffer = get_ram_console_buffer();
	if (!ram_console_buffer) {
		return NOTIFY_DONE;
	}
	
	start = ram_console_buffer->start; 
	size = ram_console_buffer->size;
	data = ram_console_buffer->data;
	
	report_start = get_panic_report_start(start, size, data);
	if(report_start == -1) {
		return NOTIFY_DONE;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	fd = sys_open("/data/ers_panic", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd < 0) {
		return NOTIFY_DONE;
	}

	if (report_start < start) {
		sys_write(fd, &data[report_start], start - report_start);
	} else {
		sys_write(fd, &data[report_start], size - report_start);
		sys_write(fd, &data[0], start);
	}
	
	sys_close(fd);
	
	set_fs(oldfs);
		
	return NOTIFY_DONE;
}
EXPORT_SYMBOL(panic_report);

static ssize_t ers_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value = atomic_read(&enable);
	if (value == 0) {
		printk("The ers of kernel was disabled.\n");
	} else {
		printk("The ers of kernel was enabled.\n");
	}
	return value;
}

static ssize_t ers_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	printk("%s\n", __func__);
	sscanf(buf, "%d", &value);
	atomic_set(&enable, value);

	return size;
}

static DEVICE_ATTR(ers, 0664 , ers_show, ers_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
//static DEVICE_ATTR(ers, S_IRUGO | S_IWUGO, ers_show, ers_store);

void ers_store_log(void)
{
	int fd_dump;
	int value;
	mm_segment_t oldfs;

	uint32_t start;
	uint32_t size;
	uint8_t *data;
	int report_start;
	value = atomic_read(&enable);
	if (value == 0) {
		return;
	}

	ram_console_buffer = get_ram_console_buffer();
	if (!ram_console_buffer) {
		return;
	}

	if((ram_console_old_log_size == 0) || (ram_console_old_log == NULL))
	{
		printk(KERN_ERR "%s, no valid old log, size : %d\n", __func__, ram_console_old_log_size);
		return;
	}
	
	start = ram_console_buffer->start; 
	size = ram_console_old_log_size;
//	data = ram_console_buffer->data;
	data = ram_console_old_log;

	report_start = get_panic_report_start(start, size, data);
	if(report_start == -1) {
		printk(KERN_ERR "%s, invalid panic report\n", __func__);
		return;
	}

	oldfs = get_fs();
	set_fs(get_ds());
/* kwnagdo.yi@lge.com 10.11.10 S 
 *0010729: change location of kernel panic log file
 * change location from /data/kernel_panic_dump to /data/ers_panic
 */
	fd_dump = sys_open("/data/ers_panic", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	if (fd_dump < 0) {
		return;
	}

	sys_write(fd_dump, &data[0], size);
	sys_close(fd_dump);
	set_fs(oldfs);
}

static ssize_t ers_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	//BUG();

	if(ram_misc_buffer == NULL)
	{
		printk(KERN_ERR "%s, NULL ram_misc_buffer\n", __func__);
		return 0;
	}

	printk(KERN_INFO "%s, ram_misc_buffer : 0x%x\n", __func__, ram_misc_buffer);

	if(ram_misc_buffer->magic_key == PANIC_MAGIC_KEY)
	{
		printk(KERN_INFO "%s, panic had been generated, save log and reset the information\n", __func__);
		ers_store_log();
		ram_misc_buffer->magic_key = 0;
	}
	else
	{
		printk("\n### %s : no valid log !!\n", __func__);
	}

	return size;
}

static DEVICE_ATTR(ers_panic, 0664 , 0, ers_panic_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
// static DEVICE_ATTR(ers_panic, S_IRUGO | S_IWUGO, 0, ers_panic_store);

static ssize_t gen_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	printk("### generate kernel panic \n");

	BUG();

	return size;
}
static DEVICE_ATTR(gen_panic, 0664 , 0, gen_panic_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
//static DEVICE_ATTR(gen_panic, S_IRUGO | S_IWUGO, 0, gen_panic_store);

static ssize_t gen_modem_panic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	printk("### generate modem panic \n");
	msm_proc_comm(PCOM_OEM_MODEM_PANIC_CMD, 0, 0); 

	return size;
}
static DEVICE_ATTR(gen_modem_panic, 0664 , 0, gen_modem_panic_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
// static DEVICE_ATTR(gen_modem_panic, S_IRUGO | S_IWUGO, 0, gen_modem_panic_store);

static ssize_t set_modem_auto_action_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val = simple_strtoul(buf, NULL, 10);

	printk("### 0 : download, 1 : reset, 2 : no_action\n");
	printk("### set modem err auto action type : %lu\n", val);
	msm_proc_comm(PCOM_OEM_SET_AUTO_ACTION_TYPE_CMD, &val, 0); 

	return size;
}
static DEVICE_ATTR(set_modem_auto_action, 0664 , 0, set_modem_auto_action_store); //#2011.07.28 - CTS FAIL android.permission.cts.FileSystemPermissionTest#testAllBlockDevicesAreNotReadableWritable
//static DEVICE_ATTR(set_modem_auto_action, S_IRUGO | S_IWUGO, 0, set_modem_auto_action_store);

static struct notifier_block ers_block = {
	    .notifier_call  = panic_report,
};

#ifdef CONFIG_LGE_LTE_ERS
static ssize_t print_switch_name(struct switch_dev *sdev, char *buf)
{
	//return sprintf(buf, "%s\n", "LTE_LOG");
	return sprintf(buf, "%s\n", lte_dump_file_name);
}

static ssize_t print_switch_state(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%d\n", sdev->state);
}
#endif
static int __devinit ers_probe(struct platform_device *pdev)
{
	int ret;

#ifdef CONFIG_LGE_LTE_ERS
	struct membank *bank = &meminfo.bank[0];
#ifdef CONFIG_MACH_LGE_BRYCE_MS910
	struct membank *bank1 = &meminfo.bank[1];
#endif
#endif

	// panic will be handled in lge_handle_panic.c and use this as ers store after re-boot
	//atomic_notifier_chain_register(&panic_notifier_list, &ers_block);

	ret = device_create_file(&pdev->dev, &dev_attr_ers);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}
	
	ret = device_create_file(&pdev->dev, &dev_attr_ers_panic);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_gen_panic);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}
	
	ret = device_create_file(&pdev->dev, &dev_attr_gen_modem_panic);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}
#ifdef CONFIG_LGE_LTE_ERS
	ret = device_create_file(&pdev->dev, &dev_attr_lte_ers);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_lte_ers_panic);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}

	ret = device_create_file(&pdev->dev, &dev_attr_gen_lte_panic);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}
/* neo.kang@lge.com	10.12.29. S
 * 0013365 : [kernel] add uevent for LTE ers */
	ret = device_create_file(&pdev->dev, &dev_attr_lte_cmd);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}
	ret = switch_dev_register(&sdev_lte);

	if( ret < 0 )
	{
		printk(KERN_ERR "%s, switch_dev_register failed\n", sdev_lte.name);
		switch_dev_unregister(&sdev_lte);
	}
		
/* neo.kang@lge.com	10.12.29. E */
#endif // CONFIG_LGE_LTE_ERS

	ret = device_create_file(&pdev->dev, &dev_attr_set_modem_auto_action);
	if (ret < 0) {
		printk("device_create_file error!\n");
		return ret;
	}

#ifdef CONFIG_LGE_LTE_ERS
#ifdef CONFIG_MACH_LGE_BRYCE_MS910
	ram_misc_buffer = ioremap(bank1->start + bank1->size + LGE_RAM_CONSOLE_SIZE, LGE_RAM_CONSOLE_MISC_SIZE);
#else
	ram_misc_buffer = ioremap(bank->start + bank->size + LGE_RAM_CONSOLE_SIZE, LGE_RAM_CONSOLE_MISC_SIZE);
#endif

	if(ram_misc_buffer== NULL)
		printk("ram_misc : failed to map memory \n");
	else 
		printk("\n### ram_misc : ok map memory at %08x!!, size %zx !!\n", (unsigned int)ram_misc_buffer, LGE_RAM_CONSOLE_MISC_SIZE);

#ifdef CONFIG_MACH_LGE_BRYCE_MS910
	ram_lte_log_buf = ioremap(bank1->start + bank1->size + LGE_RAM_CONSOLE_SIZE + LGE_RAM_CONSOLE_MISC_SIZE, LTE_LOG_SIZE);
#else
	ram_lte_log_buf = ioremap(bank->start + bank->size + LGE_RAM_CONSOLE_SIZE + LGE_RAM_CONSOLE_MISC_SIZE, LTE_LOG_SIZE);
#endif

	if( ram_lte_log_buf == NULL )
		printk("ram_lte_log_buf : failed to map memory \n");
	else 
		printk("\n### lte_log_buf : ok map memory at %08x!!, size %zx !! \n", 
				(unsigned int)ram_lte_log_buf, LTE_LOG_SIZE);

/* neo.kang@lge.com	10.12.29. S
 * 0013365 : [kernel] add uevent for LTE ers */
	atomic_set(&lte_log_handled, 0);
	init_waitqueue_head(&lte_wait_q);
/* neo.kang@lge.com	10.12.29. E */
#endif

	return ret;
}

static int __devexit ers_remove(struct platform_device *pdev)
{	
	device_remove_file(&pdev->dev, &dev_attr_ers);
	device_remove_file(&pdev->dev, &dev_attr_ers_panic);

	return 0;
}

static struct platform_driver ers_driver = {
	.probe = ers_probe,
	.remove = __devexit_p(ers_remove),
	.driver = {
		.name = ERS_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ers_init(void)
{
	return platform_driver_register(&ers_driver);
}
module_init(ers_init);

static void __exit ers_exit(void)
{
	platform_driver_unregister(&ers_driver);
}
module_exit(ers_exit);

MODULE_DESCRIPTION("Exception Reporting System Driver");
MODULE_AUTHOR("Jun-Yeong Han <junyeong.han@lge.com>");
MODULE_LICENSE("GPL");
