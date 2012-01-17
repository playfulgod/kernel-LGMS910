#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board-lge.h>

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       
#include <linux/i2c.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#define   RDWR_DEV_NAME            "rdwrdev"
#define   RDWR_DEV_MAJOR            236


int param_client=0;
int param_page=-1;

module_param_named(client,param_client,int,S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(client, "i2c client addr");

module_param_named(page,param_page,int,S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(page, "i2c page addr");

int rdwr_open (struct inode *inode, struct file *filp)
{
	printk("[jaeseong.gim]: %s\n",__func__);
	return 0;
}

ssize_t rdwr_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	printk("[jaeseong.gim]: %s\n",__func__);
	return 0;

}

ssize_t rdwr_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	printk("[jaeseong.gim]: %s\n",__func__);
	return 0;
}

ssize_t rdwr_llseek(struct file *filp, loff_t f_pos, int newpos)
{
	printk("[jaeseong.gim]: %s\n",__func__);
	return 0;
}

int rdwr_release (struct inode *inode, struct file *filp)
{
	printk("[jaeseong.gim]: %s\n",__func__);
	return 0;
}

struct file_operations rdwr_fops =
{
	.owner    = THIS_MODULE,
	.read     = rdwr_read,     
	.write    = rdwr_write,    
	.open     = rdwr_open,     
	.llseek   = rdwr_llseek,
	.release  = rdwr_release,  
};

static int __init rdwr_init(void)
{
	u8 ret;
	int i,j;

	if (param_client == 0) {/* No global client pointer? */
		printk("[jaeseong.gim]:client is null\n");
		return -1;
	}

	if(param_page<0){
	
		for(j=0;j<0xa;j++){
			ret = i2c_smbus_write_byte_data((struct i2c_client*)param_client,0xff,(u8)j);
			printk("page:%x\n",j);
			for(i=0;i<0xff;i++){
				ret = i2c_smbus_read_byte_data((struct i2c_client*)param_client,i );
				printk("%2x:%2x",i,ret);
				if((i&0x7)==0x7) printk("\n");
				else printk(" ");
			}
			printk("\n\n");
		}
	
	
	
	
	}
	else{
		ret = i2c_smbus_write_byte_data((struct i2c_client*)param_client,0xff,(u8)param_page);
		for(i=0;i<0xff;i++){
			ret = i2c_smbus_read_byte_data((struct i2c_client*)param_client,i );
			printk("%2x:%2x",i,ret);
			if((i&0x7)==0x7) printk("\n");
			else printk(" ");
		}
		printk("\n\n");
	}
	return 0;
}

static void __exit rdwr_exit(void)
{
	return 0;
}

module_init(rdwr_init);
module_exit(rdwr_exit);

MODULE_LICENSE("GPL");
