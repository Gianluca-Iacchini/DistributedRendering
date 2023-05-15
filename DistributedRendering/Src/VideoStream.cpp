#include "drpch.h"
#include "VideoStream.h"
#include "Application.h"

#define DATAGRAM_PAYLOAD_SIZE (1316)
#define DATAGRAM_VIDEO_SIZE (DATAGRAM_PAYLOAD_SIZE - 10) //Max payload reccomended size - 3 * sizeof(char) (8 for frame id, 1 for packet id, 1 for total packets)

using namespace DR;

VideoStream::VideoStream()
{
	auto logger = Application::GetInstance()->GetLogger();

	int wsResult = 0;
	wsResult = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	if (wsResult != 0)
	{
		throw std::exception("ERROR::WinSocket Startup failed.");
	}


}

DR::VideoStream::~VideoStream()
{
	closesocket(m_serverSocket);
	WSACleanup();
}

void VideoStream::Stream(std::vector<uint8_t> framePacket, uint64_t frameN, std::ostream& fpOut)
{
	uint8_t val[8] = {};
	memset(val, 0, 8);
	memcpy(val, &frameN, sizeof(frameN));

	std::vector<std::vector<uint8_t>> fragPackets;

	unsigned char nFragments = framePacket.size() / DATAGRAM_VIDEO_SIZE;
	if (framePacket.size() % DATAGRAM_VIDEO_SIZE != 0) nFragments++;

	for (int i = 0; i < nFragments; i++)
	{
		int startIndex = i * DATAGRAM_VIDEO_SIZE;
		int endIndex = framePacket.size() < (i + 1) * DATAGRAM_VIDEO_SIZE ? framePacket.size() : (i + 1) * DATAGRAM_VIDEO_SIZE;

		std::vector<uint8_t> frag = std::vector<uint8_t>(framePacket.begin() + startIndex, framePacket.begin() + endIndex);

		frag.insert(frag.begin(), &val[0], &val[8]);
		frag.insert(frag.begin(), nFragments);
		frag.insert(frag.begin(), (unsigned char)i);

		fragPackets.push_back(frag);
	}

	//ServerThreadFunction();

	if (m_isClient)
	{


		char* frameMessage;


		Application::GetInstance()->GetLogger().Info("Sending Frame: {0}, Size: {1}", frameN, fragPackets.size());

		for (std::vector<uint8_t> vFrag : fragPackets)
		{


			frameMessage = new char[vFrag.size()];
			frameMessage = (char*)vFrag.data();
			m_packetsSent += 1;

			if (sendto(m_serverSocket, frameMessage, vFrag.size(), 0, (sockaddr*)&m_client, sizeof(sockaddr_in)) == SOCKET_ERROR)
			{
				Application::GetInstance()->GetLogger().Error("ERROR::Connection error when sending message {0}", WSAGetLastError());

				return;
			}
		}

		//Application::GetInstance()->GetLogger().Info("Packets sent: {0}", m_packetsSent);
	}
}

void VideoStream::InitializeAsServer(USHORT port)
{
	if (m_isInitialized)
	{
		throw std::exception("ERROR::VideoStream already initialized");
	}

	m_isInitialized = true;

	m_serverPort = port;

	if ((m_serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		throw std::exception("ERROR::Could not create server socket.", WSAGetLastError());
	}

	m_server.sin_family = AF_INET;
	m_server.sin_addr.s_addr = INADDR_ANY;
	m_server.sin_port = htons(m_serverPort);

	if (bind(m_serverSocket, (sockaddr*)&m_server, sizeof(m_server)) == SOCKET_ERROR)
	{
		throw std::exception("ERROR::Could not bind server socket.", WSAGetLastError());
	}

	m_ServerInitialized = true;

	ServerThreadFunction();

	//m_serverListenThread = std::thread(&VideoStream::ServerThreadFunction, this);
}

void VideoStream::InitializeAsClient(USHORT port)
{
	if (m_isInitialized)
	{
		throw std::exception("ERROR::VideoStream already initialized");
	}

	m_isInitialized = true;
	m_clientPort = port;

	if ((m_clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		throw std::exception("ERROR::Could not create server socket.", WSAGetLastError());
	}

	ZeroMemory(&m_server, sizeof(m_server));
	m_server.sin_family = AF_INET;
	m_server.sin_port = htons(32333);
	inet_pton(AF_INET, "127.0.0.1", &m_server.sin_addr.S_un.S_addr);
}

void VideoStream::ClientListen(std::ostream& fpOut)
{
	m_clientListenThread = std::thread(&VideoStream::ClientThreadFunction, this, std::ref(fpOut));
}

void VideoStream::ServerThreadFunction()
{
	auto logger = Application::GetInstance()->GetLogger();
	logger.Info("Waiting for connection...");

	char clientMessage[512];

	int messageLen = SOCKET_ERROR;
	int sLen = sizeof(sockaddr_in);

	if (messageLen == recvfrom(m_serverSocket, clientMessage, sizeof(clientMessage), 0, (sockaddr*)&m_client, &sLen) == SOCKET_ERROR)
	{
		Application::GetInstance()->GetLogger().Error("ERROR::Connection error when receiving message.");
		return;
	}

	m_isClient = true;

	logger.Info("Client connected!");
}

void VideoStream::ClientThreadFunction(std::ostream& fpOut)
{
	while (!m_shouldStopListen)
	{
		if (!m_ServerInitialized)
		{
			continue;
		}

		char message[512] = {};


		if (sendto(m_clientSocket, message, strlen(message), 0, (sockaddr*)&m_server, sizeof(sockaddr_in)) == SOCKET_ERROR)
		{
			throw std::exception("ERROR::Client send failed.");
		}

		char* answer = new char[DATAGRAM_PAYLOAD_SIZE];

		int sLen = sizeof(sockaddr_in);
		int answerLen = recvfrom(m_clientSocket, answer, DATAGRAM_PAYLOAD_SIZE, 0, (sockaddr*)&m_server, &sLen);
		if (answerLen == SOCKET_ERROR)
		{

			throw std::exception("ERROR::Client receive failed.");
		}



		if (m_vFragments.empty())
		{
			Fragment frag = ConstructFragment(answer, answerLen-10);
			m_vFragments.push_back(frag);
		}
		else
		{


			auto isPresent = std::find_if(m_vFragments.begin(), m_vFragments.end(),
				[&](Fragment frag)
				{
					uint64_t fps = 0;
			memcpy(&fps, &answer[2], 8);
					return frag.fpsID == fps; });

			if (isPresent != m_vFragments.end())
			{
				if (isPresent->fpsID < m_lastFrameID)
				{
					m_vFragments.erase(isPresent);
					continue;
				}

				VideoData lVideoData = ConstructVideoData(answer, answerLen-10);

				isPresent->vVideoData.push_back(lVideoData);

				if (isPresent->vVideoData.size() == isPresent->fragmentTotal && isPresent->fpsID > m_lastFrameID)
				{
					auto videoData = isPresent->vVideoData;
					std::sort(videoData.begin(), videoData.end());

					for (VideoData d : videoData)
					{
						fpOut.write(d.data, d.size);
					}

					m_lastFrameID = isPresent->fpsID;
					auto a = m_vFragments.erase(isPresent);
				}
			}
			else
			{
				Fragment frag = ConstructFragment(answer, answerLen-10);
				if (frag.fpsID < m_lastFrameID)
					continue;

				if (m_vFragments.size() > m_MaxSize)
				{
					std::sort(m_vFragments.begin(), m_vFragments.end());

					m_vFragments.erase(m_vFragments.begin());

				}
				m_vFragments.push_back(frag);
			}
		}


	}


}

VideoStream::VideoData VideoStream::ConstructVideoData(char* data, int size)
{
	VideoData videoData;
	videoData.data = new char[size];
	memset(videoData.data, 0, size);
	memcpy(videoData.data, &data[10], size);
	videoData.size = size;
	videoData.frameID = (unsigned char)data[0];
	return videoData;
}

VideoStream::Fragment VideoStream::ConstructFragment(char* data, int size)
{
	VideoData videoData = ConstructVideoData(data, size);



	Fragment frag;
	frag.vVideoData.push_back(videoData);
	frag.fragmentTotal = (unsigned char)data[1];
	memcpy(&frag.fpsID, &data[2], 8);

	return frag;
}

void VideoStream::StopListening()
{
	m_shouldStopListen = true;
	m_clientListenThread.join();
}

bool VideoStream::m_ServerInitialized = false;
