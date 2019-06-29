#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include <string>
#include <iostream>

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27016"

typedef struct _MESSAGE
{
    char username[20];
    char time[36];
    int  content_length;
    char content[DEFAULT_BUFLEN];
} MESSAGE;

list<SOCKET> clientSockets;

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

// our thread for recving commands
DWORD WINAPI receive_cmds( LPVOID lpParam )
{
    printf( "thread created\r\n" );

    cout << "lpParam = " << lpParam << endl;

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iResult;
    int iSendResult;
    MESSAGE message;

    // set our socket to the socket passed in as a parameter
    SOCKET clientSocket = (SOCKET) lpParam;

    // Receive until the peer shuts down the connection
    do
    {
        memset(recvbuf, 0, sizeof(recvbuf));
        // isResult is the length of message it receives
        iResult = recv(clientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
        {
            memset(&message, 0, sizeof(message));
            // convert recvbuf to Message
            memcpy(&message, recvbuf, sizeof(message));
            message.content[message.content_length]='\0';

            printf("\n\nReceived Message from SOCKET %d: %s\n", clientSocket, message.content);

            // loop through the list and send echo message to them except himself
            for (std::list<SOCKET>::iterator it = clientSockets.begin(); it != clientSockets.end(); ++it)
            {
                if (*it == clientSocket)
                {
                    continue;
                }
                char tempbuf[DEFAULT_BUFLEN];

                memset(message.username, 0, sizeof(message.username));
                strcpy(message.username, to_string(clientSocket).c_str());
                cout << "Wrap sender username:" << message.username << endl;

                memset(tempbuf, 0, sizeof(tempbuf));
                memcpy(tempbuf, &message, sizeof(message));

                // Echo the buffer back to the sender
                iSendResult = send(*it, tempbuf, iResult, 0 );
                if (iSendResult == SOCKET_ERROR)
                {
                    printf("send failed with error: %d\n", WSAGetLastError());
                    closesocket(clientSocket);
                    WSACleanup();
                    return 1;
                }

                printf("Bytes sent: %d\n", iSendResult);
            }
        }
        else if (iResult == 0)
        {
            printf("Connection with %d is closing...\n", clientSocket);
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
    }
    while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(clientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
}

int __cdecl main(void)
{
    WSADATA wsaData;
    int iResult;
    MESSAGE message;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // for our thread
    DWORD thread;

    WORD Attributes = 0;

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

    int i = 0;
    while (1)
    {
        printf("\nListening %d...\n", i);
        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        clientSockets.push_back(ClientSocket);

        SetConsoleColour(&Attributes, FOREGROUND_INTENSITY | FOREGROUND_RED);
        cout << "New ClientSocket = " << ClientSocket << endl;
        ResetConsoleColour(Attributes);

        /* create our recv_cmds thread and parse client socket as a parameter */
        CreateThread( NULL, 0, receive_cmds, (LPVOID) ClientSocket, 0, &thread );
        i++;
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
