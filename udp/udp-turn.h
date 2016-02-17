#ifndef _UDP_TURN_H
#define _UDP_TURN_H


typedef enum {
  NICE_TURN_SOCKET_COMPATIBILITY_DRAFT9,
  NICE_TURN_SOCKET_COMPATIBILITY_GOOGLE,
  NICE_TURN_SOCKET_COMPATIBILITY_MSN,
  NICE_TURN_SOCKET_COMPATIBILITY_OC2007,
  NICE_TURN_SOCKET_COMPATIBILITY_RFC5766,
} NiceTurnSocketCompatibility;

#include "socket.h"
#include "stun/stunmessage.h"


G_BEGIN_DECLS

guint
nice_udp_turn_socket_parse_recv_message (NiceSocket *sock, NiceSocket **from_sock,
    NiceInputMessage *message);

gsize
nice_udp_turn_socket_parse_recv (NiceSocket *sock, NiceSocket **from_sock,
    NiceAddress *from, gsize len, guint8 *buf,
    NiceAddress *recv_from, guint8 *recv_buf, gsize recv_len);

gboolean
nice_udp_turn_socket_set_peer (NiceSocket *sock, NiceAddress *peer);

NiceSocket *
nice_udp_turn_socket_new (GMainContext *ctx, NiceAddress *addr,
    NiceSocket *base_socket, NiceAddress *server_addr,
    gchar *username, gchar *password, NiceTurnSocketCompatibility compatibility);

void
nice_udp_turn_socket_set_ms_realm(NiceSocket *sock, StunMessage *msg);

void
nice_udp_turn_socket_set_ms_connection_id (NiceSocket *sock, StunMessage *msg);


G_END_DECLS

#endif /* _UDP_TURN_H */
