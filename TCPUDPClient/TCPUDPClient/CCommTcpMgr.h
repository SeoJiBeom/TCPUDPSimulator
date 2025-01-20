#pragma once
#include <afxsock.h>
#include <vector>
#include <thread>
#include <mutex>

#define ENUM2STR(p)		#p

typedef void (*CallbackClassFunction)(LPVOID, LPVOID);

enum enumCOMM
{
	COMM_UNKNOWN = -1,
	COMM_DISCONNECT,
	COMM_CONNECT,
	COMM_SEND_CHAT,

	COMM_TYPE_COUNT,
};

struct ClientInfo {
	SOCKET clientSocket;  // 클라이언트 소켓
	std::thread clientThread;  // 스레드
};

class CCommTcpMgr
{
private:
	CString m_strServerIP;
	int m_nServerPort;

	SOCKET m_clientSocket; // 클라이언트 소켓
	std::vector<ClientInfo> m_clients; // 클라이언트 정보 목록
	std::mutex m_clientMutex; // 멀티스레드 동기화를 위한 뮤텍스

	CString	m_strQueueMessage;

public:
	CCommTcpMgr();
	virtual ~CCommTcpMgr(void);

public:
	//IP, Port 설정
	void SetNetInfo(CString strIP, long nPort)
	{
		m_strServerIP = strIP;
		m_nServerPort = nPort;
	}

	//Client 연결
	bool Initialize(CString strClientID);
	void ReceiveMessages(SOCKET clientSocket);

	//데이터 파싱
	std::vector<CString> GetCommand(CString& strData);
	long ParseMsg(CString strMsg);
	CString GetStringCMDID(int iCmdID);

	//소켓 연결 관리
	void DisconnectClient(); //직접 연결 해제
	void DisconnectServer(); //서버에서 연결 해제

	//Chat Data
	void SendChatData(CString strID, CString strChatData);
	void PrintChatData(CString strID, CString strChatData);

	//Callback
	CallbackClassFunction m_CallbackFunc_DisconnectServer;
	LPVOID m_CallbackPtr_DisconnectServer;
	CallbackClassFunction m_CallbackFunc_PrintChatData;
	LPVOID m_CallbackPtr_PrintChatData;

	void SetCallbackFunction_DisconnectServer(LPVOID callbackPtr, CallbackClassFunction callbackFunc)
	{
		m_CallbackPtr_DisconnectServer = callbackPtr;
		m_CallbackFunc_DisconnectServer = callbackFunc;
	}
	void SetCallbackFunction_PrintChatData(LPVOID callbackPtr, CallbackClassFunction callbackFunc)
	{
		m_CallbackPtr_PrintChatData = callbackPtr;
		m_CallbackFunc_PrintChatData = callbackFunc;
	}
};