#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/errno.h>

# define KERNEL_SECTOR_SIZE 512

static int iMajor_Number = 0;
module_param(iMajor_Number, int, 0);



static int iSector_size = 512;
module_param(iSector_size, int, 0);

static int  iNumber_of_sectors = 16024;
module_param(iNumber_of_sectors, int, 0);

static struct class *cStorage_class = NULL;


// Module bulk storage device data
static struct dSbd_device
{
	unsigned long lSize;
	spinlock_t sLock;
	u8  *uData;
	struct gendisk *gDisk; //Kernel representation of a disk
}blk_device;

dev_t dMy_blk_dev;

//We need to have a request queue
static struct request_queue *blk_dev_queue;

//static function to handle the queue request
static void blk_sbd_request( struct request_queue *q);




static int blk_sbd_open(struct block_device *bd, fmode_t pos)
{


	return 0;
}

//Device operations struct
static struct block_device_operations blk_dev_fops = {
	.owner = THIS_MODULE,
	.open = blk_sbd_open,
};

static __init int blk_sbd_init(void)
{

	struct device *blk_dev = NULL;

	//1>>>>>>>>>>setup the ramdisk 
	blk_device.lSize = iSector_size * iNumber_of_sectors;

	spin_lock_init(&blk_device.sLock);
	//if memory crosses 128Kb then kmalloc can't process the request.
	//512*16024 = 8204K

	blk_device.uData = vmalloc(blk_device.lSize)
	if(blk_device.uData == NULL) return -ENOMEM;

	memset(blk_device.uData, 0, blk_device.lSize);

	//2>>>>>>>>>>>>>>>>>>> Register with the generic Block layer

	// first get a request queue and assign a request routine.
	
	// Get a request queue for our device - initialize the queue.
	//Allocate the request queue
	blk_dev_queue = blk_init_queue(blk_sbd_request, &blk_device.sLock);	
	if(blk_dev_queue = NULL) goto out;

	//assign the sector size to the request queue
	blk_queue_logical_block_size(blk_dev_queue,iSector_size);

	//3>>>>>>>>>>> Register with VFS

	//acquire major and minor numbers - on the block device.

	// if given iMajor_Number =0 then it dynamically allocates the major number.
	iMajor_Number = register_blkdev(iMajor_Number, "blk_sbd0");
	if(iMajor_Number <=0) printk(KERN_WARNING "Unable to get the Major number\n") goto out;

	printk("success for major number \n");

	//with MKDEV we are creating an instace of dev_t .
	dMy_blk_dev = MKDEV(iMajor_Number, 0);

	// we need to register the device with device tree for udev service 

	cStorage_class = class_create(THIS_MODULE, "blk_dev_cls");
	if(IS_ERR(cStorage_class)) printk(KERN_ERR "unable to register the device class \n");

	blk_dev = device_crete(cStorage_class, NULL, dMy_blk_dev, NULL, "blk_sbd0");

	//4>>>>>>>>>>>>>>>>>>>>>>>>> same as char driver , VFS identifies a block device as gendisk.

	blk_device.gDisk = alloc_disk(1); //pool allocator for gendisk. 1 is the number of partitions. 
	//1 means no partition
	if(!blk_device.gDisk) goto unregister;

	blk_device.gDisk->major = 	iMajor_Number;
	blk_device.gDisk->first_minor = 0;
	blk_device.gDisk->fops = &blk_dev_fops; //synchonous operations
	blk_device.gDisk->private_data = &blk_device;
	strcpy(blk_device.gDisk->disk_name, "blk_sbd0");
	blk_device.gDisk->queue = blk_dev_queue;

	set_capacity(blk_device.gDisk, iNumber_of_sectors * (iSector_size / KERNEL_SECTOR_SIZE));

	add_disk(blk_device.gDisk);

	return 0;

unregister:
	printk()


out:


}

static __exit void  blk_sbd_exit(void)
{
	 printk(KERN_INFO "\n Driver unregistered \n");
}
module_init(blk_sbd_init);
module_exit(blk_sbd_exit);

MODULE_LICENSE("GPL");


