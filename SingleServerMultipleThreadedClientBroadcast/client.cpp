#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

SOCKET ConnectSocket = INVALID_SOCKET;

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

int send_data()
{
    char sendbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    MESSAGE message;
    int iResult;
    WORD Attributes = 0;

    printf("\n\nPlease input message you want to send:\n");
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

        return 2;
        // break;
    }

    memset(sendbuf, 0, sizeof(sendbuf));
    // convert struct type to char[]
    memcpy(sendbuf, &message, sizeof(MESSAGE));

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

    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    return 0;
}

DWORD WINAPI recv_data(LPVOID args)
{
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    MESSAGE message;
    int iResult;
    WORD Attributes = 0;

    while (1)
    {
        // printf("Receiving message...\n");
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);

        if (iResult > 0)
        {
            memset(message.content, 0, sizeof(message.content));
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
            // printf("recv failed with error: %d\n", WSAGetLastError());
        }
    }

    return 0;
}

int __cdecl main(int argc, char **argv)
{
    WSADATA wsaData;

    // Declare an addrinfo object that contains a sockaddr structure
    struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;

    int iResult;

    // for our thread
    DWORD send_thread_ID;
    HANDLE recv_thread_handler;
    DWORD recv_thread_ID;

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
    for(ptr = result; ptr != NULL; ptr = ptr->ai_next)
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

//  Shouldn't use send_data thread.
//	CreateThread( NULL, 0, send_data, NULL, 0, &send_thread_ID );

    // receive messages
    recv_thread_handler = CreateThread(NULL, 0, recv_data, NULL, 0, &recv_thread_ID);
    if (!recv_thread_handler)
    {
        printf("Error at CreateThread(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 0;
    }

    while (1)
    {
        int judge = send_data();

        if (judge == 0)
        {
            continue;
        }
        else if (judge == 1)
        {
            TerminateThread(recv_thread_handler, recv_thread_ID);
            break;
        }
        else if (judge == 2)
        {
            TerminateThread(recv_thread_handler, recv_thread_ID);
            break;
        }
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
