#ifndef _UDP_BSD_H
#define _UDP_BSD_H

#include "socket.h"

G_BEGIN_DECLS

NiceSocket *
nice_udp_bsd_socket_new (NiceAddress *addr);

G_END_DECLS

#endif /* _UDP_BSD_H */
