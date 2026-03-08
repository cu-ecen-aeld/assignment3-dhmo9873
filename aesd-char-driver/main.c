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
//https://www.google.com/search?q=container_of+linux&sxsrf=ANbL-n4Rlsx1iceB76YixxQ9GeqcLngSbA%3A1772957288258
//https://codefinity.com/courses/v2/d2508ed4-bbc6-406b-b4a1-ba4945da1862/99ffb985-49f1-4aaa-9146-e72f2626d70b/ceb5732e-671f-4df1-b0a6-52cd8d664eb6
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("DHIGVIJAY MOHAN"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
	struct aesd_dev *dev;
	dev = container_of(inode->i_cdev,struct aesd_dev, cdev);
	filp->private_data = dev;
    /**
     * TODO: handle open
     */
	
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	struct aesd_dev *dev = filp->private_data;
	size_t entry_offset_byte_rtn;
	struct aesd_buffer_entry *current_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer,*f_pos,&entry_offset_byte_rtn);

	

    if (current_entry == NULL){
		mutex_unlock(&dev->mutex_lock);
        return 0;
	}

	while (count > 0 && entry_offset_byte_rtn < current_entry->size) {

		if (copy_to_user(buf, current_entry->buffptr + entry_offset_byte_rtn, 1)) {
			mutex_unlock(&dev->mutex_lock);
			return -EFAULT;
		}

		buf++; 
		entry_offset_byte_rtn++;
		(*f_pos)++;
		retval++;
		count--;
	}
	/**
	 * TODO: handle read
	 */
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos)
{
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	struct aesd_dev *dev = filp->private_data;
	char *new_data;
	const char *overwritten_ptr;

	new_data = krealloc(dev->partial_write_buffer.buffptr, dev->partial_write_buffer.size + count, GFP_KERNEL);
	if (!new_data) {
		mutex_unlock(&dev->mutex_lock);
		return -ENOMEM;
	}

	if (copy_from_user(new_data + dev->partial_write_buffer.size, buf, count)) {
		mutex_unlock(&dev->mutex_lock);
		return -EFAULT;
	}

	dev->partial_write_buffer.buffptr = new_data;
	dev->partial_write_buffer.size += count;

	if (new_data[dev->partial_write_buffer.size - 1] == '\n') {
		overwritten_ptr = aesd_circular_buffer_add_entry(&dev->buffer, &dev->partial_write_buffer);
		if (overwritten_ptr) {
			kfree(overwritten_ptr);
		}
		dev->partial_write_buffer.buffptr = NULL;
		dev->partial_write_buffer.size = 0;
	}

	mutex_unlock(&dev->mutex_lock);
	return count;
	/**
	 * TODO: handle write
	 */
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
	 mutex_init(&aesd_device.mutex_lock);

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
	int ind;
	struct aesd_buffer_entry *entry;

	AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, ind){
		kfree(entry->buffptr);
	}

	kfree(aesd_device.partial_write_buffer.buffptr);

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
