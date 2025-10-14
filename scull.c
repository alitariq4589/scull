#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/kernel.h>
#include<linux/slab.h>
#include<linux/string.h>
#include<linux/uaccess.h>

#include "scull.h"

#define SCULL_QUANTUM 4000
#define SCULL_QSET 1000



struct scull_dev {
        struct scull_qset *data;
        unsigned long size;
        unsigned long quantum;
        unsigned long qset;
        struct cdev cdev;
        unsigned int access_key;
        struct semaphore sem;
};

struct scull_qset {
        struct scull_qset *next;
        void **data;
};

struct file_operations fops = {
        .owner = THIS_MODULE,
        // .ioctl = scull_ioctl,
        .open = scull_open,
        .read = scull_read,
        .write = scull_write,
        // .llseek = scull_llseek,
        .release = scull_release,
};

static struct scull_dev scull_device;

dev_t scull_devnum;

static int __init scull_init(){

        memset(&scull_device,)

        scull_device.size = 0;
        scull_device.quantum = SCULL_QUANTUM;
        scull_device.qset = SCULL_QSET;
        scull_device.data = NULL;

        int result = setup_scull(&scull_device);
        if (result < 0){
                scull_cleanup_module();
                return result;
        }
        return 0;
}

void cdev_init(struct cdev *cdev, struct file_operations *fops)
{
        cdev->owner = THIS_MODULE;
        cdev->ops = fops;
}

static int setup_scull(struct scull_dev *dev)
{
        dev_t dev_num;
        
        if (alloc_chardev_region(&dev_num, 0, 1, "scull")){
                printk(KERN_ALERT "scull: Device Number allocation failed!\n");
                return -EFAULT;
        }
        
        cdev_init(&dev->cdev, &fops);
        int result = cdev_add(&dev->cdev, dev_num, 1); //Already defined in the kernel
        
        if (result){
                printk(KERN_ALERT "scull: Failed to add chardevice in the kernel")
        }
        scull_devnum = dev_num;
        
        return 0;
}

int scull_trim(struct scull_dev *dev)
{
        struct scull_qset *current, *next;
        
        unsigned long i, qset = dev->qset;

        current = dev->data;

        for (current = dev->data; current; current = next){
                next = current->next;
                if (current->data){
                        for (i = 0; i < qset; i++){
                                kfree(current->data[i]);
                        }
                        kfree(current->data);
                        current->data = NULL
                }
                kfree(current);
        }
        dev->size = 0;
        dev->quantum = SCULL_QUANTUM;
        dev->qset = SCULL_QSET;
        dev->data = NULL;
        return 0;
                
}

int scull_open(struct inode *inode, struct file *filp)
{
        struct scull_dev *dev;

        dev = container_of(inode->i_cdev, struct scull_dev, cdev);

        filp->private_data = dev;

        if ((filp->f_flags & O_ACCMODE) == O_WRONLY){
                scull_trim(dev);
        }

        return 0;

}

size_t scull_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
        struct scull_dev *dev = filp->private_data;
        unsigned long rest

        int retval = 0;

        unsigned long scull_size = dev->qset * dev->quantum;

        struct scull_qset *read_pointer;

        if (*f_pos+count > dev->size){
                count = dev->size-*f_pos;
        }

        unsigned long qset_num = (unsigned long) (*f_pos) / scull_size;
        int remaining_qset = (unsigned long) (*f_pos) % scull_size
        int quantum_num = remaining_qset / dev->quantum;
        int remaining_quantum = remaining_qset % dev->quantum;

        read_pointer = scull_follow(dev, qset_num);

        if (read_pointer == NULL || read_pointer->data == NULL || read_pointer->data[quantum_num] == NULL)
                goto out;
        
        if (count > dev->quantum - remaining_quantum)
                count = dev->quantum - remaining_quantum;

        if (copy_to_user(buff, read_pointer->data[quantum_num] + remaining_quantum, count)){
                retval = -EFAULT
                goto out;
        }
        *fpos += count;
        out:    
                if (retval)
                        printk(KERN ALERT "Couldnt read complete scull data")
                return retval;


}


size_t scull_write(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
        struct scull_dev *dev = filp->private_data;
        unsigned long rest

        int retval = 0;

        unsigned long scull_size = dev->qset * dev->quantum;

        struct scull_qset *write_pointer;

        if (*f_pos+count > dev->size){
                count = dev->size-*f_pos;
        }

        unsigned long qset_num = (unsigned long) (*f_pos) / scull_size;
        int remaining_qset = (unsigned long) (*f_pos) % scull_size
        int quantum_num = remaining_qset / dev->quantum;
        int remaining_quantum = remaining_qset % dev->quantum;

        write_pointer = scull_follow(dev, qset_num);

        if (write_pointer == NULL)
                goto out;
        if (write_pointer->data == NULL){
                write_pointer->data = kmalloc(dev->qset *  sizeof(char *), GFP_KERNEL);
                if (write_pointer->data == NULL){
                        retval = -EFAULT;
                        goto out
                }
                memset(write_pointer->data, 0, remaining_quantum * sizeof(char *));
        }
        if (write_pointer->data[quantum_num] == NULL){
                write_pointer->data[quantum_num] = kmalloc(dev->quantum, GFP_KERNEL);
        }
        
        if (count > dev->quantum - remaining_quantum)
                count = dev->quantum - remaining_quantum;

        if (copy_from_user(write_pointer->data[quantum_num] + remaining_quantum, buff,  count)){
                retval = -EFAULT
                goto out;
        }

        *f_pos += count;

        if (*f_pos > dev->size)
                dev->size = *f_pos

        out:    
                if (retval)
                        printk(KERN ALERT "Couldnt read complete scull data")
                return retval;


}

int scull_follow(struct scull_dev *dev, int qset_num)
{
        int i;
        struct scull_qset *qset_pointer = dev->data;

        if (dev->data == NULL){
                if (create_qset(&dev->data))
                        return NULL;
        }

        for (i = 0; i < qset_num; i++){
                if (qset_pointer->next == NULL){
                         if (create_qset(&qset_pointer->next))
                            return NULL; // Allocation failed
                }

                qset_pointer = qset_pointer->next;
        }
        return qset_pointer;
}

int create_qset(struct scull_qset **qset){
        *qset = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (!*qset){
                printk(KERN_ALERT "scull: New qset allocation failed!");
                return -ENOMEM;
        }
        memset (*qset, 0, sizeof(struct scull_qset));
        return 0;
}

static void scull_cleanup_module(void)
{
    if (scull_device.cdev.ops) {
        cdev_del(&scull_device.cdev);
    }
    
    // 2. Unregister the allocated device major/minor numbers
    if (scull_devnum) {
        // Assume only 1 device was registered starting from scull_dev_number
        unregister_chrdev_region(scull_devnum, 1);
    }
    
    scull_trim(&scull_device); 
    
    printk(KERN_INFO "SCULL: Device unloaded.\n");
}

module_init(scull_init);
module_exit(scull_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ali Tariq");
MODULE_DESCRIPTION("A simple scull-like char driver");