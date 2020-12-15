#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_LISTEN_PORT 80
#define QUEUE_COUNT 128
#define ADDR_BUF_LEN 32
#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 14\r\n\r\n<h1>Test</h1>\n"

int main(int argc, char** argv)
{
	// Second parameter can be the server port:
	int server_port;

	if (argc > 1)
	{
		// TODO: Better parsing!
		server_port = atol(argv[1]);
	}
	else
	{
		server_port = DEFAULT_LISTEN_PORT;
	}

	// Specify the server address:
	struct sockaddr_in server_addr = { 0 };

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port); // Byteorder host -> nw
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// Create a new socket (IPv4, TCP):
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0)
	{
		printf("Failed to create socket: %s\n", strerror(errno));
		return -1;
	}

	// Bind it to the server address:
	int result = bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

	if (result < 0)
	{
		printf("Failed to bind socket to server address: %s\n", strerror(errno));
		return -1;
	}

	// Enable the listen mode:
	result = listen(sock, QUEUE_COUNT);

	if (result < 0)
	{
		printf("Failed to listen on server address: %s\n", strerror(errno));
		return -1;
	}

	// Accept clients:
	while (1)
	{
		// Receive the client address:
		struct sockaddr_in client_addr = { 0 };
		unsigned int addr_len = sizeof(client_addr);

		int new_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

		if (new_sock < 0)
		{
			printf("Failed to accept client: %s\n", strerror(errno));
			continue;
		}

		// Print some client info:
		char client_addr_str[ADDR_BUF_LEN];

		const char* actual_addr_str = inet_ntop(
			AF_INET,
			&client_addr.sin_addr,
			client_addr_str,
			ADDR_BUF_LEN
		);

		if (!actual_addr_str)
		{
			printf("Failed to stringify client address: %s\n", strerror(errno));
			continue;
		}

		int client_port = ntohs(client_addr.sin_port);

		printf("Accepted a connection: %s:%d\n", actual_addr_str, client_port);

		// Send a standard HTTP response:
		int bytes_to_write = strlen(HTTP_RESPONSE);
		int bytes_written = 0;

		while (bytes_to_write > 0)
		{
			// Write the remaining bytes:
			result = write(new_sock, &HTTP_RESPONSE[bytes_written], bytes_to_write);

			if (result < 0)
			{
				printf("Failed to write some bytes: %s\n", strerror(errno));
				return -1;
			}

			// Update the counters with the number of written bytes:
			bytes_written += result;
			bytes_to_write -= result;
		}

		// Send a FIN:
		result = shutdown(new_sock, SHUT_WR);

		if (result < 0)
		{
			printf("Failed to send FIN: %s\n", strerror(errno));
			close(new_sock);

			continue;
		}

		// Close the socket:
		close(new_sock);
		printf("Closed a connection: %s:%d\n", client_addr_str, client_port);
	}
}
