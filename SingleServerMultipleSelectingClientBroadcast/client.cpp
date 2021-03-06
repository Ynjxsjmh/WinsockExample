#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27016"
#define DEFAULT_SERVER "127.0.0.1"

typedef struct _MESSAGE
{
    char username[20];
    char time[36];
    int  content_length;
    char content[DEFAULT_BUFLEN];
} MESSAGE;

void SetConsoleColour(WORD* Attributes, DWORD Colour)
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hStdout, &Info);
    *Attributes = Info.wAttributes;
    SetConsoleTextAttribute(hStdout, Colour);
}

void ResetConsoleColour(WORD Attributes)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Attributes);
}

int __cdecl main(int argc, char **argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    // Declare an addrinfo object that contains a sockaddr structure
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
    char sendbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;
    MESSAGE message;
    WORD Attributes = 0;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(DEFAULT_SERVER, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 )
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL; ptr=ptr->ai_next)
    {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                               ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET)
        {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET)
    {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    while (1)
    {
        fd_set readfds, writefds;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

//		printf("%d %d\n", ConnectSocket, STDIN_FILENO);
        FD_SET(ConnectSocket, &readfds);
//		FD_SET(STDIN_FILENO, &readfds);
        FD_SET(ConnectSocket, &writefds);

        printf("Selecting...\n");
        iResult = select(0, &readfds, &writefds, NULL, NULL);
        if (iResult == SOCKET_ERROR)
        {
            printf("Select call failed with error code: %d", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        if (FD_ISSET(ConnectSocket, &readfds))
        {
            // We got data from server, read it
            printf("Receiving message...\n");
            iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0)
            {
                memset(&message, 0, sizeof(message));
                memcpy(&message, recvbuf, sizeof(message));
                SetConsoleColour(&Attributes, FOREGROUND_INTENSITY | FOREGROUND_RED);
                printf("Received Message from SOCKET %s: %s\n", message.username, message.content);
                ResetConsoleColour(Attributes);
            }
            else if (iResult == 0)
            {
                printf("Connection closed\n");
            }
            else
            {
                printf("recv failed with error: %d\n", WSAGetLastError());
            }
        }

        if (FD_ISSET(ConnectSocket, &writefds))
        {
            // Send data to server
            printf("\n\nPlease input message you want to send: ");

            memset(message.content, 0, sizeof(message.content));
            message.content_length = read(STDIN_FILENO, message.content, 1024) - 1;
            message.content[message.content_length] = '\0';

            if (strcmp(message.content, "quit") == 0)
            {
                // shutdown the send half of the connection since no more data will be sent
                iResult = shutdown(ConnectSocket, SD_SEND);
                if (iResult == SOCKET_ERROR)
                {
                    printf("shutdown failed: %d\n", WSAGetLastError());
                    closesocket(ConnectSocket);
                    WSACleanup();
                    return 1;
                }
                break;
            }

            memset(sendbuf, 0, sizeof(sendbuf));
            // convert struct type to char[]
            memcpy(sendbuf, &message, sizeof(message));

            // Send an initial buffer
            iResult = send(ConnectSocket, sendbuf, sizeof(sendbuf), 0);
            if (iResult == SOCKET_ERROR)
            {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return 1;
            }

            printf("Send Message: %s\n", message.content);
        }
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();


    return 0;
}
