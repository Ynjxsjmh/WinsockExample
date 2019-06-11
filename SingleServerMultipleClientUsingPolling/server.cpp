#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27016"
#define MAX_CLIENT 3

typedef struct _MESSAGE
{
    char username[20];
    char time[36];
    int  content_length;
    char content[DEFAULT_BUFLEN];
} MESSAGE;

int __cdecl main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    SOCKET ClientSockets[30];

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    //set of socket descriptors
    fd_set readfds;

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iSendResult;
    MESSAGE message;

    for (int i = 0; i < MAX_CLIENT; i++)
    {
        ClientSockets[i] = 0;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 )
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Waiting for incoming connections: " << endl;

    while (1)
    {
        // Clear the socket fd set
        FD_ZERO(&readfds);

        // Add ListenSocket socket to fd set
        FD_SET(ListenSocket, &readfds);

        // Add child sockets to fd set
        for (int  i = 0 ; i < MAX_CLIENT; i++)
        {
            SOCKET tempSocket;
            tempSocket = ClientSockets[i];
            if(tempSocket > 0)
            {
                FD_SET(tempSocket, &readfds);
            }
        }

        cout << endl << "Selecting..." << endl;
        iResult = select(0, &readfds, NULL, NULL, NULL);
        if (iResult == SOCKET_ERROR)
        {
            printf("Select call failed with error code: %d", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(ListenSocket, &readfds))
        {
            if ((ClientSocket = accept(ListenSocket, NULL, NULL)) == INVALID_SOCKET)
            {
                printf("accept failed with error: %d\n", WSAGetLastError());
                closesocket(ListenSocket);
                WSACleanup();
                return 1;
            }

            cout << endl << "New ClientSocket = " << ClientSocket << endl;

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENT; i++)
            {
                if (ClientSockets[i] == 0)
                {
                    ClientSockets[i] = ClientSocket;
                    printf("Adding to list of sockets at index %d \n", i);
                    break;
                }
            }

            cout << endl;
        }

        cout << endl << "Loop to check if something happens..." << endl;
        // Else its some IO operation on some other socket
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            cout << i << endl;

            SOCKET tempSocket;
            tempSocket = ClientSockets[i];

            // Check whether client presend in read sockets
            if (FD_ISSET(tempSocket, &readfds))
            {
                // Check if it was for closing , and also read the incoming message
                iResult = recv(tempSocket, recvbuf, recvbuflen, 0);

                if (iResult > 0)
                {
                    // Echo back the message that came in
                    memset(&message, 0, sizeof(message));
                    memcpy(&message, recvbuf, sizeof(message));
                    message.content[message.content_length]='\0';

                    printf("Received Message from SOCKET %d: %s\n", tempSocket, message.content);

                    // Echo the buffer back to the sender
                    iSendResult = send(tempSocket, recvbuf, iResult, 0 );
                    if (iSendResult == SOCKET_ERROR)
                    {
                        printf("send failed with error: %d\n", WSAGetLastError());
                    }

                    printf("Bytes sent: %d\n", iSendResult);
                }
                else if (iResult == 0)
                {
                    // Somebody disconnected, get his details and print
                    printf("Host disconnected unexpectedly with SOCKET: %d\n", tempSocket);

                    // Close the socket and mark as 0 in list for reuse
                    closesocket(tempSocket);
                    ClientSockets[i] = 0;
                }
                else
                {
                    int error_code = WSAGetLastError();
                    if (error_code == WSAECONNRESET)
                    {
                        // Somebody disconnected , get his details and print
                        printf("Host disconnected unexpectedly with SOCKET: %d\n", tempSocket);

                        // Close the socket and mark as 0 in list for reuse
                        closesocket(tempSocket);
                        ClientSockets[i] = 0;
                    }
                    else
                    {
                        printf("recv failed with error: %d\n", WSAGetLastError());
                    }
                }
            }
        }
    }

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}
