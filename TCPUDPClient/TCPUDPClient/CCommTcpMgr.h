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
	SOCKET clientSocket;  // Ŭ���̾�Ʈ ����
	std::thread clientThread;  // ������
};

class CCommTcpMgr
{
private:
	CString m_strServerIP;
	int m_nServerPort;

	SOCKET m_clientSocket; // Ŭ���̾�Ʈ ����
	std::vector<ClientInfo> m_clients; // Ŭ���̾�Ʈ ���� ���
	std::mutex m_clientMutex; // ��Ƽ������ ����ȭ�� ���� ���ؽ�

	CString	m_strQueueMessage;

public:
	CCommTcpMgr();
	virtual ~CCommTcpMgr(void);

public:
	//IP, Port ����
	void SetNetInfo(CString strIP, long nPort)
	{
		m_strServerIP = strIP;
		m_nServerPort = nPort;
	}

	//Client ����
	bool Initialize(CString strClientID);
	void ReceiveMessages(SOCKET clientSocket);

	//������ �Ľ�
	std::vector<CString> GetCommand(CString& strData);
	long ParseMsg(CString strMsg);
	CString GetStringCMDID(int iCmdID);

	//���� ���� ����
	void DisconnectClient(); //���� ���� ����
	void DisconnectServer(); //�������� ���� ����

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