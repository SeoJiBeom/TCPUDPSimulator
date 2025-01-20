#include "pch.h"
#include "CCommUdpMgr.h"
#include "CCentralMgr.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PING_INTERVAL 20 // 15초마다 핑 전송
#define RESPONSE_TIMEOUT 10 // 응답 제한 시간 (10초)

CCommUdpMgr::CCommUdpMgr()
{
    m_strConnectIP = _T("127.0.0.1");
    m_nConnectPort = 9001;
}

CCommUdpMgr::~CCommUdpMgr(void)
{
    closesocket(m_serverSocket);
}

void CCommUdpMgr::Initialize()
{
    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }

    // UDP 소켓 생성
    m_serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // 브로드캐스트 옵션 설정
    BOOL enableBroadcast = TRUE;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_BROADCAST, (char*)&enableBroadcast, sizeof(enableBroadcast)) < 0) {
        closesocket(m_serverSocket);
        WSACleanup();
        return;
    }

    // 대상 주소 설정
    sockaddr_in broadcastAddr = {};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(m_nConnectPort);
    CStringA strIP(m_strConnectIP);  // CStringA를 사용하여 변환
    inet_pton(AF_INET, strIP, &broadcastAddr.sin_addr);

    if (bind(m_serverSocket, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        closesocket(m_serverSocket);
        WSACleanup();
        return;
    }

    // 클라이언트 수신 스레드 시작
    std::thread recvThread(&CCommUdpMgr::AcceptClient, this, m_serverSocket);
    recvThread.detach();  

    std::thread pingThread(&CCommUdpMgr::SendPingPong, this);
    pingThread.detach();  

}

void CCommUdpMgr::AcceptClient(SOCKET udpSocket)
{
    sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char buffer[1024];

    while (true) {
        int receivedBytes = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
            (sockaddr*)&clientAddr, &clientAddrLen);
        if (receivedBytes > 0) {
            buffer[receivedBytes] = '\0';

            if (std::string(buffer) == "PONG") 
            {
                // 클라이언트 벡터에서 IP와 포트를 비교하여 PONG 시간을 업데이트
                std::lock_guard<std::mutex> lock(clientMutex); // 동기화를 위한 mutex
                for (auto& client : clients) {
                    if (client.addr.sin_addr.s_addr == clientAddr.sin_addr.s_addr &&
                        client.addr.sin_port == clientAddr.sin_port) {
                        client.pongTime = std::chrono::steady_clock::now(); // PONG 시간 업데이트

                        int nPort = ntohs(clientAddr.sin_port);
                        CString logMessage;
                        logMessage.Format(_T("[Pong]클라이언트 [%d] 수신"), nPort);
                        if (m_CallbackFunc_SendLogUDP) {
                            m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                        }

                        break; // 매칭된 클라이언트를 찾으면 루프 종료
                    }
                }
            }
            else
            {
                // PONG이 아닌 경우 (예: 클라이언트 등록)
                AddClientInfo(clientAddr);
            }
        }
        else if (receivedBytes == SOCKET_ERROR) 
        {
            int errorCode = WSAGetLastError();
           /* CString logMessage;
            logMessage.Format(_T("Recv Error Code : %d"), errorCode);
            if (m_CallbackFunc_SendLogUDP) {
                m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
            }*/
        }
    }
}

void CCommUdpMgr::AddClientInfo(const sockaddr_in& clientAddr) 
{
    std::lock_guard<std::mutex> lock(clientMutex);

    // IP 주소를 문자열로 변환
    char ipBuffer[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(clientAddr.sin_addr), ipBuffer, sizeof(ipBuffer)) == nullptr) {
        return; // IP 변환 실패 시 함수 종료
    }
    std::string ip(ipBuffer); // std::string으로 변환

    // 포트를 호스트 바이트 순서로 변환
    int port = ntohs(clientAddr.sin_port);

    // 이미 존재하는지 확인
    for (const auto& client : clients) {
        if (client.ip == ip && client.port == port) {
            return; // 중복된 클라이언트는 추가하지 않음
        }
    }

    // 새로운 클라이언트 추가
    std::chrono::steady_clock::time_point pingpongTime = std::chrono::steady_clock::now();
    clients.push_back({ clientAddr, ip, port, pingpongTime , pingpongTime });
}

void CCommUdpMgr::SendPingPong()
{
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(PING_INTERVAL));

        auto currentTime = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(clientMutex);

            for (auto it = clients.begin(); it != clients.end();) {
                const char* pingMessage = "PING";
                int result = sendto(m_serverSocket, pingMessage, strlen(pingMessage), 0,
                    (sockaddr*)&it->addr, sizeof(it->addr));
                if (result == SOCKET_ERROR) {
                    int errorCode = WSAGetLastError();
                    if (errorCode == WSAECONNRESET) {
                        // 클라이언트가 연결을 끊었을 경우
                        it = clients.erase(it);  // 클라이언트 목록에서 삭제
                        continue;
                    }
                    else {
                        ++it;  // 계속해서 다음 클라이언트로 넘어가도록 함
                        continue;
                    }
                }

                it->pingTime = currentTime;  // 핑 전송 시간 기록
                ++it;  // 정상적으로 핑을 보냈다면 다음 클라이언트로 넘어감
            }
            CString logMessage;
            logMessage.Format(_T("[Ping]송신 클라이언트 수(%d)"), clients.size());
            if (m_CallbackFunc_SendLogUDP)
            {
                m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
            }

            std::thread pongThread(&CCommUdpMgr::CheckPingPong, this);
            pongThread.detach();
        }
    }
}

void CCommUdpMgr::CheckPingPong()
{
    // 10초 대기
    std::this_thread::sleep_for(std::chrono::seconds(RESPONSE_TIMEOUT));

    CString logMessage;
    {
        std::lock_guard<std::mutex> lock(clientMutex);

        for (auto it = clients.begin(); it != clients.end(); ) {
            // pingTime과 pongTime의 차이를 계산
            auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(it->pingTime - it->pongTime).count();

            if (timeDiff > RESPONSE_TIMEOUT) {
                // 로그 출력
                char ipStr[INET6_ADDRSTRLEN]; // IP 주소를 저장할 문자열
                if (inet_ntop(AF_INET, &it->addr.sin_addr, ipStr, sizeof(ipStr)) == nullptr) {
                    logMessage.Format(_T("[Ping]클라이언트 IP 변환 오류"));
                    if (m_CallbackFunc_SendLogUDP) {
                        m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                    }
                }
                else {
                    CString ipStrConverted(ipStr);
                    int nPort = ntohs(it->addr.sin_port);
                    logMessage.Format(_T("[Ping]클라이언트 [%s:%d] 응답 없음"), ipStrConverted, nPort);
                    if (m_CallbackFunc_SendLogUDP) {
                        m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                    }
                }
                CString ipStrConverted(ipStr);
                int nPort = ntohs(it->addr.sin_port);
                logMessage.Format(_T("[Ping]클라이언트 [%s:%d] 목록 제거 "), ipStrConverted, nPort);
                if (m_CallbackFunc_SendLogUDP) {
                    m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                }

                // 클라이언트 목록에서 제거 후 이터레이터 업데이트
                it = clients.erase(it);
            }
            else {
                // 조건에 맞지 않으면 다음 클라이언트로 이동
                ++it;
            }
        }
    }
}