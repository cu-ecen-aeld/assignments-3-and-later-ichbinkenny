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
    aesd_device = *p_aesd_dev;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    struct aesd_dev* p_aesd_dev;
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    filp->private_data = NULL;
//     p_aesd_dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
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
    ssize_t retval = 0, status = 0;
    uint8_t entry_index;
    ssize_t read_offset;
    ssize_t read_size;
    struct aesd_dev* p_aesd_dev;
    struct aesd_buffer_entry* entry;
    
    p_aesd_dev = filp->private_data;

    // Find entry index 
    entry_index = p_aesd_dev->circular_buff->out_offs;
    // Get the entry
    if (NULL != (entry = aesd_circular_buffer_find_entry_offset_for_fpos(p_aesd_dev->circular_buff, *f_pos, &read_offset)))
    {
        // determine read size
        read_size = (count < (entry->size - read_offset)) ? count : (entry->size - read_offset);
        // copy entry data to user
        status = copy_to_user(buf, (entry->buffptr + read_offset), read_size);
        if (0 != status)
        {
            return -EFAULT;
        }
        else
        {
            *f_pos += read_size;
            return read_size;
        }
    }
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM, status;
    bool append_data = false;
    char* message;
    struct aesd_dev* p_aesd_dev = filp->private_data;
    uint8_t current_index = p_aesd_dev->circular_buff->in_offs;
    uint8_t previous_index = (current_index != 0) ? (current_index - 1) : AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED - 1; 
    struct aesd_buffer_entry* previous_entry = &p_aesd_dev->circular_buff->entry[previous_index];
    struct aesd_buffer_entry* current_entry = &p_aesd_dev->circular_buff->entry[current_index];

    // Check previous entry and see if the incoming data is to be appended.
    if (p_aesd_dev->circular_buff->full)
    {
        // Check if an append is needed
        if (previous_entry->buffptr != NULL && previous_entry->buffptr[previous_entry->size - 1] != '\n')
        {
            PDEBUG("Appending to a previous entry.\n");
            // Need to append to previous entry.
            krealloc(previous_entry->buffptr, (previous_entry->size + count), GFP_KERNEL);
            status = copy_from_user((previous_entry->buffptr + previous_entry->size), buf, count);
            if (0 != status)
            {
                return -EFAULT;
            }
            previous_entry->size += count;
            retval = count;
            // Do not move write pointer as we modified the previous entry, not the current one.
        }
        else
        {
            PDEBUG("Replacing previous entry.\n");
            // Overwrite item in current_entry with new message.
            krealloc(current_entry->buffptr, count, GFP_KERNEL);
            status = copy_from_user(current_entry->buffptr, buf, count);
            if (0 != status)
            {
                return -EFAULT;
            }
            current_entry->size = count;
            retval = count;
            // Move to next write index.
            p_aesd_dev->circular_buff->in_offs = (p_aesd_dev->circular_buff->in_offs + 1 ) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        }
    }
    else
    {
        // New entry
        // Setup new data.
        message = kmalloc(count, GFP_KERNEL);
        memset(message, 0, count);
        status = copy_from_user(message, buf, count);
        if (0 != status)
        {
            // Release message resource,
            kfree(message);
            return -ERESTARTSYS;
        }
        // Store message
        struct aesd_buffer_entry* entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);

        entry->buffptr = message;
        entry->size = count;
        aesd_circular_buffer_add_entry(p_aesd_dev->circular_buff, entry);
        retval = count;
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
    for (uint8_t i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; ++i)
    {
	if (NULL != aesd_device.circular_buff->entry[i].buffptr)
	{
		kfree(aesd_device.circular_buff->entry[i].buffptr);
	}
    }
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
