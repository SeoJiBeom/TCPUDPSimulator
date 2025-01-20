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
    // 소켓 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        AfxMessageBox(_T("소켓 초기화 실패"));
        return;
    }

    // UDP 소켓 생성
    m_clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_clientSocket == INVALID_SOCKET) {
        AfxMessageBox(_T("UDP 소켓 생성 실패"));
        WSACleanup();
        return;
    }

    // 서버 주소 설정
    sockaddr_in serverAddr = { 0 };
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, CT2A(m_strConnectIP), &serverAddr.sin_addr);
    serverAddr.sin_port = htons(m_nConnectPort);

    const char* message = "Connect";
    sendto(m_clientSocket, message, strlen(message), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

    // 수신 스레드 시작
    std::thread recvThread(&CCommUdpMgr::ReceiveData, this);
    recvThread.detach(); // 메인 스레드와 분리
}

void CCommUdpMgr::ReceiveData()
{
    sockaddr_in fromAddr;
    int fromAddrSize = sizeof(fromAddr);

    while (true) {
        char buffer[1024];
        ZeroMemory(buffer, sizeof(buffer));

        // 데이터 수신
        int bytesReceived = recvfrom(m_clientSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&fromAddr, &fromAddrSize);

        // 연결 종료 또는 오류 체크
        if (bytesReceived == 0) {   // 서버가 연결을 종료했을 때
            break;  // 루프 종료하여 스레드 종료
        }
        else if (bytesReceived < 0) {
            int errorCode = WSAGetLastError();  // 오류 코드 가져오기
            if (errorCode == 10054) {
                // 서버와 연결이 끊겼을 경우
                break;
            }
            break;  // 루프 종료하여 스레드 종료
        }

        // 수신한 데이터 처리
        buffer[bytesReceived] = '\0';
        CString receivedMessage = CString(buffer);

        if (receivedMessage == _T("PING")) {
            // 서버로 PONG 메시지 전송
            const char* pongMessage = "PONG";
            int result = sendto(m_clientSocket, pongMessage, strlen(pongMessage), 0, (sockaddr*)&fromAddr, fromAddrSize);
            if (result == SOCKET_ERROR) {
                int errorCode = WSAGetLastError();
                CString errorMessage;
                errorMessage.Format(_T("PONG 전송 실패: %d"), errorCode);
                AfxMessageBox(errorMessage);
            }
        }
    }

    // 소켓 종료
    closesocket(m_clientSocket);
    WSACleanup();
}