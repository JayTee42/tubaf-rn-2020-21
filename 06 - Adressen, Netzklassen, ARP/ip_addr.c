#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//Read an address from the CLI to "*result".
//Return 1 on success, 0 otherwise.
int read_addr(uint32_t* result)
{
	//Build the format string:
	char format[8];
	snprintf(format, sizeof(format), "%%%zus", (size_t)(INET_ADDRSTRLEN - 1));

	//Read an IP address from the user:
	char addr_str[INET_ADDRSTRLEN];
	scanf(format, addr_str);

	//Parse it:
	struct in_addr addr_struct;

	if (!inet_pton(AF_INET, addr_str, &addr_struct))
	{
		return 0;
	}

	*result = addr_struct.s_addr;
	return 1;
}

//Print the given address to "*buf".
void print_addr(uint32_t addr, char* buf)
{
	struct in_addr addr_struct =
	{
		.s_addr = addr
	};

	inet_ntop(AF_INET, &addr_struct, buf, INET_ADDRSTRLEN);
}

uint32_t get_net_identifier(uint32_t addr, uint32_t mask)
{
	//FIXME
}

uint32_t get_host(uint32_t addr, uint32_t mask)
{
	//FIXME
}

uint32_t get_broadcast(uint32_t addr, uint32_t mask)
{
	//FIXME
}

int main(void)
{
	//Read address and subnet mask:
	uint32_t addr, mask;

	printf("Please enter your address: -> ");

	if (!read_addr(&addr))
	{
		printf("Wrong format!\n");
		return -1;
	}

	printf("Please enter your subnet mask: -> ");

	if (!read_addr(&mask))
	{
		printf("Wrong format!\n");
		return -1;
	}

	//Calculate interesting info from that:
	char addr_str[INET_ADDRSTRLEN];

	//Net identifier:
	uint32_t net_identifier = get_net_identifier(addr, mask);
	print_addr(net_identifier, addr_str);
	printf("Net identifier: %s\n", addr_str);

	//Host:
	uint32_t host = get_host(addr, mask);
	print_addr(host, addr_str);
	printf("Host: %s\n", addr_str);

	//Broadcast:
	uint32_t broadcast = get_broadcast(addr, mask);
	print_addr(broadcast, addr_str);
	printf("Broadcast: %s\n", addr_str);

	return 0;
}
