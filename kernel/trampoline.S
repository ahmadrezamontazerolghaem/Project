/*  plain sigreturn is gone.  */

	.globl __rt_sigtramp_template
__rt_sigtramp_template:
	r6 = #__NR_rt_sigreturn;
	trap0(#1);
