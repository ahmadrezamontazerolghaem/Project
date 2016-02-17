#include "omni_socket.h"

#include <linux/module.h>
#include <linux/string.h>
#include <linux/in.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>

#include <net/sock.h>
#ifndef _MAX_USER_
#define _MAX_USER_ 5
#endif

typedef unsigned int socklen_t;
/* ********************************************* */
/* local funciton implemention */
static inline int _omni_sock_bind_localaddr(struct socket *sock, const char *ipaddr, const int port);
static int _omni_sock_create_multicast_reader(struct socket **sock, const char *mc_ipaddr, const int mc_port, const char *local_if, const char *source_addr);

static int _omni_sock_setsockopt(struct socket *sock, int level, int optname, char *optval, unsigned int optlen);
static int _omni_sock_getsockopt(struct socket *sock, int level, int optname, char *optval, int *optlen);

/* ********************************************* */
/*  golabl function */
int sock_create_unix(struct socket **sock)
{
	return sock_create(AF_UNIX, SOCK_DGRAM, 0, sock);
}

int sock_create_tcp(struct socket **sock)
{
	return sock_create(AF_INET, SOCK_STREAM, 0, sock);
}

int sock_create_udp(struct socket **sock)
{
	return sock_create(AF_INET, SOCK_DGRAM, 0, sock);
}

int sock_make_tcpserver(struct socket *sock, const char *srv_ip, const int srv_port, const int maxuser)
{
	/* set add reuse */
	int reuse = 1;
	if( -1 == _omni_sock_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) )
		return -1;

	/* bind */
	if( _omni_sock_bind_localaddr(sock, srv_ip, srv_port) )
		return -1;

	/* listen */
	if(-1 == sock->ops->listen(sock, maxuser) )
		return -1;

	/* now, the server can wait client to connect, use: accept */
	return 0;
}

int sock_create_tcpserver(struct socket **sock, const char *server_ipaddr, const int server_port)
{
	if( -1 == sock_create_tcp(sock) )
		return -1;

	if( -1 == sock_make_tcpserver(*sock, server_ipaddr, server_port, _MAX_USER_) )
	{
		sock_release(*sock);
		return -1;
	}

	return 0;
}

int sock_bind_to_device(struct socket *sock, const char *interface)
{
	struct ifreq ifr;
	if( interface == NULL )
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, interface, IFNAMSIZ);
	if( -1 == _omni_sock_setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char*)&ifr, sizeof(ifr)) )
		return -1;

	return 0;
}

int sock_make_udpserver(struct socket *sock, const char *ipaddr, const int port)
{
	/* just bind */
	if( _omni_sock_bind_localaddr(sock, ipaddr, port) )
		return -1;
	return 0;
}

int sock_create_udpserver(struct socket **sock, const char *server_ipaddr, const int server_port)
{
	if( -1 == sock_create_udp(sock) )
		return -1;
	if(-1 == sock_make_udpserver(*sock, server_ipaddr, server_port) )
	{
		sock_release(*sock);
		return -1;
	}
	return 0;
}

int sock_set_ttl(struct socket *sock, int ttl, int *old_ttl)
{
	int _oldttl = 0;
	int oldlen = sizeof(int);

	/* get ttl */
	if( -1 == _omni_sock_getsockopt(sock, IPPROTO_IP, IP_TTL, (char*)&_oldttl, (socklen_t*)&oldlen) )
	{
		return -1;
	}

	if( ttl == _oldttl )
	{
		return 0;
	}
	
	/* set ttl */
	if( -1 == _omni_sock_setsockopt(sock, IPPROTO_IP, IP_TTL, (char*)&ttl, sizeof(ttl)) )
	{
		return -1;
	}

	if( old_ttl )
	{
		*old_ttl = _oldttl;
	}

	return 0;
}

int sock_set_multicast_ttl(struct socket *sock, int ttl, int *old_ttl)
{
	int _oldttl = 0;
	int oldlen = sizeof(int);

	/* get ttl */
	if( -1 == _omni_sock_getsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&_oldttl, (socklen_t*)&oldlen) )
		return -1;

	if( ttl == _oldttl )
	{
		return 0;
	}
	
	/* set ttl */
	if( -1 == _omni_sock_setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl)) )
		return -1;

	if( old_ttl )
	{
		*old_ttl = _oldttl;
	}

	return 0;
}

int sock_create_multicast_writer(struct socket **sock, const char *local_if, const char *local_ip, const int local_port)
{
	if( -1 ==  sock_create_udp(sock) )
		return -1;

	if( local_if )
	{
		if( -1 == sock_bind_to_device(*sock, local_if) )
			goto err_out;
	}

	if( -1 == _omni_sock_bind_localaddr(*sock, local_ip, local_port) )
		goto err_out;

	return 0;

err_out:
	sock_release(*sock);
	return -1;
}

inline int sock_create_multicast_reader(struct socket **sock, const char *mc_ipaddr, const int mc_port, const char *local_if)
{
	return _omni_sock_create_multicast_reader(sock, mc_ipaddr, mc_port, local_if, NULL);
}

inline int sock_create_srcmulticast_writer(struct socket **sock, const char *local_if, const char *local_ip, const int local_port)
{
	return sock_create_multicast_writer(sock, local_if, local_ip, local_port);
}

inline int sock_create_srcmulticast_reader(struct socket **sock, const char *mc_ipaddr, const int mc_port, const char *local_if, const char *src_ipaddr)
{
	return _omni_sock_create_multicast_reader(sock, mc_ipaddr, mc_port, local_if, src_ipaddr);
}

inline int sock_tcpcli_connect_srv(struct socket *sock, const char *server_ipaddr, const int server_port)
{
	struct sockaddr_in localaddr;
	sock_make_sockaddr_in(server_ipaddr, server_port, &localaddr);

	if( -1 == sock->ops->connect(sock, (struct sockaddr*)&localaddr, sizeof(localaddr), 0) )
	{
		return -1;
	}

	return 0;
}

inline int sock_udpcli_connect_srv(struct socket *sock, const char *server_ipaddr, const int server_port)
{
	/* sometimes udp client will call connect with server,
	 * then it can just use read/write to exchange message with server
	 * */
	return sock_tcpcli_connect_srv(sock, server_ipaddr, server_port);
}

int sock_tcpsrv_waitfor_cli(struct socket *server_sock, struct socket **client_sock, int flags)
{
	struct sock *sk = server_sock->sk;
	int err;

	err = sock_create_lite(sk->sk_family, sk->sk_type, sk->sk_protocol, client_sock);
	if (err < 0)
		goto done;

	err = server_sock->ops->accept(server_sock, *client_sock, flags);
	if (err < 0)
	{
		sock_release(*client_sock);
		*client_sock= NULL;
		goto done;
	}

	(*client_sock)->ops = server_sock->ops;
	__module_get((*client_sock)->ops->owner);

	done:
	return err;
}

inline void sock_make_sockaddr_in(const char *ipaddr, const int port, struct sockaddr_in *sockaddr_out)
{
	memset(sockaddr_out, 0, sizeof(struct sockaddr_in) );
	sockaddr_out->sin_family = AF_INET;
	sockaddr_out->sin_port = htons(port);
	if( ipaddr )
		sockaddr_out->sin_addr.s_addr = in_aton(ipaddr);
	else
		sockaddr_out->sin_addr.s_addr = htonl(INADDR_ANY);
}

int sock_write(struct socket *sock, void *buf, int len, struct sockaddr_un*sockaddr)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL)
		return 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = sockaddr;
	msg.msg_namelen  = sizeof(struct sockaddr_un);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_sendmsg(sock,&msg,len);
	set_fs(oldfs);

	return size;
}

int sock_read(struct socket *sock, void *buf, int len, struct  sockaddr *sockaddr)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int size = 0;

	if (sock->sk==NULL)
		return 0;

	iov.iov_base = buf;
	iov.iov_len = len;

	msg.msg_flags = 0;
	msg.msg_name = sockaddr;
	msg.msg_namelen  = sizeof(struct sockaddr);
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	size = sock_recvmsg(sock,&msg,len,msg.msg_flags);
	set_fs(oldfs);

	return size;
}

/* ********************************************* */
/* local funciton implemention */
static inline int _omni_sock_bind_localaddr(struct socket *sock, const char *ipaddr, const int port)
{
	struct sockaddr_in localaddr;

	sock_make_sockaddr_in(ipaddr, port, &localaddr);

	if( -1 == sock->ops->bind(sock, (struct sockaddr*)&localaddr, sizeof(localaddr)) )
	{
		return -1;
	}

	return 0;
}

static int _omni_sock_create_multicast_reader(struct socket **sock, const char *mc_ipaddr, const int mc_port, const char *local_if, const char *source_addr)
{
	struct sockaddr_in *psockaddr = NULL, sockaddr;
	int ret = -1;

	ret = sock_create_udp(sock);
	if( ret == -1 )
		return -1;

	/* bind with local interface, and get local interface ipaddr */
	if( local_if )
	{
		/* get ipaddress by interface */
		//psockaddr = (struct sockaddr_in *) &(ifr.ifr_ifru.ifru_addr);
		struct net_device *netdev = NULL;
		struct in_device *pdev_ipaddr = NULL;
#if 0
		netdev = dev_get_by_name(local_if);
#else
		/* version > 2.6.24 */
		netdev = dev_get_by_name(&init_net, local_if);
#endif
		pdev_ipaddr = (struct in_device *)netdev->ip_ptr;
		sockaddr.sin_addr.s_addr = pdev_ipaddr->ifa_list->ifa_local;
		psockaddr = &sockaddr;
	//	printk("==== %s : 0x%x\n", local_if, pdev_ipaddr->ifa_list->ifa_local);
	}

	/* set loop back enabale */
	{
		int loop = 1;
		if( -1 == _omni_sock_setsockopt(*sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loop, sizeof(loop) ) )
		{
			goto err_out;
		}
	}

	/* join in member*/
	if( !source_addr )
	{
		/* join in membership */
		struct ip_mreq mreq;
		memset(&mreq, 0, sizeof(struct ip_mreq));
		mreq.imr_multiaddr.s_addr =  in_aton(mc_ipaddr);
		if( psockaddr )
			mreq.imr_interface.s_addr = psockaddr->sin_addr.s_addr;
		else
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		if( -1 == _omni_sock_setsockopt(*sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) )
		{
			goto err_out;
		}
	}else
	{
		/* join in source_membership */
		struct ip_mreq_source mc_req;
		memset(&mc_req, 0, sizeof(struct ip_mreq_source));
		mc_req.imr_multiaddr =  in_aton(mc_ipaddr);
		if( psockaddr )
			mc_req.imr_interface = psockaddr->sin_addr.s_addr;
		else
			mc_req.imr_interface = htonl(INADDR_ANY);

		mc_req.imr_sourceaddr = in_aton(source_addr);
		if( -1 == _omni_sock_setsockopt(*sock, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *)&mc_req, sizeof(mc_req)) )
		{
			goto err_out;
		}
	}

	/* bind with multicast ip */
	if( -1 == _omni_sock_bind_localaddr(*sock, mc_ipaddr, mc_port) )
	{
		goto err_out;
	}

	return 0;
err_out:
	sock_release(*sock);
	return -1;
}

static int _omni_sock_setsockopt(struct socket *sock, int level, int optname, char *optval, unsigned int optlen)
{
	mm_segment_t oldfs = get_fs();
	char __user *uoptval;
	int err;

	uoptval = (char __user __force *) optval;

	set_fs(KERNEL_DS);
	if (level == SOL_SOCKET)
		err = sock_setsockopt(sock, level, optname, uoptval, optlen);
	else
		err = sock->ops->setsockopt(sock, level, optname, uoptval, optlen);
	set_fs(oldfs);
	return err;
}

static int _omni_sock_getsockopt( struct socket *sock, int level, int optname, char *optval, int *optlen)
{
	mm_segment_t oldfs = get_fs();
	char __user *uoptval;
	int __user *uoptlen;
	int err;

	uoptval = (char __user __force *) optval;
	uoptlen = (int __user __force *) optlen;

	set_fs(KERNEL_DS);
	if (level == SOL_SOCKET)
		err = sock_getsockopt(sock, level, optname, uoptval, uoptlen);
	else
		err = sock->ops->getsockopt(sock, level, optname, uoptval, uoptlen);
	set_fs(oldfs);
	return err;
}

EXPORT_SYMBOL(sock_create_tcp);
EXPORT_SYMBOL(sock_create_udp);
EXPORT_SYMBOL(sock_create_tcpserver);
EXPORT_SYMBOL(sock_create_udpserver);
EXPORT_SYMBOL(sock_create_multicast_writer);
EXPORT_SYMBOL(sock_create_multicast_reader);
EXPORT_SYMBOL(sock_create_srcmulticast_writer);
EXPORT_SYMBOL(sock_create_srcmulticast_reader);
EXPORT_SYMBOL(sock_make_sockaddr_in);
EXPORT_SYMBOL(sock_read);
EXPORT_SYMBOL(sock_write);
EXPORT_SYMBOL(sock_tcpsrv_waitfor_cli);
EXPORT_SYMBOL(sock_tcpcli_connect_srv);
EXPORT_SYMBOL(sock_udpcli_connect_srv);
