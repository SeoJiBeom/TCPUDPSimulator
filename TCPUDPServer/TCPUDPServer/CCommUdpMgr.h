#pragma once

#include <afxsock.h>
#include <vector>
#include <thread>
#include <mutex>

#define ENUM2STR(p)		#p

typedef void (*CallbackClassFunction)(LPVOID, LPVOID);

struct UDPClientInfo {
	sockaddr_in addr;  // Ŭ���̾�Ʈ �ּ� ����
	std::string ip;    // Ŭ���̾�Ʈ IP
	int port;          // Ŭ���̾�Ʈ ��Ʈ
	std::chrono::steady_clock::time_point pingTime;
	std::chrono::steady_clock::time_point pongTime;
};

class CCommUdpMgr
{
private:
	CString m_strConnectIP;
	int m_nConnectPort;

	SOCKET m_serverSocket; // ���� ����
	std::vector<UDPClientInfo> clients;
	std::mutex clientMutex;

public:
	CCommUdpMgr();
	virtual ~CCommUdpMgr(void);

public:
	//IP, Port ����
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

