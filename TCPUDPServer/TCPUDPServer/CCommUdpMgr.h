#pragma once

#include <afxsock.h>
#include <vector>
#include <thread>
#include <mutex>

#define ENUM2STR(p)		#p

typedef void (*CallbackClassFunction)(LPVOID, LPVOID);

struct UDPClientInfo {
	sockaddr_in addr;  // 클라이언트 주소 정보
	std::string ip;    // 클라이언트 IP
	int port;          // 클라이언트 포트
	std::chrono::steady_clock::time_point pingTime;
	std::chrono::steady_clock::time_point pongTime;
};

class CCommUdpMgr
{
private:
	CString m_strConnectIP;
	int m_nConnectPort;

	SOCKET m_serverSocket; // 서버 소켓
	std::vector<UDPClientInfo> clients;
	std::mutex clientMutex;

public:
	CCommUdpMgr();
	virtual ~CCommUdpMgr(void);

public:
	//IP, Port 설정
	void SetNetInfo(bool bRunMode, CString strIP, long nPort)
	{
		m_strConnectIP = strIP;
		m_nConnectPort = nPort;
	}

	void Initialize();
	void AcceptClient(SOCKET udpSocket);
	void AddClientInfo(const sockaddr_in& clientAddr);
	void SendPingPong();
	void CheckPingPong();

	//Callback
	CallbackClassFunction m_CallbackFunc_SendLogUDP;
	LPVOID m_CallbackPtr_SendLogUDP;

	void SetCallbackFunction_SendLogUDP(LPVOID callbackPtr, CallbackClassFunction callbackFunc)
	{
		m_CallbackPtr_SendLogUDP = callbackPtr;
		m_CallbackFunc_SendLogUDP = callbackFunc;
	}
};

