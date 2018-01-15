#include "ip.h"

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>


SOCKET connect_ip
(
 int argc,
 char * * argv,
 struct addrinfo hints
)
{
	if ( argc == 3 )
	{
		char * const ip_address = argv[1];
		char * const port = argv[2];
		struct addrinfo * result;
		int gai_rv;
		
		if ( (gai_rv = getaddrinfo(ip_address, port, &hints, &result)) == 0 )
		{
			SOCKET s = INVALID_SOCKET;
			for ( ; result != NULL ; result = result->ai_next )
			{
				if ( (s = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) != INVALID_SOCKET )
				{
					fputs("trying to connect... ", stdout);
					if ( connect(s, result->ai_addr, result->ai_addrlen) != SOCKET_ERROR )
					{
						puts("server accepted connection");
						break;
					}
					else
					{
						puts("connection attempt failed or rejected");
						closesocket(s);
						s = INVALID_SOCKET;
					}
				}
				else
					puts("couldn't create socket");
			}
			
			return s;
		}
		else
			fprintf(stderr, "getaddrinfo failed %d\n", gai_rv);
	}
	else
		puts("missing IP address and/or port");
	
	return INVALID_SOCKET;
}
