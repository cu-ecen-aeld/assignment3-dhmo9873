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
//https://man7.org/linux/man-pages/man2/lseek.2.html
//https://fastbitlab.com/linux-device-driver-programming-lecture-45-lseek-method-implementation/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
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
    struct aesd_dev *dev = filp->private_data;
    size_t entry_offset_byte_rtn;
    struct aesd_buffer_entry *current_entry;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&dev->mutex_lock)) {
        return -ERESTARTSYS;
    }

    current_entry = aesd_circular_buffer_find_entry_offset_for_fpos(
                        &dev->buffer, *f_pos, &entry_offset_byte_rtn);

    if (current_entry) {
        size_t available = current_entry->size - entry_offset_byte_rtn;
        size_t bytes_to_copy = (available < count) ? available : count;

        if (copy_to_user(buf, current_entry->buffptr + entry_offset_byte_rtn, bytes_to_copy)) {
            retval = -EFAULT;
        } else {
            retval = bytes_to_copy;
            *f_pos += bytes_to_copy;
        }
    }

    mutex_unlock(&dev->mutex_lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
        loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    char *new_data;
    const char *overwritten_ptr;
    ssize_t retval = -ENOMEM;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&dev->mutex_lock)) {
        return -ERESTARTSYS;
    }

    new_data = krealloc(dev->partial_write_buffer.buffptr,
                        dev->partial_write_buffer.size + count,
                        GFP_KERNEL);
    if (!new_data) {
        mutex_unlock(&dev->mutex_lock);
        return -ENOMEM;
    }

    dev->partial_write_buffer.buffptr = new_data;

    if (copy_from_user(new_data + dev->partial_write_buffer.size, buf, count)) {
        mutex_unlock(&dev->mutex_lock);
        return -EFAULT;
    }

    dev->partial_write_buffer.size += count;
    retval = count;

    if (new_data[dev->partial_write_buffer.size - 1] == '\n') {
        overwritten_ptr = aesd_circular_buffer_add_entry(&dev->buffer,
                                                          &dev->partial_write_buffer);
        if (overwritten_ptr) {
            kfree(overwritten_ptr);
        }

        dev->partial_write_buffer.buffptr = NULL;
        dev->partial_write_buffer.size = 0;
    }

    mutex_unlock(&dev->mutex_lock);
    return retval;
}

loff_t aesd_lseek(struct file *filp, loff_t offset, int whence){

	struct aesd_dev *dev = filp->private_data;
	loff_t temp;
	int count,index;
    uint8_t entries;
	size_t total_size=0;

	if (mutex_lock_interruptible(&dev->mutex_lock)) {
        return -ERESTARTSYS;
    }

	entries = dev->buffer.full ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : dev->buffer.in_offs;

    for (count = 0; count < entries; count++) {
        index = (dev->buffer.out_offs + count) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        entry = &dev->buffer.entry[index];
        if (entry->buffptr != NULL) {
            total_size += entry->size;
        }
    }
    PDEBUG("current value of the file position %lld\n",filp->f_pos);

	switch(whence){
		case SEEK_SET:
			temp = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			break;
		case SEEK_END:
			temp = total_size + offset ;
			break;
		default:
			mutex_unlock(&dev->mutex_lock);
            return -EINVAL;	
	}
	
	if (temp < 0 || temp > total_size) {
        mutex_unlock(&dev->mutex_lock);
        return -EINVAL;
    }
	filp->f_pos = temp;

    mutex_unlock(&dev->mutex_lock);
	PDEBUG("lseek successful. New pos: %lld", temp);
    return temp;


}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
	.llseek = aesd_lseek,
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
    aesd_circular_buffer_init(&aesd_device.buffer);

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

	if (aesd_device.partial_write_buffer.buffptr) {
		kfree(aesd_device.partial_write_buffer.buffptr);
	}

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

