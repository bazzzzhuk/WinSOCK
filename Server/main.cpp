#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<iostream>
#include<Windows.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
#include<winerror.h>
#include<FormatLastError.h>
#include<Messages.h>

using namespace std;

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "FormatLastError.lib")

#define PORT "27015"
#define BUFFER_LENGTH 1500
#define MAX_CONNECTION 3

SOCKET sockets[MAX_CONNECTION] = {};
DWORD dwThreadIDs[MAX_CONNECTION] = {};
HANDLE hTreads[MAX_CONNECTION] = {};

static INT i = 0; //счётчик клиентов

//struct ClientParameters
//{
//	SOCKET client_socket;
//	sockaddr_in client_address;
//};
VOID ClientHandle(SOCKET client_socket);

void main()
{
	setlocale(LC_ALL, "");
	cout << "Server" << endl;
	DWORD dwError = 0;
	CHAR szERROR[256] = {};
	//1) INIT WINSOCK
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	dwError = WSAGetLastError();
	if (iResult != 0)
	{
		cout << FormatLastError(dwError, szERROR) << endl;
		cout << "WSAStartup failed: " << iResult << endl;
		return;
	}

	//2) Параметры подключения:
	addrinfo hints;
	addrinfo* result;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol - IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, PORT, &hints, &result);
	dwError = WSAGetLastError();
	if (iResult != 0)//10047
	{
		cout << FormatLastError(dwError, szERROR) << endl;
		cout << "getaddrinfo() failed: " << iResult << endl;
		WSACleanup();
		return;
	}

	//3)Создаём сокет для сервера, который он будет постоянно слушать "LISTENING"
	SOCKET listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	dwError = WSAGetLastError();
	if (listen_socket == INVALID_SOCKET)
	{
		cout << FormatLastError(dwError, szERROR) << endl;
		cout << "Listen socket error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	//4) BIND SOCKET:
	iResult = bind(listen_socket, result->ai_addr, result->ai_addrlen);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)
	{
		cout << FormatLastError(dwError, szERROR) << endl;
		cout << "Bind failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	freeaddrinfo(result);

	//5) Запускаем прослушивание сокета:
	if (listen(listen_socket, MAX_CONNECTION) == SOCKET_ERROR)
	{
		dwError = WSAGetLastError();
		cout << FormatLastError(dwError, szERROR) << endl;
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		closesocket(listen_socket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	//6) Обработка соединений от клиентов:
	do
	{
		sockaddr_in client_address;
		int client_addrlen = sizeof(client_address);
		client_address.sin_family = AF_INET;
		SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&client_address, &client_addrlen);
		dwError = WSAGetLastError();
		if (client_socket == INVALID_SOCKET)
		{
			cout << FormatLastError(dwError, szERROR) << endl;
			cout << "Accept failed with error: " << WSAGetLastError() << endl;
		}
		//6.1) Получаем информацию о сокете клиента:
		sockaddr_in client_address_in = (sockaddr_in)client_address;
		cout << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << endl;

		if (i < MAX_CONNECTION)
		{
			sockets[i] = client_socket;
			hTreads[i] = CreateThread
			(
				NULL,	//Security attributes
				0,		//stack size
				(LPTHREAD_START_ROUTINE)ClientHandle,//Указатель на функцию которая будет выполнять ся в потоке.
				(LPVOID)sockets[i],
				0,//
				&dwThreadIDs[i]
			);
			i++;
			cout << "Количество подключённых к серверу : " << i << endl;
		}
		else
		{
			CHAR recv_buffer[BUFFER_LENGTH] = {};
			iResult = recv(client_socket, recv_buffer, BUFFER_LENGTH, NULL);
			/*if (iResult != 0)
			{
				FormatLastError(WSAGetLastError(), szERROR);
				cout << szERROR << endl;
			}
			else cout << recv_buffer << endl;*/
			//CHAR szDeclainMessage[] = DECLINE_MESSAGE;
			iResult = send(client_socket, DECLINE_MESSAGE, strlen(DECLINE_MESSAGE), NULL);
			shutdown(client_socket, SD_BOTH);
			closesocket(client_socket);
		}
		for (int ii = 0; ii < i; ii++)
		{
			cout << sockets[ii] << endl;
			cout << hTreads[ii] << endl;
		}
	} while (true);


	/*iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shudown failed with " << FormatLastError(dwError, szERROR) << endl;*/

	/*iResult = shutdown(listen_socket, SD_RECEIVE);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shudown failed with " << FormatLastError(dwError, szERROR) << endl;*/

	closesocket(listen_socket);

	WSACleanup();

}

VOID ClientHandle(SOCKET client_socket)
{
	//7) Получение и отправка данных:
	cout << "--------------------------------------" << endl;
	sockaddr_in client_address;
	client_address.sin_family = AF_INET;
	INT namelen = sizeof(client_address);
	getpeername(client_socket, (sockaddr*)&client_address, &namelen);
	CHAR sz_client_address[256] = {};
	sprintf(sz_client_address, "%s:%d --> ", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

	cout << "Client connected:\t" << sz_client_address << "\tSOCKET:" << client_socket << endl;
	INT iSendResult = 0;
	INT iResult = 0;
	CHAR szERROR[256] = {};
	DWORD dwError = 0;
	do
	{
		CHAR sendbuffer[BUFFER_LENGTH] = {};
		CHAR recvbuffer[BUFFER_LENGTH] = {};
		iResult = recv(client_socket, recvbuffer, BUFFER_LENGTH, 0);
		if (iResult > 0)
		{
			cout << sz_client_address << recvbuffer << "(" << strlen(recvbuffer) << " Bytes)" << endl;
			iSendResult = send(client_socket, recvbuffer, strlen(recvbuffer), 0);
			dwError = WSAGetLastError();
			if (iSendResult == SOCKET_ERROR)
			{
				cout << FormatLastError(dwError, szERROR) << endl;
				cout << "Send failed with error: " << WSAGetLastError() << endl;
				closesocket(client_socket);
			}
			else cout << "Bytes sent: " << iSendResult << endl;
			cout << "--------------------------------------" << endl;
		}
		else if (iResult == 0)
		{
			cout << "Connection: " << sz_client_address << " closing..." << endl;
			i--;
			cout << "--------------------------------------" << endl;
		}
		else
		{
			cout << FormatLastError(dwError, szERROR) << endl;
			cout << "Receive failed with error: " << WSAGetLastError() << endl;
			i--;
			//closesocket(client_socket);
		}
	} while (iResult > 0);

	iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shudown failed with " << FormatLastError(dwError, szERROR) << endl;
	//closesocket(client_socket);
}