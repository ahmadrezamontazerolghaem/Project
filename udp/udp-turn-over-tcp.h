#ifndef _UDP_TURN_OVER_TCP_H
#define _UDP_TURN_OVER_TCP_H

#include "socket.h"
#include "agent.h"

G_BEGIN_DECLS

 
NiceSocket *
nice_udp_turn_over_tcp_socket_new (NiceSocket *base_socket,
    NiceTurnSocketCompatibility compatibility);


G_END_DECLS

#endif /* _UDP_TURN_OVER_TCP_H */
