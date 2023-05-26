#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include "encdec.h"

#define MODULE_NAME "encdec"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR NAME");

int 	encdec_open(struct inode* inode, struct file* filp);
int 	encdec_release(struct inode* inode, struct file* filp);
int 	encdec_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar(struct file* filp, char* buf, size_t count, loff_t* f_pos);
ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos);

ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos);
ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos);

int memory_size = 0;

MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops_caesar = {
    .open = encdec_open,
    .release = encdec_release,
    .read = encdec_read_caesar,
    .write = encdec_write_caesar,
    .llseek = NULL,
    .ioctl = encdec_ioctl,
    .owner = THIS_MODULE
};

struct file_operations fops_xor = {
    .open = encdec_open,
    .release = encdec_release,
    .read = encdec_read_xor,
    .write = encdec_write_xor,
    .llseek = NULL,
    .ioctl = encdec_ioctl,
    .owner = THIS_MODULE
};

//this structure is our file-object's private data structure
typedef struct {
    unsigned char key;
    int read_state;
} encdec_private_data;

char* XorBuff;
char* caeBuff;

//*********************************************************************************************
int init_module(void)
{
    major = register_chrdev(major, MODULE_NAME, &fops_caesar);
    if (major < 0)
    {
        return major;
    }
    XorBuff = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
    if (!XorBuff)
    {
        return -ENOMEM;
    }
    caeBuff = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
    if (!caeBuff)
    {
        return -ENOMEM;
    }
    return 0;
}
//*********************************************************************************************
void cleanup_module(void)
{
    unregister_chrdev(major, MODULE_NAME);// Unregister the device-driver
    kfree(XorBuff); //  Free the allocated device buffers using kfree
    kfree(caeBuff);
}

int encdec_open(struct inode* inode, struct file* filp)
{
    //safaa said, Assume normality for the values so this check doesnt matter
    // Check if filp or inode is NULL
    if (!filp || !inode) {
        return -EINVAL;// Invalid argument
    }

    int minor = MINOR(inode->i_rdev);

    if (minor == 1) {// Setting the 'filp->f_op' to the correct file-operations structure according to the minor value
        filp->f_op = &fops_xor;
    }
    else {
        filp->f_op = &fops_caesar;
    }

    // Allocate memory for 'filp->private_data' as needed
    filp->private_data = (encdec_private_data*)kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
    if (!filp->private_data) {
        return -ENOMEM;  // Failed to allocate memory
    }

    //init the default values of the fields in the encdec_private_data
    encdec_private_data* dev_private_data = (encdec_private_data*)filp->private_data;

    // Initialize the fields in the encdec_private_data structure
    dev_private_data->read_state = ENCDEC_READ_STATE_DECRYPT;
    dev_private_data->key = 0;

    return 0;
}
//*********************************************************************************************
int encdec_release(struct inode* inode, struct file* filp)
{
    //safaa said, Assume normality for the values so this check doesnt matter
    // Check if filp or inode is NULL
    if (!filp || !inode) {
        return -EINVAL;// Invalid argument
    }
    kfree(filp->private_data);	// Free the allocated memory for 'filp->private_data'
    return 0;

}
//*************************************************************************************************
int encdec_ioctl(struct inode* inode, struct file* filp, unsigned int cmd, unsigned long arg)
{
    //safaa said, Assume normality for the values so this check doesnt matter
    // Check if filp or inode is NULL
    if (!filp || !inode) {
        return -EINVAL;// Invalid argument
    }

    int minor = MINOR(inode->i_rdev);
    encdec_private_data* dev_private_data = (encdec_private_data*)filp->private_data;

    // Check the command type and perform corresponding actions
    if (cmd == ENCDEC_CMD_CHANGE_KEY) {
        dev_private_data->key = (unsigned char)arg;
    }

    else if (cmd == ENCDEC_CMD_SET_READ_STATE) {
        dev_private_data->read_state = (int)arg;
    }

    else if (cmd == ENCDEC_CMD_ZERO) {
        int i = 0;
        for (i = 0; i < memory_size; i++) {
            if (minor) {
                XorBuff[i] = 0;
            }
            else {
                caeBuff[i] = 0;
            }
        }
    }
    return 0;
}

//********************************************************************************************************

ssize_t encdec_read_caesar(struct file* filp, char* buf, size_t count, loff_t* f_pos) {
    // if filp==NULL return error bad file
    if (!filp) {
        return -EBADF;
    }
    //cant read from the encryption device, because *f_pos value =memroy_size return the error
    if (*f_pos >= memory_size) {
        return -EINVAL;
    }
    else { // if(f_pos<memory_size)
        //Allocate a temporary buffer to help with reading
        char* temp = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
        if (!temp) {
            return -ENOMEM;  // Failed to allocate memory
        }
        int counter = 0;
        encdec_private_data *dev_private_data = (encdec_private_data *)filp->private_data;
        while (*f_pos + counter < memory_size && counter < count) {
            temp[counter] = caeBuff[*f_pos + counter];
            if(dev_private_data->read_state){
            temp[counter] =((temp[counter] - (int)dev_private_data->key) + 128) % 128;
           }
            counter++;
        }
        // Update the file position indicator
        filp->f_pos = *f_pos + counter;
        // Copy data from kernel space to user space
        copy_to_user(buf, temp, counter);
        // Free the temporary buffer
        kfree(temp);
        // Return the number of bytes read
        return counter;
    }
}

//********************************************************************************************************
ssize_t encdec_write_caesar(struct file* filp, const char* buf, size_t count, loff_t* f_pos) {
    // if filp==NULL return error bad file
    if (!filp) {
        return -EBADF;
    }
    //cant write to the encryption device, because *f_pos value =memroy_size return the error
    if (*f_pos >= memory_size ) {
        return -ENOSPC;
    }
    else {
        //Allocate a temporary buffer to help with reading
        char* temp = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
        if (!temp) {
            return -ENOMEM;  // Failed to allocate memory
        }
        copy_from_user(temp, buf, count);
        int position = 0;
        // Perform CAESAR encryption and write data to the caeBuff
        while (*f_pos + position < memory_size && position < count) {
            temp[position] = (temp[position] + (int)((encdec_private_data*)filp->private_data)->key) % 128;
            caeBuff[*f_pos + position] = temp[position];
            position++;
        }
        // Update the file position indicator
        *f_pos = *f_pos + position;
        // Free the temporary buffer
        kfree(temp);
        // Return the number of bytes written
        return position;
    }
}
//********************************************************************************************************
ssize_t encdec_read_xor(struct file* filp, char* buf, size_t count, loff_t* f_pos) {
    // if filp==NULL return error bad file
    if (!filp) {
        return -EBADF;
    }
    //cant read from the encryption device, because *f_pos value =memroy_size return the error
    if (*f_pos >= memory_size) {
        return -EINVAL;
    }
    else { // if(f_pos<memory_size)
        char* temp = kmalloc(memory_size, GFP_KERNEL);
        if (!temp) {
            return -ENOMEM;  // Failed to allocate memory
        }
        int counter = 0;
        encdec_private_data* dev_private_data = (encdec_private_data*)filp->private_data;
        // Read data from the XorBuff and perform XOR decryption
        while (*f_pos + counter < memory_size && counter < count) {
            temp[counter] = XorBuff[*f_pos + counter];
            if (dev_private_data->read_state) {
                temp[counter] = temp[counter] ^ (int)dev_private_data->key;
            }
            counter++;
        }
        // Update the file position indicator
        filp->f_pos = *f_pos + counter;
        // Copy data from kernel space to user space
        copy_to_user(buf, temp, counter);

        // Free the temporary buffer
        kfree(temp);
        // Return the number of bytes read
        return counter;
    }
}

//********************************************************************************************************
ssize_t encdec_write_xor(struct file* filp, const char* buf, size_t count, loff_t* f_pos) {
    // if filp==NULL return error bad file
    if (!filp) {
        return -EBADF;
    }
    // Check if writing exceeds the available memory size
    if (*f_pos >= memory_size) {
        return -ENOSPC;// No space left on device
    }
    else {
        //Allocate a temporary buffer to help with writing
        char* temp = (char*)kmalloc(memory_size * sizeof(char), GFP_KERNEL);
        if (!temp) {
            return -ENOMEM;  // Failed to allocate memory
        }

        copy_from_user(temp, buf, count);

        int position = 0;

        // Perform XOR encryption and write data to the XorBuff
        while (*f_pos + position < memory_size && position < count) {
            temp[position] = temp[position] ^ (int)((encdec_private_data*)filp->private_data)->key;
            XorBuff[*f_pos + position] = temp[position];
            position++;
        }

        // Update the file position indicator
        *f_pos = *f_pos + position;

        // Free the temporary buffer
        kfree(temp);
        // Return the number of bytes written
        return position;
    }
}
