#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_SERVER_PORT 80

int main(int argc, char** argv)
{
	// First parameter is the server IP:
	if (argc < 2)
	{
		printf("First parameter must be the server IP address\n");
		return -1;
	}

	const char* server_addr_str = argv[1];

	// Second parameter can be the server port:
	int server_port;

	if (argc > 2)
	{
		// TODO: Better parsing!
		server_port = atol(argv[2]);
	}
	else
	{
		server_port = DEFAULT_SERVER_PORT;
	}

	// Third parameter can be the client port:
	int client_port;

	if (argc > 3)
	{
		// TODO: ...
		client_port = atol(argv[3]);
	}
	else
	{
		client_port = 0;
	}

	// Specify the client address:
	struct sockaddr_in client_addr = { 0 };

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(client_port); // Byteorder host -> nw
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Specify the server address:
	struct sockaddr_in server_addr = { 0 };

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port); // Byteorder host -> nw

	int result = inet_pton(AF_INET, server_addr_str, &server_addr.sin_addr);

	if (result != 1)
	{
		printf("Failed to parse server IP address.\n");
		return -1;
	}

	return 0;
}
