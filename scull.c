#include "scull.h"

#define SCULL_QUANTUM 4000
#define SCULL_QSET 1000


#define SCULL_DEBUG 1


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

static int __init scull_init(void){



        scull_device.size = 0;
        scull_device.quantum = SCULL_QUANTUM;
        scull_device.qset = SCULL_QSET;
        scull_device.data = NULL;

        int result = setup_scull(&scull_device);
        if (result < 0){
                scull_cleanup();
                return result;
        }
        return 0;
}

static int setup_scull(struct scull_dev *dev)
{
        dev_t dev_num;
        
        if (alloc_chrdev_region(&dev_num, 0, 1, "scull")){
                printk(KERN_ALERT "scull: Device Number allocation failed!\n");
                return -EFAULT;
        }
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Device Number Major: %d, Minor: %d\n", MAJOR(dev_num), MINOR(dev_num));
#endif
        dev->cdev.owner = THIS_MODULE;
        cdev_init(&dev->cdev, &fops);
        int result = cdev_add(&dev->cdev, dev_num, 1); //Already defined in the kernel
        
        if (result){
                printk(KERN_ALERT "scull: Failed to add chardevice in the kernel");
        }
        scull_devnum = dev_num;
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Device added to the kernel successfully\n");
#endif
        return 0;
}

int scull_trim(struct scull_dev *dev)
{

#if SCULL_DEBUG
        printk(KERN_INFO "scull: Trimming device\n");
#endif
        struct scull_qset *current_qset, *next;
        
        unsigned long i, qset = dev->qset;

        current_qset = dev->data;

        for (current_qset = dev->data; current_qset; current_qset = next){
                next = current_qset->next;
                if (current_qset->data){
                        for (i = 0; i < qset; i++){
                                if (current_qset->data[i])
                                        kfree(current_qset->data[i]);
                        }
                        kfree(current_qset->data);
                        current_qset->data = NULL;
                }
                kfree(current_qset);
        }
        dev->size = 0;
        dev->quantum = SCULL_QUANTUM;
        dev->qset = SCULL_QSET;
        dev->data = NULL;

#if SCULL_DEBUG
        printk(KERN_INFO "scull: Device trimmed successfully\n");
#endif
        return 0;
                
}

int scull_open(struct inode *inode, struct file *filp)
{
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Opening device\n");
#endif
        struct scull_dev *dev;

        dev = container_of(inode->i_cdev, struct scull_dev, cdev);

        filp->private_data = dev;

        if ((filp->f_flags & O_ACCMODE) == O_WRONLY){
#if SCULL_DEBUG
                printk(KERN_INFO "scull: Opening in write mode, trimming the device\n");
#endif
                scull_trim(dev);
        }

        return 0;

}

int scull_release(struct inode *inode, struct file *filp)
{
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Releasing device\n");
#endif
        return 0;
}

ssize_t scull_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{

#if SCULL_DEBUG
        printk(KERN_INFO "scull: Reading from device\n");
#endif
        struct scull_dev *dev = filp->private_data;

        int retval = 0;

        unsigned long scull_size = dev->qset * dev->quantum;

        struct scull_qset *read_pointer;

        if (*f_pos+count > dev->size){
                count = dev->size-*f_pos;
        }

        unsigned long qset_num = (unsigned long) (*f_pos) / scull_size;
        int remaining_qset = (unsigned long) (*f_pos) % scull_size;
        int quantum_num = remaining_qset / dev->quantum;
        int remaining_quantum = remaining_qset % dev->quantum;

        read_pointer = scull_follow(dev, qset_num);

        if (read_pointer == NULL || read_pointer->data == NULL || read_pointer->data[quantum_num] == NULL)
                goto out;
        
        if (count > dev->quantum - remaining_quantum)
                count = dev->quantum - remaining_quantum;

        if (copy_to_user(buff, read_pointer->data[quantum_num] + remaining_quantum, count)){
                retval = -EFAULT;
                goto out;
        }
        *f_pos += count;
        out:    
                if (retval)
                        printk(KERN_ALERT "Couldnt read complete scull data");
                return retval;


}


ssize_t scull_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{

#if SCULL_DEBUG
        printk(KERN_INFO "scull: Writing to device\n");
#endif
        struct scull_dev *dev = filp->private_data;

        int retval = 0;

        unsigned long scull_size = dev->qset * dev->quantum;

        struct scull_qset *write_pointer;

        if (*f_pos+count > dev->size){
                count = dev->size-*f_pos;
        }

        unsigned long qset_num = (unsigned long) (*f_pos) / scull_size;
        int remaining_qset = (unsigned long) (*f_pos) % scull_size;
        int quantum_num = remaining_qset / dev->quantum;
        int remaining_quantum = remaining_qset % dev->quantum;

        write_pointer = scull_follow(dev, qset_num);

        if (write_pointer == NULL){
#if SCULL_DEBUG
                printk(KERN_ALERT "scull: Failed to write. Write Pointer is empty\n");
#endif
                goto out;
        }
        if (write_pointer->data == NULL){
                write_pointer->data = kmalloc(dev->qset *  sizeof(char *), GFP_KERNEL);
                if (write_pointer->data == NULL){
#if SCULL_DEBUG
                printk(KERN_ALERT "scull: Failed to add data node\n");
#endif
                        retval = -EFAULT;
                        goto out;
                }
#if SCULL_DEBUG
                printk(KERN_INFO "scull: Allocated data node successfully. Zeroing memory\n");
#endif
                memset(write_pointer->data, 0, dev->qset * sizeof(char *));
        }
        if (write_pointer->data[quantum_num] == NULL){
                write_pointer->data[quantum_num] = kmalloc(dev->quantum, GFP_KERNEL);
#if SCULL_DEBUG
                printk(KERN_INFO "scull: Allocated quantum node successfully. Zeroing memory\n");
#endif
                if (write_pointer->data[quantum_num] == NULL){
#if SCULL_DEBUG
                printk(KERN_ALERT "scull: Failed to add quantum node\n");
#endif
                        retval = -EFAULT;
                        goto out;
                }
        }
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Byte count requested for write: %zu\n", count);
#endif       
        if (count > dev->quantum - remaining_quantum)
                count = dev->quantum - remaining_quantum;
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Byte count for current quantum write: %zu\n", count);
#endif

        if (copy_from_user(write_pointer->data[quantum_num] + remaining_quantum, buff,  count)){
                retval = -EFAULT;
                goto out;
        }

        *f_pos += count;
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Written %zu bytes to device\n", count);
#endif
        retval = count;

        if (*f_pos > dev->size)
                dev->size = *f_pos;

        out:    
                if (retval)
                        printk(KERN_ALERT "Couldnt read complete scull data");
                return retval;


}

struct scull_qset *scull_follow(struct scull_dev *dev, int qset_num)
{
        int i;
        struct scull_qset *qset_pointer = dev->data;

        if (dev->data == NULL){
#if SCULL_DEBUG
                printk(KERN_INFO "scull: First qset not found. Allocating first qset\n");
#endif
                if (create_qset(&dev->data))
                        return NULL;
        }

        for (i = 0; i < qset_num; i++){
                if (qset_pointer->next == NULL){
                         if (create_qset(&qset_pointer->next)){
#if SCULL_DEBUG
                                printk(KERN_ALERT "scull: Failed to allocate memory for qset in scull_follow\n");
#endif
                            return NULL; // Allocation failed
                         }
                }
#if SCULL_DEBUG
                printk(KERN_INFO "scull: qset available, Moving to next qset\n");
#endif
                qset_pointer = qset_pointer->next;
        }
        return qset_pointer;
}

int create_qset(struct scull_qset **qset){
#if SCULL_DEBUG
        printk(KERN_INFO "scull: Allocating memory for new qset with create_qset()\n");
#endif
        *qset = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (!*qset){
                printk(KERN_ALERT "scull: New qset allocation failed!");
                return -ENOMEM;
        }
        memset (*qset, 0, sizeof(struct scull_qset));
        return 0;
}

void scull_cleanup(void)
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
    
    printk(KERN_INFO "scull: Device unloaded.\n");
}

static void __exit scull_cleanup_module(void)
{
        scull_cleanup();
}

module_init(scull_init);
module_exit(scull_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ali Tariq");
MODULE_DESCRIPTION("A simple scull-like char driver");