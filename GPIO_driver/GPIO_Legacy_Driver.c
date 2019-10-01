/*
 * GPIO - Driver with the Character Driver
 *
 * This is module for the Raspberry Pi board Pin Configuration
 *
 * PIN NUM 24 corresponds to Physical PIN 18
 * PIN NUM 25 corresponds to Physical PIN 19
 * 
 * GPIO APIs used are the legacy functions.
 * 1. Request the GPIO pin
 * 2. Set the Direction of GPIO Pin In/Out
 * 3. Read / Write Operations will get and set the value to GPIO pin
 * 4. This module provides the Provision to export the GPIO pins to sysfs. 
 */ 

#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/current.h>
#include <linux/slab.h>
#include <linux/gpio.h>


#define DEV_NAME "gpio_dev"
#define DEV_COUNT  4
#define SUCCESS  0


#define GPIO_PIN_NUM_18  24
#define GPIO_PIN_NUM_19  25

struct cdev *gpio_cdev;
dev_t new_gpio_dev;
static struct class *gpio_class;
struct device dev;


static int gpio_dev_open(struct inode *inode,
			    struct file  *file)
{
	printk(KERN_INFO "open operation invoked \n");
	return SUCCESS;
}

static int gpio_dev_release(struct inode *inode,
		            struct file *file)
{
	return SUCCESS;
}

static ssize_t gpio_dev_read(struct file *file,
		            char *buf,
			    size_t lbuf,
			    loff_t *ppos)
{
	int gpio_value;
	int gpio_data=0;
	int ret;

	gpio_value = gpio_get_value(GPIO_PIN_NUM_18);
	if(gpio_value == 1) gpio_data|=0x01;

	gpio_value = gpio_get_value(GPIO_PIN_NUM_19);
	if(gpio_value == 1) gpio_data|=0x01;

	ret = copy_to_user(buf,(char*) &gpio_data,sizeof(gpio_data));
	return sizeof(int);

}


static ssize_t gpio_dev_write(struct file *file,
		            const char *buf,
			    size_t lbuf,
			    loff_t *ppos)
{
	int value_to_write=0; 
	int value;
	int ret;
        // this will fill value_to_write from the buf that recieved from user
	ret = copy_from_user(&value_to_write, 
	 		buf,  
	        	lbuf ); 

	// Sent value should be decoded for 3 LEDs. 
	value = (value_to_write & 0x01)? 0:1;
	gpio_set_value(GPIO_PIN_NUM_18, value);

	value = (value_to_write & 0x02)? 0:1;
	gpio_set_value(GPIO_PIN_NUM_19, value);
	
	return sizeof(int);
}

static struct file_operations gpio_dev_fops = {
	.owner = THIS_MODULE,
	.write = gpio_dev_write,
	.read = gpio_dev_read,
	.open = gpio_dev_open,
	.release = gpio_dev_release,
};

static __init int gpio_dev_init(void)
{
	int ret;

	if (alloc_chrdev_region (&new_gpio_dev, 0, DEV_COUNT, DEV_NAME) < 0) {
            printk (KERN_ERR "failed to reserve major/minor range\n");
            return -1;
        }

        if (!(gpio_cdev = cdev_alloc ())) {
            printk (KERN_ERR "cdev_alloc() failed\n");
            unregister_chrdev_region (new_gpio_dev, DEV_COUNT);
            return -1;
 	}
	cdev_init(gpio_cdev,&gpio_dev_fops);

	ret=cdev_add(gpio_cdev,new_gpio_dev,DEV_COUNT);
	if( ret < 0 ) {
		printk(KERN_INFO "Error registering device driver\n");
	        cdev_del (gpio_cdev);
                unregister_chrdev_region (new_gpio_dev, DEV_COUNT); 	
		return -1;
	}
	printk(KERN_INFO"\nDevice Registered: %s\n",DEV_NAME);
	printk (KERN_INFO "Major number = %d, Minor number = %d\n", MAJOR (new_gpio_dev),MINOR (new_gpio_dev));

	//Request the GPIO PINS
	ret = gpio_request_one(GPIO_PIN_NUM_18,
			       GPIOF_DIR_IN,
			       "RED_LED");
	if(ret<0) goto Error_Handle;

	ret = gpio_request_one(GPIO_PIN_NUM_19,
			       GPIOF_INIT_HIGH, /*configures as OUTPUT , initial set to HIGH*/
			       "GREEN_LED");
	if(ret<0) goto Error_Handle;

	//Export the GPIO pins to the sysfs
     ret = gpio_export(GPIO_PIN_NUM_18, true);
     if(ret<0) printk(KERN_INFO "Error exporting the gpio 18\n");

     ret = gpio_export(GPIO_PIN_NUM_19, true);
     if(ret<0) printk(KERN_INFO "Error exporting the gpio 19\n");


    gpio_class = class_create(THIS_MODULE, "gpio_rasp_cls");
	 device_create(gpio_class, NULL, new_gpio_dev, NULL,"%s","gpio_dev");

   return 0;

Error_Handle:
	printk(KERN_INFO "failed to request the GPIO pins\n");
	cdev_del (gpio_cdev);
        unregister_chrdev_region (new_gpio_dev, DEV_COUNT); 	
	return ret;
		
}

static __exit void  gpio_dev_exit(void)
{
	 gpio_free(GPIO_PIN_NUM_18);
	 gpio_free(GPIO_PIN_NUM_19);
	 gpio_unexport(GPIO_PIN_NUM_18);
	 gpio_unexport(GPIO_PIN_NUM_19);
     device_destroy(gpio_class, new_gpio_dev);
	 class_destroy(gpio_class);
	 cdev_del(gpio_cdev);
	 unregister_chrdev_region(new_gpio_dev,DEV_COUNT);
	 printk(KERN_INFO "\n Driver unregistered \n");
}
module_init(gpio_dev_init);
module_exit(gpio_dev_exit);

MODULE_LICENSE("GPL");
