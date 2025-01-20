#pragma once
#include <afxsock.h>
#include <vector>
#include <thread>
#include <mutex>

typedef void (*CallbackClassFunction)(LPVOID, LPVOID);

class CCommUdpMgr
{
private:
	CString m_strConnectIP;
	int m_nConnectPort;

	SOCKET m_clientSocket; // 서버 소켓
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
	void ReceiveData();
};

