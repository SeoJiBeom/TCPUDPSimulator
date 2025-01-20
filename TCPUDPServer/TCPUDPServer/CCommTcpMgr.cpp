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

    // 소켓 라이브러리 초기화
    if (!AfxSocketInit()) {
        logMessage.Format(_T("소켓 초기화 실패"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // 서버 소켓 생성
    m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverSocket == INVALID_SOCKET) {
        logMessage.Format(_T("서버 소켓 생성 실패"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // 서버 주소 설정
    sockaddr_in serverAddr = { 0 };
    serverAddr.sin_family = AF_INET;
    CStringA strIP(m_strConnectIP);  // CStringA를 사용하여 변환
    if (inet_pton(AF_INET, strIP, &serverAddr.sin_addr) <= 0) {
        logMessage.Format(_T("IP 주소 변환 실패"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }
    serverAddr.sin_port = htons(m_nConnectPort);

    // 소켓 바인딩
    if (bind(m_serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();  // 오류 코드 가져오기
        char msg[256];
        snprintf(msg, sizeof(msg), "소켓 바인딩 실패, 오류 코드: %d", error);
        logMessage = CA2T(msg);
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // 연결 대기
    if (listen(m_serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        logMessage.Format(_T("소켓 리슨 실패"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }
        return;
    }

    // 연결 수락 스레드 시작
    std::thread acceptThread(&CCommTcpMgr::AcceptClient, this);
    acceptThread.detach(); // 메인 스레드와 분리
}

void CCommTcpMgr::AcceptClient()
{
    while (true) {
        // 클라이언트 연결 요청 수락
        CString logMessage;
        SOCKET clientSocket = accept(m_serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            logMessage.Format(_T("클라이언트 연결 수락 실패"));
            if (m_CallbackFunc_SendLogTCP)
            {
                m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
            }
            continue;
        }

        
        logMessage.Format(_T("새 클라이언트가 연결되었습니다!"));
        if (m_CallbackFunc_SendLogTCP)
        {
            m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
        }

        // 클라이언트 정보 생성 및 스레드 시작
        std::lock_guard<std::mutex> lock(m_clientMutex);
        m_clients.push_back({ clientSocket, std::thread(&CCommTcpMgr::HandleClient, this, clientSocket) });
        m_clients.back().clientThread.detach(); // 스레드 분리
    }
}

void CCommTcpMgr::HandleClient(SOCKET clientSocket)
{
    char buffer[1024];
    CString logMessage;

    //IP, Port 추출
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
        // 클라이언트로부터 데이터 수신
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == 0) {
            // 클라이언트가 연결을 종료한 경우
            DeleteClientInfo(clientSocket, strClientIP, nPort);  // 클라이언트 정보 삭제
            logMessage.Format(_T("클라이언트가 연결을 종료했습니다."));
            if (m_CallbackFunc_SendLogTCP)
            {
                m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
            }
            break;
        }
        else if (bytesReceived < 0) {
            // 오류 발생 시
            int errorCode = WSAGetLastError();  // 오류 코드 가져오기
            if (errorCode == 10054)
            {
                // 클라이언트와 연결이 끊겼을 경우
                DeleteClientInfo(clientSocket, strClientIP, nPort);  // 클라이언트 정보 삭제
                break;
            }
            CString errorMsg;
            errorMsg.Format(_T("recv() 오류 발생, 오류 코드: %d"), errorCode);
            logMessage.Format(_T("%s"), errorMsg);
            if (m_CallbackFunc_SendLogTCP)
            {
                m_CallbackFunc_SendLogTCP(m_CallbackPtr_SendLogTCP, &logMessage);
            }
            break;
        }
        else {
            // 정상적으로 데이터를 받은 경우
            buffer[bytesReceived] = '\0';  // 문자열 끝을 표시 (널 종료)
            // 수신된 데이터를 처리하는 코드 (예: 화면 출력)
        }

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
                    if (tokens.size() == 1) {
                        strClientID = tokens[0];
                    }
                }

                //리스트 추가
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

                //모든 클라이언트에 채팅 내용 전달
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
        closesocket(it->clientSocket);  // 소켓 닫기
        m_clients.erase(it);  // 벡터에서 클라이언트 삭제
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

    // 벡터에 있는 모든 클라이언트에게 메시지 전송
    CString strPacket;
    strPacket.Format(_T("COMM_SEND_CHAT^%s,%s$"), strID, strChatData);
    CStringA strMessage(strPacket);

    for (auto& client : m_clients) {
        if (send(client.clientSocket, strMessage, strlen(strMessage), 0) == SOCKET_ERROR) {
            // 오류 발생 시 해당 클라이언트를 삭제
            sockaddr_in clientAddr;
            int addrLen = sizeof(clientAddr);

            // 클라이언트의 IP와 포트를 얻기 위한 함수
            if (getpeername(client.clientSocket, (sockaddr*)&clientAddr, &addrLen) == SOCKET_ERROR) {
                return;  // 오류 발생 시 빈 문자열 반환
            }

            // IP 주소를 문자열로 변환
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
            CString clientIP(ipStr);  // char*에서 CString으로 변환

            // 포트 번호 얻기
            int clientPort = ntohs(clientAddr.sin_port);

            DeleteClientInfo(client.clientSocket, clientIP, clientPort);
        }
    }
}