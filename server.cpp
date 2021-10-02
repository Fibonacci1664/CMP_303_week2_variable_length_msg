/*	CMP303 & 501 Lab 2 TCP server example - by Henry Fortuna and Adam Sampson
	Updated by Gaz Robinson & Andrei Boiko

	A simple server that waits for a connection.
	The server repeats back anything it receives from the client.
	All the calls are blocking -- so this program only handles
	one connection at a time.
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

// The IP address for the server to listen on
#define SERVERIP "127.0.0.1"

// The TCP port number for the server to listen on
#define SERVERPORT 5555

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40

// Prototypes
bool talk_to_client_delim(SOCKET clientSocket);
bool talk_to_client_num_prefix(SOCKET clientSocket);
void die(const char *message);

int main()
{
	printf("Echo Server\n");

	// Initialise the WinSock library -- we want version 2.2.
	WSADATA w;
	int error = WSAStartup(0x0202, &w);
	if (error != 0)
	{
		die("WSAStartup failed");
	}
	if (w.wVersion != 0x0202)
	{
		die("Wrong WinSock version");
	}

	// Create a TCP socket that we'll use to listen for connections.
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET)
	{
		die("socket failed");
	}

	// Fill out a sockaddr_in structure to describe the address we'll listen on.
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);
	// htons converts the port number to network byte order (big-endian).
	serverAddr.sin_port = htons(SERVERPORT);

	// Bind the server socket to that address.
	if (bind(serverSocket, (const sockaddr *) &serverAddr, sizeof(serverAddr)) != 0)
	{
		die("bind failed");
	}

	// ntohs does the opposite of htons.
	printf("Server socket bound to address %s, port %d\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));

	// Make the socket listen for connections.
	if (listen(serverSocket, 1) != 0)
	{
		die("listen failed");
	}

	printf("Server socket listening\n");

	bool quit = false;

	while (true)
	{
		printf("Waiting for a connection...\n");

		// Accept a new connection to the server socket.
		// This gives us back a new socket connected to the client, and
		// also fills in an address structure with the client's address.
		sockaddr_in clientAddr;
		int addrSize = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (sockaddr *) &clientAddr, &addrSize);

		if (clientSocket == INVALID_SOCKET)
		{
			// accept failed -- just try again.
			continue;
		}

		printf("Client has connected from IP address %s, port %d!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		
		//quit = talk_to_client_delim(clientSocket);
		quit = talk_to_client_num_prefix(clientSocket);

		printf("Client disconnected\n");

		// Close the connection.
		closesocket(clientSocket);

		if (quit)
		{
			break;
		}
	}

	// We won't actually get here, but if we did then we'd want to clean up...
	printf("Quitting\n");
	closesocket(serverSocket);
	WSACleanup();
	return 0;
}

// Communicate with a client.
// The socket will be closed when this function returns.
bool talk_to_client_num_prefix(SOCKET clientSocket)
{
	bool flag = false;

	while (true)
	{
		int count = 0;
		int alphaStartPos = 0;
		char buffer[MESSAGESIZE];
		char tempBuffer[MESSAGESIZE];
		char rtnBuffer[MESSAGESIZE];
		std::string rtnMsg;
		std::string numBuffer;

		// Receive entire msg, up to max 40 bytes
		count = recv(clientSocket, buffer, MESSAGESIZE, 0);

		// Copy entire buffer into temp
		memcpy(tempBuffer, buffer, min(count, MESSAGESIZE));

		if (count == SOCKET_ERROR)
		{
			die("message recv error");
		}

		// Process temp reading chars until we hit first alpha char
		for (int i = 0; i < count; ++i)
		{
			// If the thing in the tempBuffer at the current iteration is a number, append to string numBuffer
			if (std::isdigit(tempBuffer[i]))
			{
				// Disregard leading zeros, preventing any confusion with base 8, octal. Even though stoi default base = 10
				// Check against 48 as this is the char ASCII value of zero
				if (tempBuffer[i] > 48)
				{
					numBuffer.push_back(tempBuffer[i]);
				}
			}

			// This first time we hit an alpha char, break;
			if (std::isalpha(tempBuffer[i]))
			{
				alphaStartPos = i;
				break;
			}
		}

		// String to int the stored data
		int dataByteCount = 0;
		dataByteCount = std::stoi(numBuffer);
		
		// Loop over tempBuffer again, starting from where we broke out of the previous loop
		// start processing the data from this point and for the amount of dataByteCount
		// Storing this in a std::string
		for (int i = alphaStartPos; i < (alphaStartPos + dataByteCount); ++i)
		{
			rtnMsg.push_back(tempBuffer[i]);
		}

		// Copy the data from std::string to new rtn buffer
		memcpy(rtnBuffer, rtnMsg.c_str(), dataByteCount);

		// test whether the msg was quit
		if (memcmp(rtnBuffer, "quit", 4) == 0)
		{
			printf("Client asked to quit\n");
			flag = true;
			break;
		}

		// Otherwise rtn msg
		if (send(clientSocket, rtnBuffer, dataByteCount, 0) > MESSAGESIZE)
		{
			printf("send failed\n");
			return false;
		}

		if (count <= 0)
		{
			printf("Client closed connection\n");
			return true;
		}

		// (Note that recv will not write a \0 at the end of the message it's
		// received -- so we can't just use it as a C-style string directly
		// without writing the \0 ourself.)
		printf("Received %d bytes from the client: '", count);
		fwrite(buffer, 1, count, stdout);
		printf("'\n");

		if (flag)
		{
			return true;
		}
	}

	return true;
}

// Communicate with a client.
// The socket will be closed when this function returns.
bool talk_to_client_delim(SOCKET clientSocket)
{
	bool flag = false;

	while (true)
	{
		int count = 0;
		char buffer[MESSAGESIZE];
		char tempBuffer[MESSAGESIZE];

		// Receive entire msg, up to max 40 bytes
		count = recv(clientSocket, buffer, MESSAGESIZE, 0);

		// Copy entire buffer into temp
		memcpy(tempBuffer, buffer, min(count, MESSAGESIZE));

		if (count == SOCKET_ERROR)
		{
			die("message recv error");
		}

		// Process temp looking for delimiter char
		for (int i = 0; i < count; ++i)
		{
			// Check for delimiter char AND if this was also the last char, i.e. the msg end
			if (tempBuffer[i] == '#' && i == count - 1)
			{
				printf("Hit the delimiter!\n");

				if (memcmp(tempBuffer, "quit", 4) == 0)
				{
					printf("Client asked to quit\n");
					flag = true;
					break;
				}

				// Send the same data back to the client, ONLY once we've hit the final delimiter
				if (send(clientSocket, tempBuffer, count, 0) > MESSAGESIZE)
				{
					printf("send failed\n");
					return false;
				}
			}
		}

		if (count <= 0)
		{
			printf("Client closed connection\n");
			return true;
		}

		// (Note that recv will not write a \0 at the end of the message it's
		// received -- so we can't just use it as a C-style string directly
		// without writing the \0 ourself.)
		printf("Received %d bytes from the client: '", count);
		fwrite(buffer, 1, count, stdout);
		printf("'\n");

		if (flag)
		{
			return true;
		}
	}

	return true;
}

// Print an error message and exit.
void die(const char *message)
{
	fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());

#ifdef _DEBUG
	// Debug build -- drop the program into the debugger.
	abort();
#else
	exit(1);
#endif
}