/*	CMP303 & 501 Lab 2 TCP server example - by Henry Fortuna and Adam Sampson
	Updated by Gaz Robinson & Andrei Boiko

	A simple server that waits for a connection.
	The server repeats back anything it receives from the client.
	All the calls are blocking -- so this program only handles
	one connection at a time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")


// The IP address for the server to listen on
#define SERVERIP "127.0.0.1"

// The TCP port number for the server to listen on
#define SERVERPORT 5555

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 40


// Prototypes
bool talk_to_client(SOCKET clientSocket); 
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

		quit = talk_to_client(clientSocket);

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
bool talk_to_client(SOCKET clientSocket)
{
	char buffer[1];
	//std::string rcv;
	std::string rtnMsg;
	int count = 0;

	while (true)
	{
		// Receive one byte at a time
		count = recv(clientSocket, buffer, 1, 0);

		// Check for delimiter char
		if (buffer[0] == '#')
		{
			printf("Hit the delimiter!\n");

			// Copy first 4 chars from rtnMsg to quit string container
			if (rtnMsg.compare("quit") == 0)
			{
				printf("Client asked to quit\n");
				break;
			}

			// Send the same data back to the client.
			if (send(clientSocket, rtnMsg.c_str(), rtnMsg.size(), 0) > MESSAGESIZE)
			{
				printf("send failed\n");
				return false;
			}

			//rcv.clear();
			rtnMsg.clear();
		}

		if (count <= 0)
		{
			printf("Client closed connection\n");
			return true;
		}

		if (count != 1)
		{
			printf("Got strange-sized message from client\n");
			return true;
		}

		// Don't copy the delimiter char into the return msg
		if (buffer[0] != '#')
		{
			rtnMsg.push_back(buffer[0]);
		}	

		// (Note that recv will not write a \0 at the end of the message it's
		// received -- so we can't just use it as a C-style string directly
		// without writing the \0 ourself.)
		printf("Received %d bytes from the client: '", count);
		fwrite(buffer, 1, count, stdout);
		printf("'\n");
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