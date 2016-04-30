#include <linux/module.h>
#include <linux/kernel.h>
#include "omni_socket.h" 

struct socket *unsock;
struct sockaddr_un dstaddr;

static int __init _unsock_test_init(void)
{
	char buf[] = "hello,world! I am in Kernel\n";
	int len = strlen(buf)+1;
	int ret = 0;
	ret = sock_create_unix(&unsock);
	memset(&dstaddr, 0, sizeof dstaddr);
	dstaddr.sun_family = AF_LOCAL;
	strncpy(dstaddr.sun_path, " localSocketServer", sizeof(dstaddr.sun_path) - 1);
	dstaddr.sun_path[0] = 0;

	printk("sock create succes\n");

	ret = sock_write(unsock, buf, len, &dstaddr);
	printk("write : ret =%d\n", ret);
	sock_release(unsock);
	unsock = NULL;
	return 0;
}

static void __exit _unsock_test_exit(void)
{
	if( unsock )
	{
		sock_release(unsock);
	}
}

module_init(_unsock_test_init);
module_exit(_unsock_test_exit);
