#include "ipv4.h"

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ip.h"


SOCKET connect_ipv4
(
 int argc,
 char * * argv
)
{
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = IPPROTO_TCP
	};
	return connect_ip(argc, argv, hints);
}
