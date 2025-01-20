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
    // Winsock 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    // 소켓 생성
    m_clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_clientSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    CStringA strServerIP(m_strServerIP);  // CStringA로 변환

    PCSTR lpstr = strServerIP;  // CStringA는 PCSTR로 자동 변환됨

    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, lpstr, &serverAddr.sin_addr) <= 0) {
        AfxMessageBox(_T("IP 주소 변환 실패"));
        return false;
    }
    serverAddr.sin_port = htons(m_nServerPort);

    // 서버 연결
    if (connect(m_clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_clientSocket);
        WSACleanup();
        return false;
    }

    // 클라이언트 ID 입력 및 서버로 전송
    CString strPacket;
    strPacket.Format(_T("COMM_CONNECT^%s$"), strClientID);
    CStringA strID(strPacket);

    if (send(m_clientSocket, strID.GetBuffer(), strID.GetLength(), 0) == SOCKET_ERROR) {
        closesocket(m_clientSocket);
        WSACleanup();
        return false;
    }

    // 수신 스레드 생성
    std::thread recvThread(&CCommTcpMgr::ReceiveMessages, this, m_clientSocket);

    // 수신 스레드를 분리하여 메인 스레드가 계속 진행되도록 함
    recvThread.detach();

    return true;
}

void CCommTcpMgr::ReceiveMessages(SOCKET clientSocket)
{
    char buffer[1024];  // 서버로부터 수신할 데이터를 저장할 버퍼

    while (true) {
        // 서버로부터 데이터 수신
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        // 연결 종료 또는 오류 체크
        if (bytesReceived == 0) {
            // 서버가 연결을 종료했을 때
            DisconnectServer();
            break;  // 루프 종료하여 스레드 종료
        }
        else if (bytesReceived < 0) {
            // 오류 발생시
            int errorCode = WSAGetLastError();  // 오류 코드 가져오기
            if (errorCode == 10054)
            {
                // 클라이언트와 연결이 끊겼을 경우
                DisconnectServer();  // 클라이언트 정보 삭제
                break;
            }
            break;  // 루프 종료하여 스레드 종료
        }

        // 수신된 데이터 처리
        buffer[bytesReceived] = '\0';  // 문자열 끝을 표시 (널 종료)

        CString strMessage((char*)buffer);

        m_strQueueMessage += strMessage;

        //---------------------------------------------------------------------------
        // 패킷 단위로 응답에 대한 처리를 해야 한다.
        // socket 에 데이터가 진입하는데 2개 이상 붙어서 1개의 event 로 들어온다.
        // 모든 패킷에 대해서 command 단위로 분리해서 수행해야 한다.
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
                    // '^' 다음부터 '$' 이전까지의 부분 문자열 추출
                    CString dataPart = mainstr.Mid(start_pos + 1, end_pos - start_pos - 1);

                    // ','를 기준으로 데이터 분할
                    CString token;
                    int pos = 0;
                    std::vector<CString> tokens;

                    while ((pos = dataPart.Find(_T(','))) != -1) {
                        token = dataPart.Left(pos);
                        tokens.push_back(token);
                        dataPart = dataPart.Mid(pos + 1);
                    }

                    // 마지막 토큰 추가
                    tokens.push_back(dataPart);

                    // 추출된 데이터를 변수에 저장
                    if (tokens.size() == 2) {
                        strClientID = tokens[0];
                        strChatData = tokens[1];
                    }
                }

                //List에 채팅 출력
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
    // 소켓을 닫고 스레드 종료
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
    // 클라이언트 ID 입력 및 서버로 전송
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