#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Reuven Plevinsky");

static int major_number;
static struct class* sysfs_class = NULL;
static struct device* sysfs_device = NULL;
char* str = "without sysfs\n";
int str_len;

static unsigned int sysfs_int = 0;
static unsigned int sysfs_int_2 = 1;

int my_open(struct inode *_inode, struct file *_file)
{
	str_len = strlen(str);
	return 0;
}

ssize_t my_read(struct file *filp, char *buff, size_t length, loff_t *offp)
{
	if (!str_len)
		return 0;
	if (copy_to_user(buff, str, str_len))  // Send the data to the user through 'copy_to_user'
        return -EFAULT;
    str_len = 0;    
	return strlen(str);
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.read = my_read
};

ssize_t display(struct device *dev, struct device_attribute *attr, char *buf)	//sysfs show implementation
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", sysfs_int);
}

ssize_t display2(struct device *dev, struct device_attribute *attr, char *buf)	//sysfs show implementation
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", sysfs_int_2);
}

ssize_t modify(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)	//sysfs store implementation
{
	int temp;
	if (sscanf(buf, "%u", &temp) == 1)
		sysfs_int = temp;
	return count;	
}

ssize_t modify2(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)	//sysfs store implementation
{
	int temp;
	if (sscanf(buf, "%u", &temp) == 1)
		sysfs_int_2 = temp;
	return count;	
}

static DEVICE_ATTR(sysfs_att, S_IWUSR | S_IRUGO , display, modify);
static DEVICE_ATTR(sysfs_att2, S_IWUSR | S_IRUGO , display2, modify2);

static int __init sysfs_example_init(void)
{
	//create char device
	major_number = register_chrdev(0, "Sysfs_Device", &fops);\
	if (major_number < 0)
		return -1;
		
	//create sysfs class
	sysfs_class = class_create(THIS_MODULE, "Sysfs_class");
	if (IS_ERR(sysfs_class))
	{
		unregister_chrdev(major_number, "Sysfs_Device");
		return -1;
	}
	
	//create sysfs device
	sysfs_device = device_create(sysfs_class, NULL, MKDEV(major_number, 0), NULL, "sysfs_class" "_" "sysfs_Device");	
	if (IS_ERR(sysfs_device))
	{
		class_destroy(sysfs_class);
		unregister_chrdev(major_number, "Sysfs_Device");
		return -1;
	}
	
	//create sysfs file attributes	
	if (device_create_file(sysfs_device, (const struct device_attribute *)&dev_attr_sysfs_att.attr))
	{
		device_destroy(sysfs_class, MKDEV(major_number, 0));
		class_destroy(sysfs_class);
		unregister_chrdev(major_number, "Sysfs_Device");
		return -1;
	}
	
	if (device_create_file(sysfs_device, (const struct device_attribute *)&dev_attr_sysfs_att2.attr))
	{
		device_remove_file(sysfs_device, (const struct device_attribute *)&dev_attr_sysfs_att.attr);
		device_destroy(sysfs_class, MKDEV(major_number, 0));
		class_destroy(sysfs_class);
		unregister_chrdev(major_number, "Sysfs_Device");
		return -1;
	}
	
	return 0;
}

static void __exit sysfs_example_exit(void)
{
	device_remove_file(sysfs_device, (const struct device_attribute *)&dev_attr_sysfs_att2.attr);
	device_remove_file(sysfs_device, (const struct device_attribute *)&dev_attr_sysfs_att.attr);
	device_destroy(sysfs_class, MKDEV(major_number, 0));
	class_destroy(sysfs_class);
	unregister_chrdev(major_number, "Sysfs_Device");
}

module_init(sysfs_example_init);
module_exit(sysfs_example_exit);
