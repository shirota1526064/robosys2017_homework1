#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/timer.h>
#include <asm/uaccess.h>

MODULE_AUTHOR("s1526064");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

#define LED_GPIO 4
#define MY_TIMER_SECS 2 

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

struct timer_list mytimer;

static void mytimer_func(unsigned long arg){
	printk(KERN_ALERT "%d secs passed.\n",MY_TIMER_SECS);
	gpio_base[10] = 1 << LED_GPIO;
}

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){
	char c;
	if(copy_from_user(&c,buf,sizeof(char))){
		return -EFAULT;
	}
	if(c == '0'){
		gpio_base[10] = 1 << LED_GPIO;
	}
	else if(c == '1'){
		gpio_base[7] = 1 << LED_GPIO;
		init_timer(&mytimer);
		mytimer.expires = jiffies + MY_TIMER_SECS * HZ;
		mytimer.data = 0;
		mytimer.function = mytimer_func;
		add_timer(&mytimer);
	}
	printk(KERN_INFO "led_write is called\n");
	return 1;
}

static struct file_operations led_fops = {
        .owner = THIS_MODULE,
        .write = led_write
};
static int __init init_mod(void)
{
	int retval;
	gpio_base = ioremap_nocache(0x3f200000, 0xA0);
	
	const u32 led = LED_GPIO;
	const u32 index = led/10;//GPFSEL2
	const u32 shift = (led%10)*3;//15bit
	const u32 mask = ~(0x7 << shift);//11111111111111000111111111111111
	gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);//001: output flag
	retval =  alloc_chrdev_region(&dev, 0, 1, "myled");
	
	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));
	
	cdev_init(&cdv, &led_fops);
	retval = cdev_add(&cdv, dev, 1);	
	
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));
		return retval;
	}
	cls = class_create(THIS_MODULE,"myled");
        if(IS_ERR(cls)){
                printk(KERN_ERR "class_create failed.");
                return PTR_ERR(cls);
        }
	return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
	del_timer(&mytimer);
}

module_init(init_mod);
module_exit(cleanup_mod);
