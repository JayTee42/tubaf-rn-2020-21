#include <stdio.h>

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

	return 0;
}
