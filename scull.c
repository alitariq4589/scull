#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/cdev.h>
#include<linux/kernel.h>

#define SCULL_QUANTUM 4000
#define SCULL_QSET 1000



struct scull_dev *dev {
        struct scull_dev *data;
        unsigned long size;
        unsigned long quantum;
        unsigned long qset;
        struct cdev cdev;
        unsigned int access_key;
        struct semaphore sem;
};

struct scull_qset *qset{
        struct scull_qset *qset;
        void **data;
};

struct file_operations fops = {
        .owner = THIS_MODULE;
        .ioctl = scull_ioctl;
        .open = scull_open;
        .read = scull_read;
        .write = scull_write;
        .llseek = scull_llseek;
        .release = scull_release;
};

void cdev_init(struct cdev *cdev, struct file_operations *fops)
{
        cdev->owner = THIS_MODULE;
        cdev_alloc(cdev);
        cdev->ops = fops;
}

static int setup_scull(struct scull_dev *dev)
{
        dev_t dev_num;
        
        if (alloc_chardev_region(&dev_num, 0, 1, "scull")){
                printk(KERN_ALERT "scull: Device Number allocation failed!\n");
        }
        
        cdev_init(&dev->cdev, &fops);
        result = cdev_add(&dev->cdev, dev_num); //Already defined in the kernel
        
        if (result){
                printk(KERN_ALERT "scull: Failed to add chardevice in the kernel")
        }
        
        return 0;
}

int scull_trim(struct scull_dev *dev)
{
        struct scull_qset *current, *next;
        
        unsigned long i, qset = dev->qset;

        current = dev->data;
        next = current->next;

        for (current = dev->data; current; current = next){
                if (current->data){
                        for (i = 0; i < qset; i++){
                                kfree(current->data[i]);
                        }
                }
                kfree(current->data);
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

        inode->i_cdev = contianer_of(&dev->cdev, struct scull_dev *, dev);

        filp->private_data = dev;

        if (filp->f_flags * O_ACCMODE == O_WRONLY){
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

        if (fpos+count > dev->size){
                count = dev->size-fpos;
        }

        unsigned long qset_num = (unsigned long) (f_pos) / scull_size;
        int remaining_qset = (unsigned long) (f_pos) % scull_size
        int quantum_num = remaining_qset / dev->quantum;
        int remaining_quantum = remaining_qset % dev->quantum;

        read_pointer = scull_follow(dev, qset_num);

        if (read_pointer == NULL | read_pointer->data == NULL | read_pointer->data[quantum_num] == NULL)
                goto out;
        
        if (count > dev->quantum - remaining_quantum)
                count = dev->quantum - remaining_quantum;

        if (copy_to_user(buff, read_pointer->data[quantum] + remaining_quantum, count)){
                retval = -EFAULT
                goto out;
        }

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

        struct scull_qset *read_pointer;

        if (fpos+count > dev->size){
                count = dev->size-fpos;
        }

        unsigned long qset_num = (unsigned long) (f_pos) / scull_size;
        int remaining_qset = (unsigned long) (f_pos) % scull_size
        int quantum_num = remaining_qset / dev->quantum;
        int remaining_quantum = remaining_qset % dev->quantum;

        write_pointer = scull_follow(dev, qset_num);

        if (write_pointer == NULL | write_pointer->data[quantum_num] == NULL)
                goto out;
        if (write_pointer->data == NULL){
                write_pointer->data = kmalloc(dev->quantum *  sizeof(char *), GFP_KERNEL);
                if (write_pointer->data == NULL){
                        retval = -EFAULT;
                        goto out
                }
        }
        if (write_pointer->data[quantum_num] == NULL){
                write_pointer->data[quantum_num] = kmalloc(dev->quantum, GFP_KERNEL);
        }
        
        if (count > dev->quantum - remaining_quantum)
                count = dev->quantum - remaining_quantum;

        if (copy_from_user(write_pointer->data[quantum] + remaining_quantum, buff,  count)){
                retval = -EFAULT
                goto out;
        }

        fpos += count;

        dev->size = fpos

        out:    
                if (retval)
                        printk(KERN ALERT "Couldnt read complete scull data")
                return retval;


}