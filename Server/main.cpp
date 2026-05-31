#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<iostream>
#include<Windows.h>
#include<winsock2.h>
#include<WS2tcpip.h>
#include<iphlpapi.h>
#include<winerror.h>

#include<fstream>
//#include<string>
#include<sstream>

#include<FormatLastError.h>

using namespace std;

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "FormatLastError.lib")

#define PORT "27015"
#define BUFFER_LENGTH 1500
#define MAX_CONNECTION 5

void save(CHAR message_for_save[]);

void main()
{
	setlocale(LC_ALL, "");
	cout << "Server" << endl;
	DWORD dwError = 0;
	CHAR sz_Buf[256] = {};
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
	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	SOCKET client_socket = accept(listen_socket, (SOCKADDR*)&addr, &addrlen);
	char* ip = inet_ntoa(addr.sin_addr);
	sprintf(sz_Buf, "Ip-адрес клиента: \t%s", ip);
	cout << sz_Buf << endl;
	cout << "Порт  клиента: \t\t" << ntohs(addr.sin_port) << endl;
	cout << "Сокет клиента: \t\t" << client_socket << endl;
	dwError = WSAGetLastError();
	if (client_socket == INVALID_SOCKET)
	{
		cout << FormatLastError(dwError, szERROR) << endl;
		cout << "Accept failed with error: " << WSAGetLastError() << endl;
	}

	//7) Получение и отправка данных:
	CHAR recvbuffer[BUFFER_LENGTH] = {};
	CHAR sendbuffer[BUFFER_LENGTH] = {};
	INT iSendResult = 0;
	dwError = WSAGetLastError();
	do
	{
		iResult = recv(client_socket, recvbuffer, BUFFER_LENGTH, 0);
		if (iResult > 0)
		{
			cout << recvbuffer << "(" << strlen(recvbuffer) << " Bytes)" << endl;
			save(recvbuffer);
			iSendResult = send(client_socket, recvbuffer, strlen(recvbuffer), 0);
			dwError = WSAGetLastError();
			if (iSendResult == SOCKET_ERROR)
			{
				cout << FormatLastError(dwError, szERROR) << endl;
				cout << "Send failed with error: " << WSAGetLastError() << endl;
				closesocket(client_socket);
			}
			else cout << "Bytes sent: " << iSendResult << endl;
		}
		else if (iResult == 0)cout << "Connection closing..." << endl;
		else
		{
			cout << FormatLastError(dwError, szERROR) << endl;
			cout << "Receive failed with error: " << WSAGetLastError() << endl;
			closesocket(client_socket);
		}
	} while (iResult > 0);

	iResult = shutdown(client_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shudown failed with " << FormatLastError(dwError, szERROR) << endl;

	iResult = shutdown(listen_socket, SD_BOTH);
	dwError = WSAGetLastError();
	if (iResult == SOCKET_ERROR)cout << "Client shudown failed with " << FormatLastError(dwError, szERROR) << endl;

	closesocket(client_socket);
	closesocket(listen_socket);

	WSACleanup();
}
void save(CHAR message_for_save[])
{
	string filename = "recv_client.txt";
	std::ofstream fout(filename);
	for (int i = 0; i < strlen(message_for_save); i++)
	{
		fout << message_for_save[i];
	}
	fout.close();
	std::string cmd = "notepad ";
	cmd += filename;
	system(cmd.c_str());
}
