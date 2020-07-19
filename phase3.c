#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/rcupdate.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vahid Zee");
MODULE_DESCRIPTION("A Simple PID tracker Device Driver");
MODULE_VERSION("1.0");
#define DEVICE_NAME "task_tracker"

#define MSG_BUFFER_LEN 16384
#define INPUT_BUFFER_MAX_SIZE 16
#define FILE_PATH_LEN 1024
#define MAX_NUM_OPEN_FILES 256
#define MAJOR_NUMBER 0 // if set to zero will be dynamically allocated

/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);

static int device_release(struct inode *, struct file *);

static ssize_t device_read(struct file *, char *, size_t, loff_t *);

static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int cur_pid = 1;
static int device_open_count = 0;

static int major_num;

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release};

/* When a process reads from our device, this gets called. */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset)
{
    int bytes_read = len, i = 0;
    char * msg = (char *) kmalloc(GFP_KERNEL,MSG_BUFFER_LEN*sizeof(char));
    char * buf = (char *) kmalloc(GFP_KERNEL,FILE_PATH_LEN*sizeof(char));

    char *msg_ptr, *cwd;
    
    // getting PCB
    sprintf(msg, "NOT FOUND\n");
    

    /* Set the msg_ptr to the buffer */
    msg_ptr = msg;

    /* Put data in the buffer */
    while (len-- && *msg_ptr)
        put_user(*(msg_ptr++), buffer++);
    kfree(buf);
    kfree(msg);
    return bytes_read;
}

/* Called when a process tries to write to our device */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset)
{
    char *ptr;
    char input_buffer[INPUT_BUFFER_MAX_SIZE];
    int bytes_count = len;

    if (len > INPUT_BUFFER_MAX_SIZE)
    {
        printk(KERN_ALERT "input is larger than expected - skipping input");
        return len;
    }
    printk(KERN_INFO "updating pid(%d) input(%d)", cur_pid, bytes_count);
    ptr = input_buffer;
    while (bytes_count--)
        get_user(*(ptr++), buffer++);
    sscanf(input_buffer, "%d", &cur_pid);
    printk(KERN_INFO "pid is updated to %d", cur_pid);
    return len;
}

/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *file)
{
    /* If device is open, return busy */
    if (device_open_count)
        return -EBUSY;
    
    device_open_count++;
    try_module_get(THIS_MODULE);
    return 0;
}

/* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *file)
{
    /* Decrement the open counter and usage count. Without this, the module would not unload. */
    device_open_count--;
    module_put(THIS_MODULE);
    return 0;
}

static int __init module_startup(void)
{
    printk(KERN_INFO "initializing module\n");
    major_num = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &file_ops);
    if (major_num < 0)
    {
        printk(KERN_ALERT "Could not register device: %d\n", major_num);
        return major_num;
    }
    else
    {
        printk(KERN_INFO "Phase3 module loaded with device major number % d\n", major_num);
        return 0;
    }
}
static void __exit module_cleanup(void)
{
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "Phase3 module successfully unloaded\n");
}
module_init(module_startup);
module_exit(module_cleanup);