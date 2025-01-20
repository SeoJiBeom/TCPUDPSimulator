#include "pch.h"
#include "CCommUdpMgr.h"

#pragma comment(lib, "ws2_32.lib")
CCommUdpMgr::CCommUdpMgr()
{
    m_strConnectIP = _T("127.0.0.1");
    m_nConnectPort = 9001;
}

CCommUdpMgr::~CCommUdpMgr(void)
{
    closesocket(m_clientSocket);
}

void CCommUdpMgr::Initialize()
{
    // ���� �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        AfxMessageBox(_T("���� �ʱ�ȭ ����"));
        return;
    }

    // UDP ���� ����
    m_clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_clientSocket == INVALID_SOCKET) {
        AfxMessageBox(_T("UDP ���� ���� ����"));
        WSACleanup();
        return;
    }

    // ���� �ּ� ����
    sockaddr_in serverAddr = { 0 };
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, CT2A(m_strConnectIP), &serverAddr.sin_addr);
    serverAddr.sin_port = htons(m_nConnectPort);

    const char* message = "Connect";
    sendto(m_clientSocket, message, strlen(message), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

    // ���� ������ ����
    std::thread recvThread(&CCommUdpMgr::ReceiveData, this);
    recvThread.detach(); // ���� ������� �и�
}

void CCommUdpMgr::ReceiveData()
{
    sockaddr_in fromAddr;
    int fromAddrSize = sizeof(fromAddr);

    while (true) {
        char buffer[1024];
        ZeroMemory(buffer, sizeof(buffer));

        // ������ ����
        int bytesReceived = recvfrom(m_clientSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&fromAddr, &fromAddrSize);

        // ���� ���� �Ǵ� ���� üũ
        if (bytesReceived == 0) {   // ������ ������ �������� ��
            break;  // ���� �����Ͽ� ������ ����
        }
        else if (bytesReceived < 0) {
            int errorCode = WSAGetLastError();  // ���� �ڵ� ��������
            if (errorCode == 10054) {
                // ������ ������ ������ ���
                break;
            }
            break;  // ���� �����Ͽ� ������ ����
        }

        // ������ ������ ó��
        buffer[bytesReceived] = '\0';
        CString receivedMessage = CString(buffer);

        if (receivedMessage == _T("PING")) {
            // ������ PONG �޽��� ����
            const char* pongMessage = "PONG";
            int result = sendto(m_clientSocket, pongMessage, strlen(pongMessage), 0, (sockaddr*)&fromAddr, fromAddrSize);
            if (result == SOCKET_ERROR) {
                int errorCode = WSAGetLastError();
                CString errorMessage;
                errorMessage.Format(_T("PONG ���� ����: %d"), errorCode);
                AfxMessageBox(errorMessage);
            }
        }
    }

    // ���� ����
    closesocket(m_clientSocket);
    WSACleanup();
}