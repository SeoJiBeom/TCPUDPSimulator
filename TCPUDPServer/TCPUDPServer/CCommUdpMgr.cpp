#include "pch.h"
#include "CCommUdpMgr.h"
#include "CCentralMgr.h"
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PING_INTERVAL 20 // 15�ʸ��� �� ����
#define RESPONSE_TIMEOUT 10 // ���� ���� �ð� (10��)

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
    // Winsock �ʱ�ȭ
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }

    // UDP ���� ����
    m_serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // ��ε�ĳ��Ʈ �ɼ� ����
    BOOL enableBroadcast = TRUE;
    if (setsockopt(m_serverSocket, SOL_SOCKET, SO_BROADCAST, (char*)&enableBroadcast, sizeof(enableBroadcast)) < 0) {
        closesocket(m_serverSocket);
        WSACleanup();
        return;
    }

    // ��� �ּ� ����
    sockaddr_in broadcastAddr = {};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(m_nConnectPort);
    CStringA strIP(m_strConnectIP);  // CStringA�� ����Ͽ� ��ȯ
    inet_pton(AF_INET, strIP, &broadcastAddr.sin_addr);

    if (bind(m_serverSocket, (sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        closesocket(m_serverSocket);
        WSACleanup();
        return;
    }

    // Ŭ���̾�Ʈ ���� ������ ����
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
                // Ŭ���̾�Ʈ ���Ϳ��� IP�� ��Ʈ�� ���Ͽ� PONG �ð��� ������Ʈ
                std::lock_guard<std::mutex> lock(clientMutex); // ����ȭ�� ���� mutex
                for (auto& client : clients) {
                    if (client.addr.sin_addr.s_addr == clientAddr.sin_addr.s_addr &&
                        client.addr.sin_port == clientAddr.sin_port) {
                        client.pongTime = std::chrono::steady_clock::now(); // PONG �ð� ������Ʈ

                        int nPort = ntohs(clientAddr.sin_port);
                        CString logMessage;
                        logMessage.Format(_T("[Pong]Ŭ���̾�Ʈ [%d] ����"), nPort);
                        if (m_CallbackFunc_SendLogUDP) {
                            m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                        }

                        break; // ��Ī�� Ŭ���̾�Ʈ�� ã���� ���� ����
                    }
                }
            }
            else
            {
                // PONG�� �ƴ� ��� (��: Ŭ���̾�Ʈ ���)
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

    // IP �ּҸ� ���ڿ��� ��ȯ
    char ipBuffer[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(clientAddr.sin_addr), ipBuffer, sizeof(ipBuffer)) == nullptr) {
        return; // IP ��ȯ ���� �� �Լ� ����
    }
    std::string ip(ipBuffer); // std::string���� ��ȯ

    // ��Ʈ�� ȣ��Ʈ ����Ʈ ������ ��ȯ
    int port = ntohs(clientAddr.sin_port);

    // �̹� �����ϴ��� Ȯ��
    for (const auto& client : clients) {
        if (client.ip == ip && client.port == port) {
            return; // �ߺ��� Ŭ���̾�Ʈ�� �߰����� ����
        }
    }

    // ���ο� Ŭ���̾�Ʈ �߰�
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
                        // Ŭ���̾�Ʈ�� ������ ������ ���
                        it = clients.erase(it);  // Ŭ���̾�Ʈ ��Ͽ��� ����
                        continue;
                    }
                    else {
                        ++it;  // ����ؼ� ���� Ŭ���̾�Ʈ�� �Ѿ���� ��
                        continue;
                    }
                }

                it->pingTime = currentTime;  // �� ���� �ð� ���
                ++it;  // ���������� ���� ���´ٸ� ���� Ŭ���̾�Ʈ�� �Ѿ
            }
            CString logMessage;
            logMessage.Format(_T("[Ping]�۽� Ŭ���̾�Ʈ ��(%d)"), clients.size());
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
    // 10�� ���
    std::this_thread::sleep_for(std::chrono::seconds(RESPONSE_TIMEOUT));

    CString logMessage;
    {
        std::lock_guard<std::mutex> lock(clientMutex);

        for (auto it = clients.begin(); it != clients.end(); ) {
            // pingTime�� pongTime�� ���̸� ���
            auto timeDiff = std::chrono::duration_cast<std::chrono::seconds>(it->pingTime - it->pongTime).count();

            if (timeDiff > RESPONSE_TIMEOUT) {
                // �α� ���
                char ipStr[INET6_ADDRSTRLEN]; // IP �ּҸ� ������ ���ڿ�
                if (inet_ntop(AF_INET, &it->addr.sin_addr, ipStr, sizeof(ipStr)) == nullptr) {
                    logMessage.Format(_T("[Ping]Ŭ���̾�Ʈ IP ��ȯ ����"));
                    if (m_CallbackFunc_SendLogUDP) {
                        m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                    }
                }
                else {
                    CString ipStrConverted(ipStr);
                    int nPort = ntohs(it->addr.sin_port);
                    logMessage.Format(_T("[Ping]Ŭ���̾�Ʈ [%s:%d] ���� ����"), ipStrConverted, nPort);
                    if (m_CallbackFunc_SendLogUDP) {
                        m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                    }
                }
                CString ipStrConverted(ipStr);
                int nPort = ntohs(it->addr.sin_port);
                logMessage.Format(_T("[Ping]Ŭ���̾�Ʈ [%s:%d] ��� ���� "), ipStrConverted, nPort);
                if (m_CallbackFunc_SendLogUDP) {
                    m_CallbackFunc_SendLogUDP(m_CallbackPtr_SendLogUDP, &logMessage);
                }

                // Ŭ���̾�Ʈ ��Ͽ��� ���� �� ���ͷ����� ������Ʈ
                it = clients.erase(it);
            }
            else {
                // ���ǿ� ���� ������ ���� Ŭ���̾�Ʈ�� �̵�
                ++it;
            }
        }
    }
}