#include<linux/types.h>

static int scull_init();
static void scull_cleanup(struct scull_dev *dev);
void cdev_init(struct cdev *cdev, struct file_operations *fops);
static int setup_scull(struct scull_dev *dev);
int scull_trim(struct scull_dev *dev);
int scull_open(struct inode *inode, struct file *filp);
size_t scull_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
size_t scull_write(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
int scull_follow(struct scull_dev *dev, int qset_num);
int create_qset(struct scull_qset **qset);
static void scull_cleanup_module(void);