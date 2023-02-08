/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Kenneth Hunter"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev* p_aesd_dev;
    PDEBUG("open");
    p_aesd_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    // See if we need to create a circular buffer (i.e. this is the first open call)
    if (NULL == p_aesd_dev->circular_buff)
    {
	p_aesd_dev->circular_buff = kzalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
    }
    // Set filp->private_data with contents of AESD_DEV struct.
    //
    filp->private_data = p_aesd_dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    struct aesd_dev* p_aesd_dev;
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    p_aesd_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    // free allocated buffer memory for circular buffer
//     if (NULL != p_aesd_dev->circular_buff)
//     {
// 	// Free allocated entries in circular buffer
// 	for (size_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; ++i)
// 	{
// 		if (NULL != p_aesd_dev->circular_buff->entry[i].buffptr)
// 		{
// 			kfree(p_aesd_dev->circular_buff->entry[i].buffptr);
// 		}
// 	}
// 	kfree(p_aesd_dev->circular_buff);
//     }
//     filp->private_data = NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    char* message;
    struct aesd_dev* p_aesd_dev;
    struct aesd_buffer_entry* write_entry;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    if (NULL != filp && NULL != filp->private_data && NULL != buf)
    {
	message = kmalloc(count, GFP_KERNEL);
	if (0 != copy_from_user(message, buf, count))
	{
		kfree(message);
		retval = -EFAULT;
	}
	else
	{

    		p_aesd_dev = (struct aesd_dev*)filp->private_data;
		write_entry = (struct aesd_buffer_entry*)kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
		write_entry->size = count;
		write_entry->buffptr = message;
		// TODO: implement with an offset if a newline wasnt found. 
		aesd_circular_buffer_add_entry(p_aesd_dev->circular_buff, write_entry);
		PDEBUG("Added entry %s to the buffer at position %d\n", message, p_aesd_dev->circular_buff->in_offs - 1);
		retval = count;
	}
    }

    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    // Setup dev major and minor using allocation.
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
