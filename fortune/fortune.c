/**
 * http://www.ibm.com/developerworks/linux/library/l-proc/index.html
 *
 * Usage:
 * # insmod fortune.ko
 * $ echo My fortune text! > /proc/fortune
 * $ echo My other fortune text! > /proc/fortune
 * $ echo Yet another fortune text! > /proc/fortune
 * $ cat /proc/fortune
 * $ cat /proc/fortune
 * $ cat /proc/fortune
 * $ cat /proc/fortune
 * # rmmod fortune
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define COOKIE_BUF_SIZE PAGE_SIZE

MODULE_LICENSE("GPL");

ssize_t fortune_read(struct file *file, char *buf, size_t count, loff_t *f_pos);
ssize_t fortune_write(struct file *file, const char *buf, size_t count, loff_t *f_pos);

int fortune_init(void);
void fortune_exit(void);

module_init(fortune_init);
module_exit(fortune_exit);

struct file_operations fops = {
    .read = fortune_read,
    .write = fortune_write,
};

const char *proc_filename = "fortune";

char *cookie_buf;
struct proc_dir_entry *proc_file;

unsigned int read_index;
unsigned int write_index;

ssize_t fortune_read(struct file *file, char *buf, size_t count, loff_t *f_pos){
    int len;
    static int finished = 0;

    printk(KERN_INFO "f_pos: %i", *f_pos);

    if(finished){
        finished = 0;
        return 0;
    }

    if(read_index >= write_index){
        read_index = 0;
    }

    finished = 1;

    len = sprintf(buf, "%s\n", &cookie_buf[read_index]);

    read_index += len; // if I cat the empty /proc/fortune then this will be incremented to 1 and the next output will be stripped off the first letter
    *f_pos += len;

    return len;
}

ssize_t fortune_write(struct file *file, const char *buf, size_t count, loff_t *f_pos){
    int free_space = (COOKIE_BUF_SIZE - write_index) + 1;

    if(count > free_space){
        printk(KERN_INFO "Cookie pot full.\n");
        return -ENOSPC;
    }

    if(copy_from_user(&cookie_buf[write_index], buf, count)){
        return -EFAULT;
    }

    write_index += count;

    cookie_buf[write_index-1] = 0;

    return count;
}

int fortune_init(void){
    cookie_buf = vmalloc(COOKIE_BUF_SIZE);

    if(!cookie_buf){
        printk(KERN_INFO "Not enough memory for the cookie pot.\n");
        return -ENOMEM;
    }

    memset(cookie_buf, 0, COOKIE_BUF_SIZE);


    proc_file = proc_create(proc_filename, 0666, NULL, &fops);

    if(!proc_file){
        vfree(cookie_buf);
        printk(KERN_INFO "Cannot create fortune file.\n");
        return -ENOMEM;
    }

    read_index = 0;
    write_index = 0;

    printk(KERN_INFO "Fortune module loaded.\n");

    return 0;
}

void fortune_exit(void){
    remove_proc_entry(proc_filename, NULL);

    if(cookie_buf){
        vfree(cookie_buf);
    }

    printk(KERN_INFO "Fortune module unloaded.\n");
}
