#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/file.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/moduleparam.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/highmem.h>
#include <linux/ipc_namespace.h>
#include <asm/io.h>
#include <linux/ipc.h>
#include <linux/slab.h>
#include <linux/fsnotify.h>
#include <linux/fdtable.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roberto Palamaro");
MODULE_DESCRIPTION("OS2 thesis");

#define __NR_SYS_OPEN   2
//#define MAX_SESSION		3
#define BUFFER_SIZE		2048

static int MAX_SESSION = -1;
module_param(MAX_SESSION,int,0644);
MODULE_PARM_DESC(MAX_SESSION, "The number of parallel session on the same file.");


wait_queue_head_t queue_for_session;

atomic64_t current_sessions;

extern void *sys_call_table[];

int *pid_to_session;
char **buffer_for_session;

struct semaphore mutex_to_acquire_a_session;
struct semaphore mutex_to_write;
struct semaphore sem_to_session;


asmlinkage long (*original_sys_open)(const char __user*,int, umode_t);

ssize_t (*original_write_routine)(struct file *, const char *, size_t, loff_t *);


ssize_t my_read_routine (struct file *file, char * buf, size_t size, loff_t * offset){

	int i,ret,my_session=-1;
	mm_segment_t fs=get_fs();
	set_fs(KERNEL_DS);
	printk("This is read routine for pid %d \n",current->pid);
	for (i=0;i<MAX_SESSION;i++){
		if((int)pid_to_session[i]==current->pid){
			printk("Ok..Pid:%d Session:%d \n",current->pid,i);
			my_session = i;
			break;
		}
	}
	if (my_session<0){
		printk(KERN_ERR"Error with session \n");
		goto error;
	}
	printk ("Reading the buffer \n");
	if (size > BUFFER_SIZE){
		printk(KERN_WARNING"Size is > BUFFER SIZE return %d bytes",BUFFER_SIZE);
		ret = copy_to_user(buf,buffer_for_session[my_session]+*offset,BUFFER_SIZE);
	}else{
		ret = copy_to_user(buf,buffer_for_session[my_session]+*offset,size);
	}
	if (ret == 0){
		printk("Readed all what you request \n");
	}
	set_fs(fs);
	return (ssize_t)size-ret;
		
error:
	set_fs(fs);
	return -1;

}

int my_flush_routine (struct file *file,fl_owner_t id){

	int i,my_session=-1;
	ssize_t len;
	loff_t of = 0;
	mm_segment_t fs=get_fs();
	char *outputBuffer = kmalloc(BUFFER_SIZE*sizeof(char),GFP_USER);
	set_fs(KERNEL_DS);
	printk("This is flush routine for pid %d \n",current->pid);	
	for (i=0;i<MAX_SESSION;i++){
		if((int)pid_to_session[i]==current->pid){
			printk("Ok..Pid:%d Session:%d \n",current->pid,i);
			my_session = i;
			break;
		}
	}
	if (my_session<0){
		goto error;
	}
	copy_to_user(outputBuffer,buffer_for_session[my_session],BUFFER_SIZE);
	printk("Acquiring sem to write on file \n");
	down_interruptible(&mutex_to_write);
	len = do_sync_write(file,outputBuffer,strlen(outputBuffer),&of);
	if (len <0){
		printk("sync_write_failed \n");
	}
	up(&mutex_to_write);
	atomic64_dec(&current_sessions);
	pid_to_session[my_session] = 0;
	wake_up_interruptible(&queue_for_session);
	printk("Writed success. on file..writed %d bytes \n",(int)len);
	set_fs(fs);
	up(&sem_to_session);
	kfree(outputBuffer);
	return 0;
error:
	set_fs(fs);
	kfree(outputBuffer);
	return -1;
}

ssize_t my_write_routine (struct file *file, const char *buff, size_t size, loff_t *offset)
{
	int i,ret,my_session=-1;
	char *my_buff_temp = (char*)kmalloc(BUFFER_SIZE*sizeof(char),GFP_KERNEL);
	mm_segment_t fs=get_fs();
	set_fs(KERNEL_DS);
	printk("This is write routine for pid %d \n",current->pid);
	for (i=0;i<MAX_SESSION;i++){
		if((int)pid_to_session[i]==current->pid){
			printk("Ok..Pid:%d Session:%d \n",current->pid,i);
			my_session = i;
			break;
		}
	}
	if (my_session<0){
		goto error;
	}
	memcpy(my_buff_temp,buffer_for_session[my_session],BUFFER_SIZE);	
	if (size > BUFFER_SIZE){
		printk(KERN_ERR"Error buffer space is only we truncate %d \n",BUFFER_SIZE);
		ret = copy_from_user(my_buff_temp+*offset,buff,BUFFER_SIZE);
	}
	else{
		ret = copy_from_user(my_buff_temp+*offset,buff,size);
	}
	if (ret == 0){
		printk("Writed all what you request in the limit of buffer \n");
	}
	printk("This is the buffer temp %s \n",my_buff_temp);
	memcpy(buffer_for_session[my_session],my_buff_temp,BUFFER_SIZE);
	set_fs(fs);
	kfree(my_buff_temp);
	return (ssize_t)size-ret;
	
error:
	set_fs(fs);
	kfree(my_buff_temp);
	return -1;
}

static struct file_operations f_ops ={
	.write = my_write_routine,
	.read = my_read_routine,
	.flush = my_flush_routine
};


asmlinkage long open_with_session(const char* buff,int flag, umode_t mode)
{
	int flag_without_session,fd,i;
	struct file *file;
	mm_segment_t fs=get_fs();
	set_fs(KERNEL_DS);
	if ((flag & 1 <<26)!=0){
		// SESSION CASE
			/*
			if (atomic64_read(&current_sessions)<MAX_SESSION){
				atomic64_inc(&current_sessions);
				printk ("Hi..%d..Session available..subscribe to session \n",current->pid);
				down_interruptible(&mutex_to_acquire_a_session);
				for(i=0;i<MAX_SESSION;i++){
					if (pid_to_session[i]==0){
						printk("My session is:%d \n",i);
						pid_to_session[i]=current->pid;
						break;
					}
				}
				up(&mutex_to_acquire_a_session);
				flag_without_session = flag &(~(1<<26));
				fd = original_sys_open(buff,flag_without_session,mode);
				if(fd<0){
					printk("Error while using open..fd<0 \n");
					goto error;
				}
				file = fget(fd);
				f_ops.aio_write = file->f_op->aio_write;
				file->f_op = &f_ops;
				set_fs(fs);
				return fd;
			}else{
				printk("Currently all the sessions are busy..sleep \n");
				interruptible_sleep_on(&queue_for_session);
			}
			*/
		/*
		if (atomic64_read(&current_sessions) == MAX_SESSION){
			printk("Currently all the sessions are busy..sleep \n");
			interruptible_sleep_on(&queue_for_session);
		}
		*/
		//atomic64_inc(&current_sessions);
		down_interruptible(&sem_to_session);
		printk ("Hi..%d..Session available..subscribe to session \n",current->pid);
		down_interruptible(&mutex_to_acquire_a_session);
		for(i=0;i<MAX_SESSION;i++){
			if (pid_to_session[i]==0){
				printk("My session is:%d \n",i);
				pid_to_session[i]=current->pid;
				break;
			}
		}
		up(&mutex_to_acquire_a_session);
		flag_without_session = flag &(~(1<<26));
		fd = original_sys_open(buff,flag_without_session,mode);
		if(fd<0){
			printk("Error while using open..fd<0 \n");
			goto error;
		}
		file = fget(fd);
		f_ops.aio_write = file->f_op->aio_write;
		file->f_op = &f_ops;
		set_fs(fs);
		return fd;		
	}else{
		return original_sys_open(buff,flag,mode);
	}
error:
	set_fs(fs);
	return -1;
}
static void disable_page_protection(void) 
{
  unsigned long value;
  asm volatile("mov %%cr0, %0" : "=r" (value));

  if(!(value & 0x00010000))
    return;

  asm volatile("mov %0, %%cr0" : : "r" (value & ~0x00010000));
}

static void enable_page_protection(void) 
{
  unsigned long value;
  asm volatile("mov %%cr0, %0" : "=r" (value));

  if((value & 0x00010000))
    return;

  asm volatile("mov %0, %%cr0" : : "r" (value | 0x00010000));
}

static int mod_init(void)
{
	int i;
	printk("Backup previuous sys_open\n");
	original_sys_open=sys_call_table[__NR_SYS_OPEN];
	disable_page_protection();
	sys_call_table[__NR_SYS_OPEN] = open_with_session;
	enable_page_protection();
	printk("Init all control structures \n");
	buffer_for_session = kmalloc(MAX_SESSION*sizeof(buffer_for_session),GFP_KERNEL);
	pid_to_session = kmalloc(MAX_SESSION*sizeof(int),GFP_KERNEL);
	for (i=0;i<MAX_SESSION;i++){
		buffer_for_session[i] = kmalloc(BUFFER_SIZE*sizeof(char),GFP_KERNEL);
		pid_to_session[i]=0;
	}
	init_waitqueue_head(&queue_for_session);
	atomic64_set(&current_sessions,0);
	printk("Done.Now we can use semantic session on file with %d session \n",MAX_SESSION);
	sema_init(&mutex_to_acquire_a_session,1);
	sema_init(&mutex_to_write,1);
	sema_init(&sem_to_session,MAX_SESSION);
	return 0;
}


static void mod_exit(void)
{
	printk("Exiting module...restore old open\n");
	disable_page_protection();
	sys_call_table[__NR_SYS_OPEN]=original_sys_open;
	enable_page_protection();
	kfree(buffer_for_session);
	kfree(pid_to_session);
	printk("All done.\n");
}

module_init(mod_init);
module_exit(mod_exit);
