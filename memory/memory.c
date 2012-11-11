/**
 * http://www.freesoftwaremagazine.com/articles/drivers_linux
 *
 * First do:
 * # mknod /dev/memory c 60 0 (this will create a virtual device)
 * # chmod 666 /dev/memory (in order to have acces to the device)
 *
 * Usage:
 *
 * # insmod memory.ko
 * $ echo -n asd > /dev/memory
 * $ cat /dev/memory
 * # rmmod memory
 *
 * TODO:
 * http://stackoverflow.com/questions/5970595/create-a-device-node-in-code
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>

MODULE_LICENSE("GPL");

int memory_open(struct inode *inode, struct file *file);
int memory_release(struct inode *inode, struct file *file);
ssize_t memory_read(struct file *file, char *buf, size_t count, loff_t *f_pos);
ssize_t memory_write(struct file *file, const char *buf, size_t count, loff_t *f_pos);

int memory_init(void);
void memory_exit(void);

module_init(memory_init);
module_exit(memory_exit);

struct file_operations memory_fops = {
    read: memory_read, //.read = memory_read,
    write: memory_write, //.write = memory_write,
    open: memory_open, //.open = memory_open,
    release: memory_release, //.release = memory_release,
};

struct miscdevice memory_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "memory",
    .fops = &memory_fops,
};

const int memory_major = 60;
const char *module_name = "memory";

char *memory_buffer;

int memory_open(struct inode *inode, struct file *file){
    return 0;
}

int memory_release(struct inode *inode, struct file *file){
    return 0;
}

ssize_t memory_read(struct file *file, char *buf, size_t count, loff_t *f_pos){
    copy_to_user(buf, memory_buffer, 1);

    if(*f_pos == 0){
        *f_pos += 1;
        return 1;
    }
    else {
        return 0;
    }
}

ssize_t memory_write(struct file *file, const char *buf, size_t count, loff_t *f_pos){
    copy_from_user(memory_buffer, buf + count - 1, 1); // only the last char

    return 1; // we wrote just one char (the last one)
}

int memory_init(void){
    int result;

    /*result = register_chrdev(memory_major, module_name, &memory_fops);*/

    /*if(result < 0){*/
        /*printk(KERN_INFO "Memory module cannot obtain major number %d.\n", memory_major);*/
        /*return result;*/
    /*}*/

    result = misc_register(&memory_dev);

    if(result < 0){
        printk(KERN_INFO "Memory module cannot register device.\n");
        return result;
    }

    memory_buffer = kmalloc(1, GFP_KERNEL);

    if(!memory_buffer){
        result = -ENOMEM;
        goto fail;
    }

    memset(memory_buffer, 0, 1);
    printk(KERN_INFO "Loaded memory module.\n");
    return 0;

    fail:
        memory_exit();
        return result;
}

void memory_exit(void){
    /*unregister_chrdev(memory_major, module_name);*/
    misc_deregister(&memory_dev);

    if(memory_buffer){
        kfree(memory_buffer);
    }

    printk(KERN_INFO "Unloaded memory module.\n");
}
