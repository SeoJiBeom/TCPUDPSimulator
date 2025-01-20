#include "pch.h"
#include "CCommTcpMgr.h"
#include "CCentralMgr.h"
#include <winsock2.h>

CCommTcpMgr::CCommTcpMgr()
{
    m_strServerIP = _T("127.0.0.1");
    m_nServerPort = 9000;
}

CCommTcpMgr::~CCommTcpMgr(void)
{
    closesocket(m_clientSocket);
}

bool CCommTcpMgr::Initialize(CString strClientID)
{
    // Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    // ���� ����
    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_clientSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    CStringA strServerIP(m_strServerIP);  // CStringA�� ��ȯ

    PCSTR lpstr = strServerIP;  // CStringA�� PCSTR�� �ڵ� ��ȯ��

    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, lpstr, &serverAddr.sin_addr) <= 0) {
        AfxMessageBox(_T("IP �ּ� ��ȯ ����"));
        return false;
    }
    serverAddr.sin_port = htons(m_nServerPort);

    // ���� ����
    if (connect(m_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_clientSocket);
        WSACleanup();
        return false;
    }

    // Ŭ���̾�Ʈ ID �Է� �� ������ ����
    CString strPacket;
    strPacket.Format(_T("COMM_CONNECT^%s$"), strClientID);
    CStringA strID(strPacket);

    if (send(m_clientSocket, strID.GetBuffer(), strID.GetLength(), 0) == SOCKET_ERROR) {
        closesocket(m_clientSocket);
        WSACleanup();
        return false;
    }

    // ���� ������ ����
    std::thread recvThread(&CCommTcpMgr::ReceiveMessages, this, m_clientSocket);

    // ���� �����带 �и��Ͽ� ���� �����尡 ��� ����ǵ��� ��
    recvThread.detach();

    return true;
}

void CCommTcpMgr::ReceiveMessages(SOCKET clientSocket)
{
    char buffer[1024];  // �����κ��� ������ �����͸� ������ ����

    while (true) {
        // �����κ��� ������ ����
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        // ���� ���� �Ǵ� ���� üũ
        if (bytesReceived == 0) {
            // ������ ������ �������� ��
            DisconnectServer();
            break;  // ���� �����Ͽ� ������ ����
        }
        else if (bytesReceived < 0) {
            // ���� �߻���
            int errorCode = WSAGetLastError();  // ���� �ڵ� ��������
            if (errorCode == 10054)
            {
                // Ŭ���̾�Ʈ�� ������ ������ ���
                DisconnectServer();  // Ŭ���̾�Ʈ ���� ����
                break;
            }
            break;  // ���� �����Ͽ� ������ ����
        }

        // ���ŵ� ������ ó��
        buffer[bytesReceived] = '\0';  // ���ڿ� ���� ǥ�� (�� ����)

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

                //List�� ä�� ���
                PrintChatData(strClientID, strChatData);
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
    // ������ �ݰ� ������ ����
    closesocket(clientSocket);
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
    case COMM_SEND_CHAT:                        strCMDID = ENUM2STR(COMM_SEND_CHAT);							break;
    default:									strCMDID = ENUM2STR(COMM_UNKNOWN);							break;
    }

    return strCMDID;
}

void CCommTcpMgr::DisconnectClient()
{
    closesocket(m_clientSocket);
}

void CCommTcpMgr::SendChatData(CString strID, CString strChatData)
{
    // Ŭ���̾�Ʈ ID �Է� �� ������ ����
    CString strPacket;
    strPacket.Format(_T("COMM_SEND_CHAT^%s,%s$"), strID, strChatData);
    CStringA strMessage(strPacket);

    if (send(m_clientSocket, strMessage.GetBuffer(), strMessage.GetLength(), 0) == SOCKET_ERROR) {
        closesocket(m_clientSocket);
        WSACleanup();
        return;
    }
}

void CCommTcpMgr::DisconnectServer()
{
    if (m_CallbackFunc_DisconnectServer)
    {
        m_CallbackFunc_DisconnectServer(m_CallbackPtr_DisconnectServer, nullptr);
    }
}

void CCommTcpMgr::PrintChatData(CString strID, CString strChatData)
{
    stCallbackChat stChat;
    stChat.strClientID = strID;
    stChat.strChatData = strChatData;

    if (m_CallbackFunc_PrintChatData)
    {
        m_CallbackFunc_PrintChatData(m_CallbackPtr_PrintChatData, &stChat);
    }
}