///@file port_socket.h
///@brief socket and internet defs and decls
///@details this socket interface should be built on FreeRTOS components and a
///internet middleware(e.g. uart parser for esp8266-1s peripheral). see @ref
///https://www.man7.org/linux/man-pages/man2/socket.2.html
#pragma once

#include <stdint.h>

#include "port_unistd.h"

///@defgroup port_sock_addr_def
///@brief internet address defined numbers
///@{

///@brief ipv4 address family
#define AF_INET 0

///@brief address struct
struct sockaddr {
  uint16_t sa_family;
  uint8_t sa_data[14];
};
///@brief length type of address len
typedef uint8_t socklen_t;

///@}

///@defgroup port_sock_ip
///@brief ip-based protocol types, etc
///@{

#define IPPROTO_TCP 0

///@}

///@defgroup port_sock_def
///@{

#define SOCK_STREAM 0

/// non-blocking send/recv
#define MSG_DONTWAIT 1 << 0
/// do not signal SIGPIPE when conn closes upon send/recv
#define MSG_NOSIGNAL 1 << 1

///@}

///@defgroup port_sock_api
///@brief berkerley standard socket api's
///@{

///@brief create socket endpoint
///@param domain see @ref port_sock_addr_def
///@param type see @ref port_sock_def
///@param protocol see @ref port_sock_ip
///@return listening sock file descriptor (sockfd); on error, returns -1 and
/// errno set
extern int port_socket(int domain, int type, int protocol);

///@brief bind listening port to socket
extern int port_bind(int sockfd, const struct sockaddr *addr,
                     socklen_t addrlen);

// port_connect for client side not impl

///@brief socket listen (after bind)
///@param backlog queue size for pending conns
int listen(int sockfd, int backlog);

///@brief accept a connection
///@param addr [out] socket address info to be filled
///@param addrlen [inout] set to actual size of addr length
///@return accepted socket file descriptor (sockfd), on error -1 and errno set
extern int accept(int sockfd, struct sockaddr *_Nullable restrict addr,
                  socklen_t *_Nullable restrict addrlen);

///@brief socket send()
///@param flags see @ref port_sock_def for MSG_
///@return number of bytes send, -1 on error
extern ssize_t send(int sockfd, const void *buff, size_t size, int flags);

///@brief socket recv()
///@details see @ref send
extern ssize_t recv(int sockfd, void *buff, size_t size, int flags);

///@}