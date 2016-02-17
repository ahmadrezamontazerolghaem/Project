#ifndef _TCP_BSD_H
#define _TCP_BSD_H

#include "socket.h"

G_BEGIN_DECLS

NiceSocket *
nice_tcp_bsd_socket_new (GMainContext *ctx, NiceAddress *remote_addr,
    NiceAddress *local_addr, gboolean reliable);

NiceSocket *
nice_tcp_bsd_socket_new_from_gsock (GMainContext *ctx, GSocket *gsock,
    NiceAddress *remote_addr, NiceAddress *local_addr, gboolean reliable);

G_END_DECLS

#endif /* _TCP_BSD_H */
