#ifndef _TCP_ACTIVE_H
#define _TCP_ACTIVE_H

#include "socket.h"
 
G_BEGIN_DECLS
 

NiceSocket * nice_tcp_active_socket_new (GMainContext *ctx, NiceAddress *addr);
NiceSocket * nice_tcp_active_socket_connect (NiceSocket *socket, NiceAddress *addr);


G_END_DECLS

#endif /* _TCP_ACTIVE_H */
