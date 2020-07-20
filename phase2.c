#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/rcupdate.h>
#include <linux/fdtable.h>
#include <linux/fs_struct.h>
#include <linux/time.h>
#include <linux/cred.h>
#include <linux/string.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vahid Zee");
MODULE_DESCRIPTION("Open Functionality revised");
MODULE_VERSION("1.0");
#define DEVICE_NAME "phase2"
#define LOG_FILE "/tmp/phase2log"
#define MAX_PATH_LEN 300
#define MAX_UID_LEN 20
#define LOG_LEN 500
#define USER_FLAG '0'
#define FILE_FLAG '1'
#define MAJOR_NUMBER 0 // if set to zero will be dynamically allocated

typedef asmlinkage long (*sys_call_ptr_t)(const struct pt_regs *);
static sys_call_ptr_t *sys_call_table;
typedef asmlinkage long (*custom_open) (const char __user *filename, int flags, umode_t mode);

custom_open old_open;


/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);

static int device_release(struct inode *, struct file *);

static ssize_t device_read(struct file *, char *, size_t, loff_t *);

static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

static int device_open_count = 0;
static int major_num;

typedef struct Node
{
    int uid;
    int secl;
    struct Node *next;
} u_entry;

typedef struct Nodef
{
    char *path;
    int secl;
    struct Nodef *next;
} f_entry;

static int users_count = 0;
static u_entry *users = NULL;
static int files_count = 0;
static f_entry *files = NULL;

/* This structure points to all of the device functions */
static struct file_operations file_ops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release};

u_entry * find_user_entry( int uid ){
    u_entry * cur = users;
    while(cur != NULL){
        if(cur->uid == uid)
            break;
        cur = cur->next;
    }
    return cur;
}
void add_user(int uid, int secl)
{
    u_entry *entry = find_user_entry(uid);
    if(entry != NULL){
        entry->secl = secl;
        return;
    }
    entry = (u_entry *)kmalloc(sizeof(u_entry),GFP_KERNEL);
    if (entry == NULL)
    {
        printk(KERN_INFO "Unable to allocate user entry");
        return;
    }
    entry->next = users;
    entry->secl = secl;
    entry->uid = uid;
    users = entry;
    printk(KERN_INFO "added user entry %d: uid(%d) secl(%d)\n", ++users_count, uid, secl);
}
f_entry * find_file_entry( char * path ){
    f_entry * cur = files;
    while(cur != NULL){
        if(strcmp(path, cur->path) == 0)
            break;
        cur = cur->next;
    }
    return cur;
}
void add_file(char *path, int secl)
{   
    f_entry *entry = find_file_entry(path);
    if(entry != NULL){
        entry->secl = secl;
        return;
    }
    entry = (f_entry *)kmalloc(sizeof(f_entry), GFP_KERNEL);
    if (entry == NULL)
    {
        printk(KERN_INFO "Unable to allocate file entry");
        return;
    }
    entry->next = files;
    entry->secl = secl;
    entry->path = path;
    files = entry;
    printk(KERN_INFO "added file entry %d: path(%s) secl(%d)\n", ++files_count, path, secl);
}

/* When a process reads from our device, this gets called. -> return a list of files & users */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset)
{
    int bytes_read = len;
    char *msg = (char *)kmalloc(((MAX_PATH_LEN * files_count + MAX_UID_LEN * users_count) + 2) * sizeof(char), GFP_KERNEL);
    if (msg == NULL)
    {
        printk(KERN_INFO "Cannot allocate buffer for read buffer driver");
        return 0;
    }
    sprintf(msg, "");
    char *msg_ptr;
    f_entry *cur_file = files;
    u_entry *cur_user = users;
    while (cur_file != NULL)
    {
        sprintf(msg, "%s%d%s:", msg, cur_file->secl, cur_file->path);
        cur_file = cur_file->next;
    }
    if (cur_user != NULL)
    {
        sprintf(msg, "%susers:", msg);
        while (cur_user != NULL)
        {
            sprintf(msg, "%s%d%d:", msg, cur_user->secl, cur_user->uid);
            cur_user = cur_user->next;
        }
    }
    sprintf(msg, "%s\n", msg);

    /* Set the msg_ptr to the buffer */
    msg_ptr = msg;
    /* Put data in the buffer */
    while (len-- && *msg_ptr)
        put_user(*(msg_ptr++), buffer++);
    kfree(msg);
    return bytes_read;
}

/* Called when a process tries to write to our device */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset)
{
    char *ptr;
    char input_buffer[MAX_PATH_LEN];
    int bytes_count = len;

    if (len > MAX_PATH_LEN)
    {
        printk(KERN_ALERT "input is larger than expected - skipping input");
        return len;
    }
    ptr = input_buffer;
    while (bytes_count--)
        get_user(*(ptr++), buffer++);
    if (input_buffer[0] == USER_FLAG)
    {
        int uid;
        sscanf(input_buffer + 2, "%d", &uid);
        add_user(uid, input_buffer[1] - '0');
    }
    else if (input_buffer[0] == FILE_FLAG)
    {
        int i = 0, j = 0;
        for(; i < len && input_buffer[i] != '\n'; i++);
        char * path = (char *)kmalloc((i-1) * sizeof(char), GFP_KERNEL);
        for(; j < i -2; j++)
            *(path + j) = input_buffer[2 + j];
        *(path + i - 2) = '\0';
        add_file(path, input_buffer[1] - '0');
    }
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

struct file *file_open(const char *path, int flags, int rights) 
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void log_access(const char * filename, int secf,  int cur_uid, int secu, int wo, int rw){
    char data[LOG_LEN];
    char timestr[80];
    struct timespec curr_tm;
    getnstimeofday(&curr_tm);
    
    sprintf(data, "uid:%d - secu: %d - filepath: %s - secf: %d - r(%d)w(%d)rw(%d) - time(%.2lu:%.2lu:%.2lu:%.6lu)\n", 
         cur_uid, secu,filename, secf, (!(wo|rw) ? 1: 0), (wo ? 1: 0), (rw ? 1: 0), (curr_tm.tv_sec / 3600) % (24),
                   (curr_tm.tv_sec / 60) % (60), curr_tm.tv_sec % 60, curr_tm.tv_nsec / 1000);
    printk(KERN_INFO, "%d\n", data);
    struct file * handle =  file_open(LOG_FILE, O_WRONLY|O_CREAT|O_APPEND, 0644);
    if(handle == NULL){
        sprintf(data, "cannot open log file");
        return;
    }
    mm_segment_t oldfs;
    unsigned long long offset =0;
    oldfs = get_fs();
    set_fs(get_ds());
    vfs_write(handle, data, strlen(data), &offset);
    set_fs(oldfs);
    filp_close(handle, NULL);
}

static asmlinkage long my_open(const char __user *filename, int flags, umode_t mode)
{
    int cur_uid = (int) get_current_user()->uid.val;
    int secu = 0, secf = 0;
    u_entry * cur_u;
    f_entry * cur_f;
    char kfilename[MAX_PATH_LEN];
    char *ptr = kfilename, *buffer=filename;
    int bytes_count = MAX_PATH_LEN;
    while (bytes_count--){
        get_user(*ptr, buffer++);
        if(*(ptr++) == '\0')
            break;
    }

    int write_only = flags & O_WRONLY;
	int read_write = flags & O_RDWR;
	
    if((cur_u = find_user_entry(cur_uid))!= NULL)
        secu = cur_u->secl;

    if((cur_f = find_file_entry(kfilename))!= NULL){
        secf = cur_f->secl;
        log_access(kfilename, secf, cur_uid,  secu, write_only, read_write );
    }

    if (secu == secf)
		return old_open(filename, flags, mode);
    
    if( secu < secf) {
        if(write_only)
            return old_open(filename, flags, mode);
        printk(KERN_INFO "read permition denied\n");
        return -1;
    } else {
        if(!(write_only|read_write))
            return old_open(filename, flags, mode);
        return -1;
    }
    
    return old_open(filename, flags, mode);
}

static int __init module_startup(void)
{
    printk(KERN_INFO "initializing module\n");
    major_num = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &file_ops);
    
    sys_call_table = (sys_call_ptr_t *)kallsyms_lookup_name("sys_call_table");
    old_open = (custom_open)sys_call_table[__NR_open];

    write_cr0(read_cr0() & (~0x10000));
    sys_call_table[__NR_open] = (sys_call_ptr_t)my_open;
    write_cr0(read_cr0() | 0x10000);
    printk(KERN_INFO "open syscall was replaced\n");

    if (major_num < 0)
    {
        printk(KERN_ALERT "Could not register device: %d\n", major_num);
        return major_num;
    }
    else
    {
        printk(KERN_INFO "phase2 module loaded with device major number % d\n", major_num);
        return 0;
    }
}
static void __exit module_cleanup(void)
{
    unregister_chrdev(major_num, DEVICE_NAME);
    u_entry * cur_u = users;
    u_entry * temp_u;
    f_entry * cur_f = files;
    f_entry * temp_f;
    while(cur_u != NULL){
        temp_u = cur_u;
        cur_u = cur_u->next;
        kfree(temp_u);
    }
    while(cur_f != NULL){
        temp_f = cur_f;
        cur_f = cur_f->next;
        kfree(temp_f);
    }
    write_cr0(read_cr0() & (~0x10000));
    sys_call_table[__NR_open] = (sys_call_ptr_t)old_open;    
    write_cr0(read_cr0() | 0x10000);
    printk(KERN_INFO "open syscall was replaced with the old one\n");
    printk(KERN_INFO "phase2 module successfully unloaded\n");
}
module_init(module_startup);
module_exit(module_cleanup);
///home/vahidzee/Desktop/OSProject/test_file