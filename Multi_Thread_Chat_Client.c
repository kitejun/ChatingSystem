/*
로컬 호스트 주소에 서버를 생성 / 복수의 클라이언트를 받은 후에
클라이언트가 받은 메시지를 다른 클라언트들에게 전송하는 기능을 가지고 있습니다.
*/

#include <stdio.h>
#include <string.h>
#include <winsock.h>
#include <process.h>

#pragma comment(lib,"wsock32.lib")

#define PORT 20000						// 사용할 포트는 20000
#define IP "127.0.0.1"						// 접속할 서버는 로컬 호스트

void recv_thread(void* kim);					// 스레드 수행 함수의 프로토 타입

int ret = 0;								// 리턴 값
SOCKET server;							// 소켓 값
HANDLE hMutex;							// 뮤텍스용

int main()
{
	// 서버 접속을 위한 소켓 생성
	SOCKADDR_IN server_info;

	// 윈속 초기화
	WSADATA wsd;
	char buff[1024];
	char num[1024];
	char num2[1024];
	char *t1 = " <", *t3 = "> ";

	// Welcome Screen
	printf("■■■■■■■■■■■■■■■■■■■■■■\n");
	printf("■                                        ■\n");
	printf("■     멀티 스레드를 이용한 멀티 채팅     ■\n");
	printf("■           클라이언트 (Client)          ■\n");
	printf("■                                        ■\n");
	printf("■■■■■■■■■■■■■■■■■■■■■■\n");

	// 뮤택스 초기화
	hMutex = CreateMutex(NULL, FALSE, NULL);
	if (!hMutex)
	{
		printf("☞ Mutex 오류입니다. ☜\n");
		return 0;

	}

	if (WSAStartup(MAKEWORD(1, 1), &wsd) != 0)
	{
		printf("☞ Winsock 오류입니다. ☜\n");
		return 0;
	}

	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server == SOCKET_ERROR)
	{
		printf("☞ Winsock 오류입니다. ☜\n");
		closesocket(server);
		WSACleanup();
		return 0;

	}

	server_info.sin_addr.s_addr = inet_addr(IP);
	server_info.sin_family = AF_INET;
	server_info.sin_port = htons(PORT);

	// 서버와 연결
	if (connect(server, (SOCKADDR_IN*)&server_info, sizeof(server_info)) == SOCKET_ERROR)
	{
		printf("☞ connect() 오류입니다. ☜\n");
		closesocket(server);
		WSACleanup();
		return 0;

	}

	

	// 서버에서 우선 주는 user 메시지 분석
	ret = recv(server, num, 1024, 0);

	// 가득 찼다고 메시지가 왔다면
	if (!strcmp("가득찼습니다.\n", num))
	{
		printf("♥ 인원이 꽉 찼습니다. ♥\n");
		closesocket(server);
		WSACleanup();
		return 0;					// 접속을 종료
	}
	printf("♥ 서버와 연결 성공 ♥\n");
	printf("★ %s 환영합니다 ★\n", num);

	// 정상 접속이 되면 스레드 작동 - 받는 메시지를 스레드로 실시간 수행
	_beginthread(recv_thread, 0, NULL);

	// 보내는 메시지는 스레드로 넣을 필요가 없습니다.
	while (ret != INVALID_SOCKET || ret != SOCKET_ERROR)
	{

		// 딜레이 : CPU 점유율 감소용
		Sleep(5);
		fgets(buff, 1024, stdin);
		sprintf(num2, "%s%s%s", t1, num, t3); // 빈 배열 num2 에 여러 문자열 집어넣기

		strcat(num2, buff);

		// 전송 결과 잘못 된 결과를 얻었을때 탈출
		if (ret == INVALID_SOCKET || ret == SOCKET_ERROR) break;

		ret = send(server, num2, strlen(num2), 0);

		// num2 초기화
		memset(num2, 0, 1024);

	}

	// 서버가 끊겼을 경우
	printf("♠ 서버와 연결이 끊겼습니다. ♠\n");
	closesocket(server);
	WSACleanup();

	return 0;
}

// 받는 스레드 부분
void recv_thread(void* kim)
{
	int ret_thread = 60000;
	char buff_thread[1024] = { 0 };

	// 스레드용 리턴 값이 우너하는 값이 아니면 받는 중에 서버와 통신이 끊겼다고 보고 나감
	while (ret_thread != INVALID_SOCKET || ret_thread != SOCKET_ERROR)
	{
		Sleep(5);	// CPU 점유률 100% 방지용

		// 서버에서 주는 메시지를 실시간으로 기다렸다가 받습니다.
		ret_thread = recv(server, buff_thread, sizeof(buff_thread), 0);

		// 서버에서 받는 작업을 한 결과 비정상일 때 탈출
		if (ret_thread == INVALID_SOCKET || ret_thread == SOCKET_ERROR)
			break;

		// 정상적으로 받은 버퍼를 출력
		printf("▷ (크기:%d) 메시지 받음 : %s", strlen(buff_thread), buff_thread);
		memset(buff_thread, 0, 1024);	// 받은 버퍼를 초기와
	}

	// 작업이 끝난 소켓을 무효화시킴
	WaitForSingleObject(hMutex, 100L);
	ret = INVALID_SOCKET;
	ReleaseMutex(hMutex);

	return;
}