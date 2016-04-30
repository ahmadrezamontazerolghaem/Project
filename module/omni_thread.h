#ifndef _OMNI_THREAD_H_
#define _OMNI_THREAD_H_

#include <linux/kthread.h>

typedef struct task_struct* omni_thread_t;

static inline omni_thread_t omni_thread_create(
		int(*func)(void*), void* param, const char *name)
{
	return kthread_run(func, param, name);
}

static inline void omni_thread_stop(omni_thread_t _thread)
{ 
	if( _thread )
	{
		/* send kill */
#if 0
		/* 3.6.6 kernel */
		kernel(find_vpid(_thread->pid), SIGKILL, 0);
#elif 1
		/* 3.2.0 kernel */
		kill_pid(get_pid(task_pid(_thread)), SIGKILL, 0);
#endif

		/* stop */
		if( _thread )
			kthread_stop(_thread);
	}
}

static inline void omni_thread_pre_init(void)
{
	allow_signal(SIGINT);
	allow_signal(SIGTERM);
	allow_signal(SIGKILL);

	set_current_state(TASK_INTERRUPTIBLE);
}

#define omni_thread_while(condition) \
	while( ( !kthread_should_stop() && \
			 !signal_pending(current) ) && \
				(condition) )

#define omni_thread_for(a_init, condition, a_after) \
	for( a_init; \
			( !kthread_should_stop() && \
				   !signal_pending(current) ) && \
					(condition) ; \
			a_after )

#endif
