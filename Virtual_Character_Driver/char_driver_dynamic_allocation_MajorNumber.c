
/*
 * Virtual Character Driver
 * 1. Dynamically allocates the Major and Minor Numbers
 * 2. Creates device entry in the devfs and sysfs
 * 3. Provided the read, write file operation from virtual device memory
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/device.h>
//#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/current.h>
#include <linux/slab.h>

#define CHAR_DEV_NAME "vdeg_cdrv"
#define MAX_LENGTH 4000
#define SUCCESS 0


/*char_device_buf is a device memory buffer on which the operations should take place
 * Since this is a virtual driver - we really dont have any device memory*/
static char *char_device_buf;
struct cdev *vdeg_cdev;
static struct class *vdeg_class;
dev_t mydev;

static int char_dev_open(struct inode *inode,
			    struct file  *file)
{
	static int counter = 0;
	counter++;
	printk(KERN_INFO "Number of times open invoked=%d \n",counter);
	printk(KERN_INFO "Major:Minor=%d:%d\n",imajor(inode),iminor(inode));
	printk(KERN_INFO "Process Id of the current process: %d\n",current->pid);
	printk(KERN_INFO "ref: %d\n",module_refcount(THIS_MODULE));
	return SUCCESS;
}

static int char_dev_release(struct inode *inode,
		            struct file *file)
{
	return SUCCESS;
}

/*Following steps should be done by a read call
1. Verify the read request
2. Read data from device : in this case read from the buffer
3. Process the data
4. Transfer to application buffer
*/
static ssize_t char_dev_read(struct file *file, /*file descriptor of the file*/
		         char *buf, /*output buffer */
			 size_t lbuf, /*number of bytes to read*/
			 loff_t *ppos)/*offset position with in the memory - extra argument passed by sys_read*/

{
/*We dont know the current position with in the device memory - so we need to verify if the position is with in max_length or not*/
	int i_maxbytes;//bytes that i can read  
	int i_bytes_to_read;//Number of bytes to read
	int i_nBytes;//NUmber of bytes actually read

	i_maxbytes = MAX_LENGTH - *ppos; //max length minus current position is the bytes that i can read
	//if i_maxbytes is greater than the lbuf - which is requested read bytes, we can service request.
	//else we need to truncate to the i_maxbytes
	
	if(i_maxbytes > lbuf) i_bytes_to_read = lbuf; //more bytes are there to read than requested - so good
	else i_bytes_to_read = i_maxbytes; //requested more bytes than the available bytes to read - so truncate.

	if(i_bytes_to_read == 0){
		printk(KERN_INFO "reached end of device \n");
		return -ENOSPC; /*causes read() to return EOF*/
	}

	/*copy_to_user - is pointer safe mechanism to copy from kernel buffer to user space buffer
	this function returns the number of bytes failed to read*/
  
	i_nBytes = i_bytes_to_read - copy_to_user(buf,  /*to*/
						  char_device_buf + *ppos, /*from : starting addres + current position*/ 
						  i_bytes_to_read);/*How many bytes*/
	//Classical driver has to update the offset position - so that next transfer will start from here.
	*ppos+=i_nBytes;
	return i_nBytes;

}
/*write is called when the user calls write on the device.
 * It writes in to 'file', the content of 'buf', starting from offset 'ppos'
 * up to 'lbuf' number of bytes
*/
static ssize_t char_dev_write(struct file *file,//file to write in to
		            const char *buf, //data buffer
			    size_t lbuf, //number of bytes
			    loff_t *ppos) //present position of pointer
{
	//Validate the write request
	int nbytes; /* Number of bytes written */
	int iBytes_to_do; //number of bytes to write
	int iMaxBytes; //max number of bytes that can be written

	iMaxBytes = MAX_LENGTH - *ppos;

	//if max bytes greater than requested the - OK
	//else truncate to max bytes
	if(iMaxBytes > lbuf) iBytes_to_do = lbuf;
	else iBytes_to_do = iMaxBytes;
	
	/*copy_from_user is safe mechanism to copy from user space to kernel space
	 this function returns the bytes failed to write*/
	  nbytes = lbuf - copy_from_user( char_device_buf + *ppos, /* to */
					buf, /* from */ 
					lbuf ); /* how many bytes */
	*ppos += nbytes;//update the offset position so that next transfer will start from here.
	printk(KERN_INFO "Rec'vd from App data %s of length %d \n",char_device_buf, nbytes);
	return nbytes;
}

static struct file_operations char_dev_fops = {
	.owner = THIS_MODULE,
	.open = char_dev_open,
	.read = char_dev_read,
	.write = char_dev_write,
	.release = char_dev_release,
};

static __init int char_dev_init(void)
{
	int ret,count=1;

	if (alloc_chrdev_region (&mydev, 0, count, CHAR_DEV_NAME) < 0) {
            printk (KERN_ERR "failed to reserve major/minor range\n");
            return -1;
    }

        if (!(vdeg_cdev = cdev_alloc ())) {
            printk (KERN_ERR "cdev_alloc() failed\n");
            unregister_chrdev_region (mydev, count);
            return -1;
 	}
	cdev_init(vdeg_cdev,&char_dev_fops);

	ret=cdev_add(vdeg_cdev,mydev,count);
	if( ret < 0 ) {
		printk(KERN_INFO "Error registering device driver\n");
	        cdev_del (vdeg_cdev);
                unregister_chrdev_region (mydev, count); 	
		return -1;
	}
	printk(KERN_INFO"\nDevice Registered: %s\n",CHAR_DEV_NAME);
	printk (KERN_INFO "Major number = %d, Minor number = %d\n", MAJOR (mydev),MINOR (mydev));

	vdeg_class = class_create(THIS_MODULE, "vdeg_virtual_cls");
	device_create(vdeg_class, NULL, mydev, NULL,"%s","vdeg_cdrv");

	char_device_buf =(char *)kmalloc(MAX_LENGTH,GFP_KERNEL);
	return 0;
}

static __exit void  char_dev_exit(void)
{
	 device_destroy(vdeg_class, mydev);
	 class_destroy(vdeg_class);
	 cdev_del(vdeg_cdev);
	 unregister_chrdev_region(mydev,1);
	 kfree(char_device_buf);
	 printk(KERN_INFO "\n Driver unregistered \n");
}
module_init(char_dev_init);
module_exit(char_dev_exit);

MODULE_LICENSE("GPL");
