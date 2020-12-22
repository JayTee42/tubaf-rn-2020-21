#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

// Receive timeout in seconds:
#define TIMEOUT_SECS 5

// The lengths of the request message parts (IP / ICMP / total):
#define REQ_IP_HEADER_LEN 20
#define REQ_ICMP_HEADER_LEN 8
#define REQ_TOTAL_LEN (REQ_IP_HEADER_LEN + REQ_ICMP_HEADER_LEN)

// Buffer lengths:
#define RESP_BUF_LEN 4096
#define ADDR_BUF_LEN 32

// Potential response codes:
typedef enum __resp_t__
{
	RESP_FAIL,
	RESP_TIMEOUT,
	RESP_EXCEEDED,
	RESP_UNREACHABLE,
	RESP_ALIVE
} resp_t;

static bool parse_dest_addr(int argc, char** argv, struct sockaddr_in* out_addr)
{
	// We expect exactly one parameter (the destination IP):
	if (argc != 2)
	{
		printf("Usage: tubatracert <dest IP>\n");
		return false;
	}

	// Parse that IP address:
	struct sockaddr_in addr = { .sin_family = AF_INET };
	int result = inet_pton(AF_INET, argv[1], &addr.sin_addr);

	if (result != 1)
	{
		printf("Failed to parse destination IP address.\n");
		return false;
	}

	*out_addr = addr;
	return true;
}

static bool create_icmp_socket(int* out_sock)
{
	// Create the raw socket:
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (sock < 0)
	{
		printf("Failed to create raw socket: %s\n", strerror(errno));
		return false;
	}

	// To set the hop count, we must control the IP header.
	// Prevent the kernel from prepending it to all data we send:
	int val = 1;
	int result = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));

	if (result < 0)
	{
		printf("Failed to configure ICMP socket (IP header): %s\n", strerror(errno));
		return false;
	}

	// Specify the receive timeout on the socket:
	struct timeval timeout = { .tv_sec = TIMEOUT_SECS };
	result = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	if (result < 0)
	{
		printf("Failed to configure ICMP socket (timeout): %s\n", strerror(errno));
		return false;
	}

	*out_sock = sock;
	return true;
}

static uint16_t calc_icmp_checksum(const uint8_t* packet, ssize_t len)
{
	// Sum all the 16-bit words in the packet:
	uint32_t checksum = 0;

	while (len > 1)
	{
		checksum += *(uint16_t*)packet;
		packet += sizeof(uint16_t);
		len -= (ssize_t)sizeof(uint16_t);
	}

	// Handle a potential overhang byte:
	if (len == 1)
	{
		checksum += (uint16_t)*packet;
	}

	// Fold potential overflows:
	while (checksum >> 16)
	{
		checksum = (checksum & 0xFFFF) + (checksum >> 16);
	}

	return (uint16_t)~checksum;
}

static bool send_icmp_echo_req(int sock, struct sockaddr_in* addr, uint8_t ttl, uint16_t ident, uint16_t seq_num)
{
	// Fill the IP header:
	uint32_t dst_addr = (uint32_t)addr->sin_addr.s_addr;
	uint8_t packet[REQ_TOTAL_LEN];
	uint8_t* ip = &packet[0];

	ip[0] = 0x45;                       // IPv4; 5 * 4 = 20 bytes header
	ip[1] = 0x00;                       // DSCP
	*(uint16_t*)&ip[2] = REQ_TOTAL_LEN; // Total length (checked by kernel)
	*(uint16_t*)&ip[4] = 0x0000;        // Identifier (managed by kernel)
	*(uint16_t*)&ip[6] = 0x0000;        // Flags, Fragment offset (managed by kernel)
	ip[8] = ttl;                        // TTL
	ip[9] = IPPROTO_ICMP;               // Contained protocol
	*(uint16_t*)&ip[10] = 0x0000;       // Checksum (managed by kernel)
	*(uint32_t*)&ip[12] = 0x00000000;   // Source address (managed by kernel)
	*(uint32_t*)&ip[16] = dst_addr;     // Destination address

	// Fill the ICMP header:
	uint8_t* icmp = &packet[REQ_IP_HEADER_LEN];

	icmp[0] = 0x08;                 // Type (echo request)
	icmp[1] = 0x00;                 // Code
	*(uint16_t*)&icmp[2] = 0x0000;  // Checksum (pending)
	*(uint16_t*)&icmp[4] = ident;   // Identifier
	*(uint16_t*)&icmp[6] = seq_num; // Sequence number

	// Calculate the ICMP checksum:
	*(uint16_t*)&icmp[2] = calc_icmp_checksum(icmp, REQ_ICMP_HEADER_LEN);

	// Send the raw packet:
	ssize_t result = sendto(sock, packet, REQ_TOTAL_LEN, 0, (struct sockaddr*)addr, sizeof(*addr));

	if (result < 0)
	{
		printf("Failed to send raw ICMP packet: %s\n", strerror(errno));
		return false;
	}

	// Ensure that we have sent the whole packet:
	if (result != REQ_TOTAL_LEN)
	{
		printf("ICMP packet has only been sent partially.\n");
		return false;
	}

	return true;
}

resp_t recv_icmp_echo_resp(int sock, uint16_t ident, uint16_t seq_num, struct sockaddr_in* out_resp_addr)
{
	// Create a sufficiently large receive buffer:
	uint8_t packet[RESP_BUF_LEN];

	while (true)
	{
		// Receive the next packet:
		struct sockaddr_in resp_addr;
		socklen_t resp_addr_len = sizeof(resp_addr);

		ssize_t result = recvfrom(sock, packet, RESP_BUF_LEN, 0, (struct sockaddr*)&resp_addr, &resp_addr_len);

		// Is this a timeout?
		if (errno == EAGAIN)
		{
			return RESP_TIMEOUT;
		}

		// Is this some other kind of error?
		if (result < 0)
		{
			printf("errno is %d\n", errno);
			printf("Failed to receive raw packet: %s\n", strerror(errno));
			return RESP_FAIL;
		}

		// Skip empty or non-IPv4 packets:
		const uint8_t* ip = &packet[0];

		if ((result == 0) || (((ip[0] & 0xF0) >> 4) != 0x04))
		{
			continue;
		}

		// Read and verify the IP header length and the protocol type:
		size_t ip_header_len = (size_t)(ip[0] & 0x0F) * 4;

		if ((ip_header_len < 20) || (result < ip_header_len) || (ip[9] != IPPROTO_ICMP))
		{
			continue;
		}

		// Calculate the offset and length of the ICMP payload:
		const uint8_t* icmp = &ip[ip_header_len];
		size_t icmp_length = (size_t)result - ip_header_len;

		// Ensure that there is space for an ICMP header:
		if (icmp_length < 8)
		{
			continue;
		}

		// Extract the ICMP type and code:
		uint8_t icmp_type = icmp[0];
		uint8_t icmp_code = icmp[1];

		// (0x0b, 0x00): TTL exceeded
		if ((icmp_type == 0x0B) && (icmp_code == 0x00))
		{
			// TODO: Validate identifier and sequence number.
			*out_resp_addr = resp_addr;
			return RESP_EXCEEDED;
		}

		// (0x03, _): Unreachable
		if (icmp_type == 0x03)
		{
			// TODO: Validate identifier and sequence number.
			return RESP_UNREACHABLE;
		}

		// (0x00, 0x00): Echo Response
		if ((icmp_type == 0x00) && (icmp_code == 0x00))
		{
			// Validate identifier and sequence numer:
			if ((*(uint16_t*)&icmp[4] == ident) && (*(uint16_t*)&icmp[6] == seq_num))
			{
				return RESP_ALIVE;
			}
		}
	}
}

int main(int argc, char** argv)
{
	// Parse the destination address:
	struct sockaddr_in addr;

	if (!parse_dest_addr(argc, argv, &addr))
	{
		return -1;
	}

	// Create a new ICMP socket:
	int sock;

	if (!create_icmp_socket(&sock))
	{
		return -1;
	}

	// Initialize the random number generator:
	srand((unsigned int)time(NULL));

	// Walk the hosts:
	uint8_t ttl = 1;

	while (true)
	{
		// Select an identifier and a sequence number:
		uint16_t ident = (uint16_t)rand();
		uint16_t seq_num = (uint16_t)rand();

		// Try to send the echo request:
		if (!send_icmp_echo_req(sock, &addr, ttl, ident, seq_num))
		{
			break;
		}

		// Try to receive the response:
		struct sockaddr_in resp_addr;
		resp_t resp = recv_icmp_echo_resp(sock, ident, seq_num, &resp_addr);

		// Handle the response:
		switch (resp)
		{
		case RESP_FAIL: break;
		case RESP_TIMEOUT:

			printf("Timeout has occurred!\n");
			break;

		case RESP_EXCEEDED:

			{
				char resp_addr_str_buf[ADDR_BUF_LEN];
				const char* resp_addr_str = inet_ntop(AF_INET, &resp_addr.sin_addr, resp_addr_str_buf, ADDR_BUF_LEN);

				if (!resp_addr_str)
				{
					resp_addr_str = "failed to stringify";
				}

				printf("TTL %d: Middlebox <%s>\n", ttl, resp_addr_str);
			}

			break;

		case RESP_UNREACHABLE:

			printf("Host is unreachable!\n");
			break;

		case RESP_ALIVE:

			printf("TTL %d: Destination reached!\n", ttl);
			break;
		}

		// Only stay in the loop if this was a middlebox:
		if (resp != RESP_EXCEEDED)
		{
			break;
		}

		// Check if we have exceeded the TTL space:
		if (ttl == 0xFF)
		{
			printf("TTL exceeded!\n");
			break;
		}

		// Continue with the next TTL:
		ttl++;
	}

	// Close the socket:
	close(sock);

	return 0;
}
