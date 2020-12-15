#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_SERVER_PORT 80
#define HTTP_BUFFER_LENGTH 4096
#define HTTP_REQUEST_FORMAT "GET / HTTP/1.0\r\nHost: %s\r\n\r\n"

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

	// Create a new socket (IPv4, TCP):
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0)
	{
		printf("Failed to create socket: %s\n", strerror(errno));
		return -1;
	}

	// Bind it to the client address:
	result = bind(sock, (struct sockaddr*)&client_addr, sizeof(client_addr));

	if (result < 0)
	{
		printf("Failed to bind socket to client address: %s\n", strerror(errno));
		return -1;
	}

	// Connect to the server address:
	result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (result < 0)
	{
		printf("Failed to connect to server address: %s\n", strerror(errno));
		return -1;
	}

	// Build a HTTP GET request:
	char http_buf[HTTP_BUFFER_LENGTH];
	int request_length = snprintf(http_buf, HTTP_BUFFER_LENGTH, HTTP_REQUEST_FORMAT, server_addr_str);

	// Send it:
	int bytes_written = 0;

	while (request_length > 0)
	{
		// Write the remaining bytes:
		result = write(sock, &http_buf[bytes_written], request_length);

		if (result < 0)
		{
			printf("Failed to write some bytes: %s\n", strerror(errno));
			return -1;
		}

		// Update the counters with the number of written bytes:
		bytes_written += result;
		request_length -= result;
	}

	// Read the answer:
	do
	{
		// Read a buffer full of bytes:
		result = read(sock, http_buf, HTTP_BUFFER_LENGTH);

		// Check for error:
		if (result < 0)
		{
			printf("Failed to read some bytes: %s\n", strerror(errno));
			return -1;
		}

		// Print all the read bytes to the terminal:
		for (int i = 0; i < result; i++)
		{
			putchar(http_buf[i]);
		}
	} while (result > 0);

	// Close the socket:
	close(sock);
	printf("\n\nConnection closed.\n");

	return 0;
}
