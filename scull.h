#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/kernel.h>
#include<linux/slab.h>
#include<linux/string.h>
#include<linux/uaccess.h>
#include<linux/types.h>

struct scull_dev {
        struct scull_qset *data;
        unsigned long size;
        unsigned long quantum;
        unsigned long qset;
        struct cdev cdev;
        unsigned int access_key;
        struct semaphore sem;
};

static int setup_scull(struct scull_dev *dev);
int scull_trim(struct scull_dev *dev);
int scull_open(struct inode *inode, struct file *filp);
ssize_t scull_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
struct scull_qset *scull_follow(struct scull_dev *dev, int qset_num);
int create_qset(struct scull_qset **qset);
static void scull_cleanup_module(void);
int scull_release(struct inode *inode, struct file *filp);