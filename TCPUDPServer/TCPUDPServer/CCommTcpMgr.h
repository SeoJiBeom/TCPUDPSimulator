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

struct TCPClientInfo {
	SOCKET clientSocket;  // Ŭ���̾�Ʈ ����
	std::thread clientThread;  // ������
};

class CCommTcpMgr
{
private:
	CString m_strConnectIP;
	int m_nConnectPort;

	SOCKET m_serverSocket; // ���� ����
	std::vector<TCPClientInfo> m_clients; // Ŭ���̾�Ʈ ���� ���
	std::mutex m_clientMutex; // ��Ƽ������ ����ȭ�� ���� ���ؽ�

	CString	m_strQueueMessage;

public:
	CCommTcpMgr();
	virtual ~CCommTcpMgr(void);

public:
	//IP, Port ����
	void SetNetInfo(bool bRunMode, CString strIP, long nPort)
	{
		m_strConnectIP = strIP;
		m_nConnectPort = nPort;
	}
	
	//Client ����
	void Initialize();
	void AcceptClient();
	void HandleClient(SOCKET clientSocket);

	//������ �Ľ�
	std::vector<CString> GetCommand(CString& strData);
	long ParseMsg(CString strMsg);
	CString GetStringCMDID(int iCmdID);

	//Client ����Ʈ
	void AddClientInfo(CString id, CString ip, int port);
	void DeleteClientInfo(SOCKET clientSocket, CString ip, int port);
	void BroadCastChatData(CString strID, CString strChatData);

	//Callback
	CallbackClassFunction m_CallbackFunc_ConnectClient;
	LPVOID m_CallbackPtr_ConnectClient;
	CallbackClassFunction m_CallbackFunc_DisconnectClient;
	LPVOID m_CallbackPtr_DisconnectClient;
	CallbackClassFunction m_CallbackFunc_SendLogTCP;
	LPVOID m_CallbackPtr_SendLogTCP;

	void SetCallbackFunction_ConnectClient(LPVOID callbackPtr, CallbackClassFunction callbackFunc)
	{
		m_CallbackPtr_ConnectClient = callbackPtr;
		m_CallbackFunc_ConnectClient = callbackFunc;
	}

	void SetCallbackFunction_DisconnectClient(LPVOID callbackPtr, CallbackClassFunction callbackFunc)
	{
		m_CallbackPtr_DisconnectClient = callbackPtr;
		m_CallbackFunc_DisconnectClient = callbackFunc;
	}

	void SetCallbackFunction_SendLogTCP(LPVOID callbackPtr, CallbackClassFunction callbackFunc)
	{
		m_CallbackPtr_SendLogTCP = callbackPtr;
		m_CallbackFunc_SendLogTCP = callbackFunc;
	}
};

