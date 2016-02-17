#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/module.h>

static struct task_struct *_mythread = NULL;

static int _thread_func(void *data)
{
	printk("thread start\n");
	while( !kthread_should_stop() )
	{
		printk("hello, world. HZ=%d, %lu\n", HZ, jiffies);
		ssleep(1);
	}

	printk("thread exit\n");
	return 0;
}

static int __init _test_init(void)
{
	_mythread = kthread_run(_thread_func, NULL, "_mythread");
	printk("init over\n");
	return 0;
}

static void __exit _test_exit(void)
{
	if( _mythread )
	{
		printk("stop\n");
		kthread_stop(_mythread);
		_mythread = NULL;
	}
	
	printk("exit over\n");
}

module_init(_test_init);
module_exit(_test_exit);
