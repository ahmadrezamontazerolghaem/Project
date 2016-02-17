#ifndef _TCP_PASSIVE_H
#define _TCP_PASSIVE_H

#include "socket.h"

G_BEGIN_DECLS


NiceSocket * nice_tcp_passive_socket_new (GMainContext *ctx, NiceAddress *addr);
NiceSocket * nice_tcp_passive_socket_accept (NiceSocket *socket);


G_END_DECLS

#endif /* _TCP_PASSIVE_H */
