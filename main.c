#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include "client.h"
#include "ipv4.h"
#include "ipv6.h"


static void
setup_connection
(
 SOCKET sock,
 const char * const nickname
)
{
	char to_send_d[33];
	size_t to_send_s = strlen(nickname);
	if ( to_send_s > 32 )
		to_send_s = 32;
	*to_send_d = (unsigned char)to_send_s;
	memcpy(to_send_d+1, nickname, to_send_s);
	send(sock, to_send_d, to_send_s+1, 0);
}

int main
(
 int argc,
 char * * argv
)
{
	int rv;
	WSADATA wsa_data;
	
	if ( argc > 2 )
	{
		if ( WSAStartup(MAKEWORD(2,2), &wsa_data) == 0 )
		{
			char * const mode = argv[2];
			SOCKET s;
			
			if ( strcmp(mode, "ipv4") == 0 )
				s = connect_ipv4(argc-2, argv+2);
			else
			if ( strcmp(mode, "ipv6") == 0 )
				s = connect_ipv6(argc-2, argv+2);
			else
			{
				s = INVALID_SOCKET;
				fprintf(stderr, "error: connection mode '%s' unknown and not implemented\n", mode);
			}
			
			if ( s != INVALID_SOCKET )
			{
				setup_connection(s, argv[1]);
				rv = client(s);
			}
			else
				rv = 0;
			
			WSACleanup();
		}
		else
			rv = 0;
	}
	else
		rv = 0;
	
	return !rv;
}
