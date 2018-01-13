#include "client.h"

#include <string.h>
#include <stdio.h>
#include <winsock2.h>


enum Phase {
	phase_getting_nickname_length,
	phase_getting_nickname,
	phase_getting_message_length,
	phase_getting_message
};

typedef struct
{
	SOCKET socket;
} ConnectionData;

typedef struct
{
	WSAOVERLAPPED wsa_overlapped;
	enum Phase phase;
	unsigned char received;
	unsigned char nickname_length;
	char nickname[32];
	unsigned char message_length;
	char message[256];
} OperationData;

DWORD WINAPI
worker_thread
(
 LPVOID data
)
{
	HANDLE completion_port = (HANDLE)data;
	DWORD flags = 0;
	
	for ( ; ; )
	{
		DWORD size;
		ConnectionData * connection_data;
		OperationData * operation_data;
		if ( GetQueuedCompletionStatus(completion_port, &size, (PULONG_PTR)&connection_data, (LPOVERLAPPED *)&operation_data, INFINITE) )
		{
			if ( size )
			{
				switch ( operation_data->phase )
				{
					case phase_getting_nickname_length:
					{
						//printf("nickname length: %i\n", operation_data->nickname_length);
						
						operation_data->phase = phase_getting_nickname;
						
						operation_data->received = 0;
						
						WSABUF buffer_info = {
							.buf = operation_data->nickname,
							.len = operation_data->nickname_length
						};
						WSARecv(connection_data->socket, &buffer_info, 1, NULL, &flags, &(operation_data->wsa_overlapped), NULL);
						
						break;
					}
					
					case phase_getting_nickname:
					{
						operation_data->received += size;
						
						if ( operation_data->received == operation_data->nickname_length )
						{
							operation_data->phase = phase_getting_message_length;
							
							WSABUF buffer_info = {
								.buf = &(operation_data->message_length),
								.len = 1
							};
							WSARecv(connection_data->socket, &buffer_info, 1, NULL, &flags, &(operation_data->wsa_overlapped), NULL);
						}
						else
						{
							WSABUF buffer_info = {
								.buf = operation_data->nickname + operation_data->received,
								.len = operation_data->nickname_length - operation_data->received
							};
							WSARecv(connection_data->socket, &buffer_info, 1, NULL, &flags, &(operation_data->wsa_overlapped), NULL);
						}
						
						break;
					}
					
					case phase_getting_message_length:
					{
						//printf("message length: %i\n", operation_data->message_length);
						
						operation_data->phase = phase_getting_message;
						
						operation_data->received = 0;
						
						WSABUF buffer_info = {
							.buf = operation_data->message,
							.len = operation_data->message_length
						};
						WSARecv(connection_data->socket, &buffer_info, 1, NULL, &flags, &(operation_data->wsa_overlapped), NULL);
						
						break;
					}
					
					case phase_getting_message:
					{
						operation_data->received += size;
						
						if ( operation_data->received == operation_data->message_length )
						{
							printf("%.*s wrote: %.*s\n", operation_data->nickname_length, operation_data->nickname, operation_data->message_length, operation_data->message);
							
							operation_data->phase = phase_getting_nickname_length;
							
							WSABUF buffer_info = {
								.buf = &(operation_data->nickname_length),
								.len = 1
							};
							WSARecv(connection_data->socket, &buffer_info, 1, NULL, &flags, &(operation_data->wsa_overlapped), NULL);
						}
						else
						{
							WSABUF buffer_info = {
								.buf = operation_data->message + operation_data->received,
								.len = operation_data->message_length - operation_data->received
							};
							WSARecv(connection_data->socket, &buffer_info, 1, NULL, &flags, &(operation_data->wsa_overlapped), NULL);
						}
						
						break;
					}
				}
			}
		}
		else
		{
			DWORD error_code = GetLastError();
			if ( error_code == ERROR_ABANDONED_WAIT_0 )
				return EXIT_SUCCESS;
			else
				//win_perror("couldn't retrieve completion packet from the completion port queue", error_code);
				printf("couldn't retrieve completion packet from the completion port queue: %i\n", error_code);
		}
	}
}

int client
(
 SOCKET sock
)
{
	int rv;
	HANDLE completion_port;
	
	completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if ( completion_port )
	{
		HANDLE thread;
		if ( thread = CreateThread(NULL, 0, worker_thread, completion_port, 0, NULL) )
		{
			ConnectionData connection_data;
			
			CloseHandle(thread);
			
			connection_data.socket = sock;
			
			if ( CreateIoCompletionPort((HANDLE)sock, completion_port, (ULONG_PTR)&connection_data, 0) )
			{
				OperationData operation_data = {0};
				DWORD flags = 0;
				
				operation_data.phase = phase_getting_nickname_length;
				
				WSABUF buffer_info = {
					.buf = &(operation_data.nickname_length),
					.len = 1
				};
				/* Post initial recv - FIXME: Use ConnectEx */
				if ( WSARecv(sock, &buffer_info, 1, NULL, &flags, &(operation_data.wsa_overlapped), NULL) == SOCKET_ERROR )
				{
					if ( WSAGetLastError() != ERROR_IO_PENDING )
						printf("%d\n", WSAGetLastError());
				}
				
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
			}
		}
		
		CloseHandle(completion_port);
		
		rv = 1;
	}
	else
	{
		rv = 0;
	}
	
	closesocket(sock);
	
	return rv;
}
