#include "client.h"

#include <string.h>
#include <stdio.h>
#include <winsock2.h>


static int
receive_all
(
 SOCKET sock,
 char * buf,
 int len
)
{
	int received = 0;
	int rv;
	
recv_again:
	rv = recv(sock, buf, len, 0);
	if ( rv != SOCKET_ERROR )
	{
		received += rv;
		if ( received < len )
			goto recv_again;
		else
			return 1;
	}
	else
		return 0;
}

enum Phase {
	phase_get_nickname_length,
	phase_get_nickname,
	phase_get_message_length,
	phase_get_message
};

DWORD WINAPI
receive_thread
(
 LPVOID data
)
{
	SOCKET sock = *(SOCKET *)data;
	enum Phase phase = phase_get_nickname_length;
	unsigned char nickname_length = 0;
	char nickname[32];
	unsigned char message_length = 0;
	char message[256];
	FILE * log_file = fopen("log.txt", "a");
	
	for ( ; ; )
	{
		switch ( phase )
		{
			case phase_get_nickname_length:
				if ( recv(sock, &nickname_length, 1, 0) )
				{
					phase = phase_get_nickname;
					break;
				}
				else
					return 0;
			case phase_get_nickname:
				if ( receive_all(sock, nickname, nickname_length) )
				{
					phase = phase_get_message_length;
					break;
				}
				else
					goto exit_point;
			case phase_get_message_length:
				if ( recv(sock, &message_length, 1, 0) )
				{
					phase = phase_get_message;
					break;
				}
				else
					goto exit_point;
			case phase_get_message:
				if ( receive_all(sock, message, message_length) )
				{
					printf("%.*s wrote: %.*s\n", nickname_length, nickname, message_length, message);
					if ( log_file )
						fprintf(log_file, "%.*s wrote: %.*s\n", nickname_length, nickname, message_length, message);
						fflush(log_file);
					
					phase = phase_get_nickname_length;
					break;
				}
				else
					goto exit_point;
		}
	}
	
	exit_point:
	if ( log_file )
		fclose(log_file);
	return 0;
}

int client
(
 SOCKET sock
)
{
	int rv;
	HANDLE thread_handle;
	if ( thread_handle = CreateThread(NULL, 0, receive_thread, &sock, 0, NULL) )
	{
		for ( ; ; )
		{
			char message[256];
			
			puts("Enter a message:");
			fgets(message+1, sizeof(message)-1, stdin);
			if ( message[1] == '\n' )
				break;
			else
			{
				size_t msg_len = strlen(message+1);
				message[msg_len] = 0;
				*message = msg_len-1;
				send(sock, message, msg_len, 0);
			}
		}
		
		rv = 1;
	}
	else
		rv = 0;
	
	closesocket(sock);
	
	if ( thread_handle )
	{
		WaitForSingleObject(thread_handle, INFINITE);
		CloseHandle(thread_handle);
	}
	
	return rv;
}
