#include "pch.h"
#include "CCommTcpMgr.h"
#include "CCentralMgr.h"
#include <winsock2.h>

CCommTcpMgr::CCommTcpMgr()
{
    m_strConnectIP = _T("127.0.0.1");
    m_nConnectPort = 9000;
}

CCommTcpMgr::~CCommTcpMgr(void)
{
    closesocket(m_serverSocket);
}

void CCommTcpMgr::Initialize()
{
    CString logMessage;

    // ���� ���̺귯�� �ʱ�ȭ
    if (!AfxSocketInit()) {
        logMessage.Format(_T("���� �ʱ�ȭ ����"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // ���� ���� ����
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        logMessage.Format(_T("���� ���� ���� ����"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // ���� �ּ� ����
    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    CStringA strIP(m_strConnectIP);  // CStringA�� ����Ͽ� ��ȯ
    if (inet_pton(AF_INET, strIP, &serverAddr.sin_addr) <= 0) {
        logMessage.Format(_T("IP �ּ� ��ȯ ����"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }
    serverAddr.sin_port = htons(m_nConnectPort);

    // ���� ���ε�
    if (bind(m_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();  // ���� �ڵ� ��������
        char msg[256];
        snprintf(msg, sizeof(msg), "���� ���ε� ����, ���� �ڵ�: %d", error);
        logMessage = CA2T(msg);
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // ���� ���
    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        logMessage.Format(_T("���� ���� ����"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // ���� ���� ������ ����
    std::thread acceptThread(&CCommTcpMgr::AcceptClient, this);
    acceptThread.detach(); // ���� ������� �и�
}

void CCommTcpMgr::AcceptClient()
{
    while (true) {
        // Ŭ���̾�Ʈ ���� ��û ����
        CString logMessage;
        SOCKET clientSocket = accept(m_serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            logMessage.Format(_T("Ŭ���̾�Ʈ ���� ���� ����"));
            if (m_CallbackFunc_SendLogTCP)
            {
                m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
            }
            continue;
        }

        
        logMessage.Format(_T("�� Ŭ���̾�Ʈ�� ����Ǿ����ϴ�!"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }

        // Ŭ���̾�Ʈ ���� ���� �� ������ ����
        std::lock_guard<std::mutex> lock(m_clientMutex);
        m_clients.push_back({ clientSocket, std::thread(&CCommTcpMgr::HandleClient, this, clientSocket) });
        m_clients.back().clientThread.detach(); // ������ �и�
    }
}

void CCommTcpMgr::HandleClient(SOCKET clientSocket)
{
    char buffer[1024];
    CString logMessage;

    //IP, Port ����
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(clientSocket, (sockaddr*)&clientAddr, &addrLen);

    char clientIP[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP)) == NULL) {
        return;
    }

    CString strClientIP(clientIP);
    int nPort = ntohs(clientAddr.sin_port);

    while (true) {
        // Ŭ���̾�Ʈ�κ��� ������ ����
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == 0) {
            // Ŭ���̾�Ʈ�� ������ ������ ���
            DeleteClientInfo(clientSocket, strClientIP, nPort);  // Ŭ���̾�Ʈ ���� ����
            logMessage.Format(_T("Ŭ���̾�Ʈ�� ������ �����߽��ϴ�."));
            if (m_CallbackFunc_SendLogTCP)
            {
                m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
            }
            break;
        }
        else if (bytesReceived < 0) {
            // ���� �߻� ��
            int errorCode = WSAGetLastError();  // ���� �ڵ� ��������
            if (errorCode == 10054)
            {
                // Ŭ���̾�Ʈ�� ������ ������ ���
                DeleteClientInfo(clientSocket, strClientIP, nPort);  // Ŭ���̾�Ʈ ���� ����
                break;
            }
            CString errorMsg;
            errorMsg.Format(_T("recv() ���� �߻�, ���� �ڵ�: %d"), errorCode);
            logMessage.Format(_T("%s"), errorMsg);
            if (m_CallbackFunc_SendLogTCP)
            {
                m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
            }
            break;
        }
        else {
            // ���������� �����͸� ���� ���
            buffer[bytesReceived] = '\0';  // ���ڿ� ���� ǥ�� (�� ����)
            // ���ŵ� �����͸� ó���ϴ� �ڵ� (��: ȭ�� ���)
        }

        CString strMessage((char*)buffer);

        m_strQueueMessage += strMessage;

        //---------------------------------------------------------------------------
        // ��Ŷ ������ ���信 ���� ó���� �ؾ� �Ѵ�.
        // socket �� �����Ͱ� �����ϴµ� 2�� �̻� �پ 1���� event �� ���´�.
        // ��� ��Ŷ�� ���ؼ� command ������ �и��ؼ� �����ؾ� �Ѵ�.
        //---------------------------------------------------------------------------
        std::vector<CString> vecPacketCommandPool = GetCommand(m_strQueueMessage);

		for (size_t i = 0; i < vecPacketCommandPool.size(); ++i)
		{
			const CString strCommand = vecPacketCommandPool[i];

			const int nCommand = ParseMsg(strCommand);

			// get packet data and do job
			switch (nCommand)
			{
            case COMM_CONNECT:
            {
                CString strClientID = _T("");
                CString mainstr = strCommand;
                CString substr = _T("COMM_CONNECT");
                mainstr.Delete(0, substr.GetLength());

                LPCTSTR messageStr = mainstr;
                int start_pos = _tcsstr(messageStr, _T("^")) - messageStr;
                int end_pos = _tcsstr(messageStr, _T("$")) - messageStr;

                if (start_pos >= 0 && end_pos >= 0) {
                    // '^' �������� '$' ���������� �κ� ���ڿ� ����
                    CString dataPart = mainstr.Mid(start_pos + 1, end_pos - start_pos - 1);

                    // ','�� �������� ������ ����
                    CString token;
                    int pos = 0;
                    std::vector<CString> tokens;

                    while ((pos = dataPart.Find(_T(','))) != -1) {
                        token = dataPart.Left(pos);
                        tokens.push_back(token);
                        dataPart = dataPart.Mid(pos + 1);
                    }

                    // ������ ��ū �߰�
                    tokens.push_back(dataPart);

                    // ����� �����͸� ������ ����
                    if (tokens.size() == 1) {
                        strClientID = tokens[0];
                    }
                }

                //����Ʈ �߰�
                AddClientInfo(strClientID, strClientIP, nPort);
            }
            break;
            case COMM_SEND_CHAT:
            {
                CString strClientID = _T("");
                CString strChatData = _T("");
                CString mainstr = strCommand;
                CString substr = _T("COMM_SEND_CHAT");
                mainstr.Delete(0, substr.GetLength());

                LPCTSTR messageStr = mainstr;
                int start_pos = _tcsstr(messageStr, _T("^")) - messageStr;
                int end_pos = _tcsstr(messageStr, _T("$")) - messageStr;

                if (start_pos >= 0 && end_pos >= 0) {
                    // '^' �������� '$' ���������� �κ� ���ڿ� ����
                    CString dataPart = mainstr.Mid(start_pos + 1, end_pos - start_pos - 1);

                    // ','�� �������� ������ ����
                    CString token;
                    int pos = 0;
                    std::vector<CString> tokens;

                    while ((pos = dataPart.Find(_T(','))) != -1) {
                        token = dataPart.Left(pos);
                        tokens.push_back(token);
                        dataPart = dataPart.Mid(pos + 1);
                    }

                    // ������ ��ū �߰�
                    tokens.push_back(dataPart);

                    // ����� �����͸� ������ ����
                    if (tokens.size() == 2) {
                        strClientID = tokens[0];
                        strChatData = tokens[1];
                    }
                }

                //��� Ŭ���̾�Ʈ�� ä�� ���� ����
                BroadCastChatData(strClientID, strChatData);
            }
            break;
            case COMM_DISCONNECT:
            {
            }
            break;
			default:
			{
				//SendPacketMsg(COMM_UNKNOWN);
			}
			break;
			}
		}
    }
}

std::vector<CString> CCommTcpMgr::GetCommand(CString& strData)
{
    CString strMessage = strData;

    std::vector<CString> vecResult;

    while (1)
    {
        if (strMessage.IsEmpty() == true)
            break;

        const int ipos = strMessage.Find('$');
        if (ipos < 0)
            break;

        CString strParam = strMessage.Left(ipos + 1);
        if (strParam.IsEmpty() == true)
            break;

        vecResult.push_back(strParam);

        strMessage.Replace(strParam, _T(""));
    }

    strData = strMessage;

    return 	vecResult;
}

long CCommTcpMgr::ParseMsg(CString strMsg)
{
    strMsg.Replace(_T("$"), _T(""));
    strMsg.Replace(_T(" "), _T(""));

    if (strMsg.IsEmpty() == false)
    {
        CString strParam;
        AfxExtractSubString(strParam, strMsg, 0, '^');

        for (long n = 0; n < COMM_TYPE_COUNT; ++n)
        {
            CString strComm = GetStringCMDID(n);

            if (strParam.CompareNoCase(strComm) == 0)
            {
                return n;
            }
        }
    }

    return COMM_UNKNOWN;
}

CString CCommTcpMgr::GetStringCMDID(int iCmdID)
{
    CString strCMDID;

    switch (iCmdID)
    {
    case COMM_UNKNOWN:							strCMDID = ENUM2STR(COMM_UNKNOWN);							break;
    case COMM_DISCONNECT:						strCMDID = ENUM2STR(COMM_DISCONNECT);						break;
    case COMM_CONNECT:							strCMDID = ENUM2STR(COMM_CONNECT);							break;
    case COMM_SEND_CHAT:						strCMDID = ENUM2STR(COMM_SEND_CHAT);							break;       
    default:									strCMDID = ENUM2STR(COMM_UNKNOWN);							break;
    }

    return strCMDID;
}

void CCommTcpMgr::AddClientInfo(CString id, CString ip, int port)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    CString timeStr;
    timeStr.Format(_T("%04d-%02d-%02d %02d:%02d:%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    stCallbackClient stClient;
    stClient.strClientID = id;
    stClient.strClientIP = ip;
    stClient.nPort = port;
    stClient.strConnectTime = timeStr;

    if (m_CallbackFunc_ConnectClient)
    {
        m_CallbackFunc_ConnectClient(m_CallbackPtr_ConnectClient, &stClient);
    }
}

void CCommTcpMgr::DeleteClientInfo(SOCKET clientSocket, CString ip, int port)
{
    std::lock_guard<std::mutex> lock(m_clientMutex);

    auto it = std::find_if(m_clients.begin(), m_clients.end(),
        [clientSocket](const TCPClientInfo& client) { return client.clientSocket == clientSocket; });

    if (it != m_clients.end()) {
        closesocket(it->clientSocket);  // ���� �ݱ�
        m_clients.erase(it);  // ���Ϳ��� Ŭ���̾�Ʈ ����
    }

    stCallbackClient stClient;
    stClient.strClientIP = ip;
    stClient.nPort = port;

    if (m_CallbackFunc_DisconnectClient)
    {
        m_CallbackFunc_DisconnectClient(m_CallbackPtr_DisconnectClient, &stClient);
    }
}

void CCommTcpMgr::BroadCastChatData(CString strID, CString strChatData)
{
    std::lock_guard<std::mutex> lock(m_clientMutex);

    // ���Ϳ� �ִ� ��� Ŭ���̾�Ʈ���� �޽��� ����
    CString strPacket;
    strPacket.Format(_T("COMM_SEND_CHAT^%s,%s$"), strID, strChatData);
    CStringA strMessage(strPacket);

    for (auto& client : m_clients) {
        if (send(client.clientSocket, strMessage, strlen(strMessage), 0) == SOCKET_ERROR) {
            // ���� �߻� �� �ش� Ŭ���̾�Ʈ�� ����
            sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);

            // Ŭ���̾�Ʈ�� IP�� ��Ʈ�� ��� ���� �Լ�
            if (getpeername(client.clientSocket, (sockaddr*)&clientAddr, &addrLen) == SOCKET_ERROR) {
                return;  // ���� �߻� �� �� ���ڿ� ��ȯ
            }

            // IP �ּҸ� ���ڿ��� ��ȯ
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
            CString clientIP(ipStr);  // char*���� CString���� ��ȯ

            // ��Ʈ ��ȣ ���
            int clientPort = ntohs(clientAddr.sin_port);

            DeleteClientInfo(client.clientSocket, clientIP, clientPort);
        }
    }
}