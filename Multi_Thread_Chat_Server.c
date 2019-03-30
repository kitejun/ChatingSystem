#include <stdio.h>
#include <process.h>
#include <winsock.h>
#include <windows.h>

#pragma comment(lib, "wsock32.lib")	
#define PORT 20000							// 사용포트는 20000
#define MAX_CLIENT 5							// 최대 허용 인원 수 5개( 작성자 자유 )
#define ALLOW 6000							// 최대 생성 가능 소켓 번호 6000

void recv_client(void * kim);					// 스레드 함수 프로토 타입
int client_num = 0;							// 점유 횟수 (클라이언트 갯수)
int number = 0;								// 클라언트 번호
char user_hi[] = "번 손님";					// Welcome 정상 초기 글
char user_full[] = "가득찼습니다.\n";			// Welcome 사용자 초가시 생기는 글
int client_sock[ALLOW];						// client_sock (클라이언트 Welcome Socket)
HANDLE hMutex;								// 뮤택스
SOCKADDR_IN server_info, client_info;

int main()
{
	int addrsize, ret;
	SOCKET server;
	
	// Welcome Screen
	printf("■■■■■■■■■■■■■■■■■■■■■■\n");
	printf("■                                        ■\n");
	printf("■     멀티 스레드를 이용한 멀티 채팅     ■\n");
	printf("■             서버 (Server)              ■\n");
	printf("■                                        ■\n");
	printf("■■■■■■■■■■■■■■■■■■■■■■\n");


	// 뮤택스 생성
	hMutex = CreateMutex(NULL, FALSE, NULL);	// 생성 실패시 오류
	if (!hMutex)
	{
		printf("☞ Mutex 오류입니다. ☜\n");
		CloseHandle(hMutex);
		return 0;
	}

	// 윈속 초기화
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(1, 1), &wsd) != 0)
	{
		printf("☞Winsock 오류입니다.☜\n");
		WSACleanup();
		return 0;

	}
	// 소켓 생성
	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == SOCKET_ERROR)
	{
		printf("☞socket() 오류입니다.☜\n");
		closesocket(server);
		WSACleanup();
		return 0;
	}

	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(PORT);

	// Bind
	if (bind(server, (SOCKADDR_IN*)&server_info, sizeof(server_info)) == SOCKET_ERROR)
	{
		printf("☞bind() 오류입니다.☜\n");
		closesocket(server);
		WSACleanup();
		return 0;

	}

	printf("♥ 채팅방이 시작되었습니다. %d명 남았습니다. ♥\n", MAX_CLIENT - client_num);
	listen(server, 10);
	addrsize = sizeof(client_info);

	// 클라이언트 접속을 기다립니다.
	while (1)
	{
		// 블록킹 방식으로 Client 를 기다립니다.
		client_sock[number] = accept(server, (SOCKADDR_IN*)&client_info, &addrsize);

		if (client_num < MAX_CLIENT)		// 수용 가능할때
		{
			printf("|| ★ %d번 손님이 (IP:%s, 소켓 번호:%d) 입장하였습니다.\n", number + 1, inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));
			_beginthread(recv_client, 0, &client_sock[number]);
			Sleep(10);
			
		}

		else								// 수용 인원 초과
		{
			addrsize = sizeof(client_info);
			if (client_sock[number] == INVALID_SOCKET)
			{
				printf("accept() 오류입니다.\n");
				closesocket(client_sock[number]);
				closesocket(server);
				WSACleanup();
				return 1;
			}

			ret = send(client_sock[number], user_full, sizeof(user_full), 0);
		}
		closesocket(client_sock[number]);
	}
	return 0;
}

void recv_client(void * kim)
{
	// 접속 성공
	WaitForSingleObject(hMutex, INFINITE);
	client_num++;																	// 클라이언트 접속자 증가
	number++;																	// 클라이언트 번호 증가
	printf("|| ♬ 수용 가능 인원이 %d명 남았습니다.\n", MAX_CLIENT - client_num);		// 갯수로 판단

	ReleaseMutex(hMutex);

	char user[50] = { 0 };														// accept 된 소켓에게 줄 버퍼 생성
	char buff[1024] = { 0 };
	int ret, i;


	_itoa(number, user, 10);														// 클라이언트 번호
	strcat(user, user_hi);															// 정상 환영 메시지 환영
	ret = send(*(SOCKET*)kim, user, sizeof(user), 0);								// 전송

	while (ret != SOCKET_ERROR || ret != INVALID_SOCKET)
	{
		ret = recv(*(SOCKET*)kim, buff, 1024, 0);									// 클라이언트의 메시지를 받음

		// broadcast 부분
		for (i = 0; i < ALLOW; i++)												
		{							
			// 받은 클라이언트 소켓의 메모리 주소와 보내는 클라이언트 소켓 메모리 주소가 다를때만 전송
			WaitForSingleObject(hMutex, INFINITE);
			if (((unsigned*)&client_sock[i] != (SOCKET*)kim))
			{
				send(client_sock[i], buff, strlen(buff), 0);
			}
			ReleaseMutex(hMutex);
		}

		// 서버 창에 손님간에 대화 기록 남김
		if (strlen(buff) != 0) 
			printf("|| ▶(크기:%d) %s", strlen(buff), buff);

		memset(buff, 0, 1024);

	}

	// 접속된 소켓이 연결을 끊었을 때
	WaitForSingleObject(hMutex, INFINITE);
	client_num--;
	printf("|| ♠손님이 나갔습니다.\n || 수용 가능 인원 %d명입니다.\n", MAX_CLIENT - client_num);
	number--;
	ReleaseMutex(hMutex);

	closesocket(*(int*)kim);

	return;
}