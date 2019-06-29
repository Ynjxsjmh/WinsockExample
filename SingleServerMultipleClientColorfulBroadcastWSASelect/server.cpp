#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <list>
#include <iostream>

using namespace std;

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27016"

typedef struct _MESSAGE {
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

int __cdecl main(void) {
    WSADATA wsaData;
    int iResult;
    MESSAGE message;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // for our thread
    DWORD thread;

    WORD Attributes = 0;

    // 套接字数组
    SOCKET SocketArray[WSA_MAXIMUM_WAIT_EVENTS];
    // 事件句柄数组
    WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
    WSAEVENT NewEvent;
    SOCKET AcceptSocket;
    DWORD EventTotal = 0;
    DWORD Index;
    WSANETWORKEVENTS NetworkEvents;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
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
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // 创建一个事件对象，将它与监听套接字相关联，并注册了网络事件
    NewEvent = WSACreateEvent();
    WSAEventSelect(ListenSocket, NewEvent, FD_ACCEPT | FD_CLOSE);

    // 启动监听，并将监听套接字和对应的事件添加到相应的数组中
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    SocketArray[EventTotal] = ListenSocket;
    EventArray[EventTotal] = NewEvent;
    EventTotal++;

    while (1) {
        Index = WSAWaitForMultipleEvents(EventTotal, EventArray, FALSE, WSA_INFINITE, FALSE);

        WSAEnumNetworkEvents(SocketArray[Index-WSA_WAIT_EVENT_0], EventArray[Index-WSA_WAIT_EVENT_0], &NetworkEvents);

        // 检查 FD_ACCEPT 的消息
        if (NetworkEvents.lNetworkEvents & FD_ACCEPT) {
            if (NetworkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
                printf("FD_ACCEPT failed with error %d\n", NetworkEvents.iErrorCode[FD_ACCEPT_BIT]);
                break;
            }

            // 是 FD_ACCEPT 消息，并且没有错误，那就接收这个新的连接请求
            // 并把产生的套接字添加到套接字和事件数组中
            printf("Accepting...\n");
            ClientSocket = accept(SocketArray[Index-WSA_WAIT_EVENT_0], NULL, NULL);

            if (EventTotal > WSA_MAXIMUM_WAIT_EVENTS) {
                printf("Too many connections\n");
                closesocket(AcceptSocket);
                break;
            }

            // 为 ClientSocket 套接字创建一个新的事件对象
            NewEvent = WSACreateEvent();

            // 将该事件对象与 ClientSocket 套接字相关联，并注册网络事件
            WSAEventSelect(ClientSocket, NewEvent, FD_READ | FD_WRITE | FD_CLOSE);

            // 将该套接字和对应的事件对象添加到数组中，统一管理
            EventArray[EventTotal] = NewEvent;
            SocketArray[EventTotal] = AcceptSocket;
            EventTotal++;
            printf("haha\n");
            printf("Socket %d connected\n", ClientSocket);
        }

        // 处理 FD_READ 消息通知
        if (NetworkEvents.lNetworkEvents & FD_READ) {
            if (NetworkEvents.iErrorCode[FD_READ_BIT] != 0) {
                printf("FD_READ failed with error %d\n", NetworkEvents.iErrorCode[FD_READ_BIT]);
                break;
            }

            // 从套接字读数据
            recv(SocketArray[Index-WSA_WAIT_EVENT_0], recvbuf, sizeof(recvbuf), 0);
        }

        // 处理 FD_READ 消息通知
        if (NetworkEvents.lNetworkEvents & FD_WRITE) {
            if (NetworkEvents.iErrorCode[FD_WRITE_BIT != 0]) {
                printf("FD_WRITE failed with error %d\n", NetworkEvents.iErrorCode[FD_WRITE_BIT]);
                break;
            }

            // 写数据到套接字
            send(SocketArray[Index-WSA_WAIT_EVENT_0], recvbuf, sizeof(recvbuf), 0);
        }

        // 处理 FD_CLOSE 消息通知
        if (NetworkEvents.lNetworkEvents & FD_CLOSE) {
            if (NetworkEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
                printf("FD_CLOSE failed with error %d\n", NetworkEvents.iErrorCode[FD_CLOSE_BIT]);
                break;
            }

            // 关闭该套接字
            closesocket(SocketArray[Index-WSA_WAIT_EVENT_0]);

            // 从套接字和事件数组中删除该套接字和相应的事件句柄
            // 并将 EventTotal 计数器减1，该函数实现省略了
            // CompressArrays(EventArray, SocketArray, &EventTotal);
        }
    }

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
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
