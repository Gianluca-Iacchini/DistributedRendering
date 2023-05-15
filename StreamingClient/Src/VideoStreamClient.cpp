#include "scpch.h"
#include <glad/glad.h>
#include "VideoStreamClient.h"


#define CLIENT_PORT 19998
#define DATAGRAM_PAYLOAD_SIZE (1316)

using namespace SC;

VideoStreamClient::VideoStreamClient()
{
	int wsResult = 0;
	wsResult = WSAStartup(MAKEWORD(2, 2), &m_wsaData);
	if (wsResult != 0)
	{
		throw std::exception("ERROR::WinSocket Startup failed.");
	}

	//m_pixels = new unsigned char[1920 * 1080 * 3];
	//memset(m_pixels, 0, 1920 * 1080 * 3);

	m_Decoder = Decoder();
	m_Decoder.InitializeDecoder();

	m_VideoData = new VideoData();
}

VideoStreamClient::~VideoStreamClient()
{
	m_clientListenThread.join();
	closesocket(m_clientSocket);
	WSACleanup();
}

void VideoStreamClient::Initialize(std::ofstream& fpOut)
{
	if (m_isInitialized)
	{
		throw std::exception("ERROR::VideoStream already initialized");
	}

	m_isInitialized = true;
	m_clientPort = CLIENT_PORT;

	if ((m_clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		throw std::exception("ERROR::Could not create server socket.", WSAGetLastError());
	}

	ZeroMemory(&m_serverAddr, sizeof(m_serverAddr));
	m_serverAddr.sin_family = AF_INET;
	m_serverAddr.sin_port = htons(32333);
	inet_pton(AF_INET, "127.0.0.1", &m_serverAddr.sin_addr.S_un.S_addr);

	m_VideoData->m_Logger = this->m_Logger;
	m_VideoData->m_Decoder = &m_Decoder;
	m_VideoData->StartDecoding(fpOut);

}

void VideoStreamClient::Listen(std::ostream& fpOut)
{
	m_clientListenThread = std::thread(&VideoStreamClient::ClientThreadFunction, this, std::ref(fpOut));
}

void VideoStreamClient::ClientThreadFunction(std::ostream& fpOut)
{
	while (!m_shouldStopListening)
	{
		char message[512] = {};

		if (!m_sent) {
			if (sendto(m_clientSocket, message, strlen(message), 0, (sockaddr*)&m_serverAddr, sizeof(sockaddr_in)) == SOCKET_ERROR)
			{
				throw std::exception("ERROR::Client send failed.");
				continue;
			}
			m_sent = true;
		}

		char* answer = new char[DATAGRAM_PAYLOAD_SIZE];

		int sLen = sizeof(sockaddr_in);
		int answerLen = recvfrom(m_clientSocket, answer, DATAGRAM_PAYLOAD_SIZE, 0, (sockaddr*)&m_serverAddr, &sLen);
		if (answerLen == SOCKET_ERROR)
		{
			throw std::exception("ERROR::Client receive failed.");
		}

		packetReceived += 1;

		m_VideoData->PushData(answer, answerLen);

		//m_Logger.Info("Packets received: {0}", packetReceived);

		//Frame* frame = m_VideoData->SendData(answer, answerLen);

		//if (frame)
		//{
		//	//m_Logger.Info("Completed Frame: {0}, Size: {1}", frame->frameID, frame->vFragments.size());

		//	for (Fragment f : frame->vFragments)
		//	{
		//		fpOut.write(f.data, f.size);
		//	}
		//}

		//m_Logger.Info("Packets received: {0}", packetReceived);

		//if (m_vFragments.empty())
		//{
		//	Frame frag = ConstructFragment(answer, answerLen - 10);
		//	m_vFragments.push_back(frag);
		//}
		//else
		//{


		//	auto isPresent = std::find_if(m_vFragments.begin(), m_vFragments.end(),
		//		[&](Frame frag)
		//		{
		//			uint64_t fps = 0;
		//	memcpy(&fps, &answer[2], 8);
		//	return frag.fragmentID == fps; });

		//	if (isPresent != m_vFragments.end())
		//	{
		//		if (isPresent->fragmentID <= m_lastFrameID)
		//		{
		//			m_Logger.Info("Frame dropped");
		//			m_vFragments.erase(isPresent);
		//			continue;
		//		}

		//		Fragment lVideoData = ConstructVideoData(answer, answerLen - 10);

		//		isPresent->vFragments.push_back(lVideoData);

		//		if (isPresent->vFragments.size() == isPresent->fragmentTotal )
		//		{
		//			auto videoData = isPresent->vFragments;
		//			std::sort(videoData.begin(), videoData.end());

		//			int bufferSize = 0;

		//			////Decoder::DecodedFrameData* dfd = nullptr;

		//			for (Fragment d : videoData)
		//			{
		//				bufferSize += d.size;
		//			}

		//			int dataSize = bufferSize;

		//			uint8_t* inbuf = new uint8_t[bufferSize + AV_INPUT_BUFFER_PADDING_SIZE];
		//			memset(inbuf, 0, bufferSize);
		//			memset(inbuf + bufferSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
		//			uint8_t* data;

		//			int bufferIndex = 0;
		//			for (Fragment d : videoData)
		//			{
		//				memcpy(inbuf + bufferIndex, d.data, d.size);
		//				bufferIndex += d.size;

		//				//fpOut.write(d.data, d.size);
		//				//memcpy(inbuf, d.data, d.size);
		//				//data = inbuf;
		//				//dataSize = d.size;
		//				//while (dataSize > 0)
		//				//{
		//				//	int len = m_Decoder.Decode(data, dataSize, m_pixels);

		//				//	data += len;
		//				//	dataSize -= len;
		//				//}
		//			}

		//			data = inbuf;
		//			while (dataSize > 0)
		//			{
		//				int len = m_Decoder.Decode(data, dataSize, m_pixels);

		//				data += len;
		//				dataSize -= len;
		//			}

		//			m_lastFrameID = isPresent->fragmentID;
		//			m_vFragments.erase(isPresent);
		//		}
		//	}
		//	else
		//	{
		//		Frame frag = ConstructFragment(answer, answerLen - 10);
		//		if (frag.fragmentID < m_lastFrameID)
		//		{
		//			m_Logger.Info("Frame dropped");
		//			continue;
		//		}


		//		//if (m_vFragments.size() > m_MaxSize)
		//		//{
		//		//	std::sort(m_vFragments.begin(), m_vFragments.end());

		//		//	m_vFragments.erase(m_vFragments.begin());

		//		//}
		//		else
		//			m_vFragments.push_back(frag);
		//	}
		//}


	}
}
//
//Fragment VideoStreamClient::ConstructVideoData(char* data, int size)
//{
//	Fragment videoData;
//	videoData.data = new char[size];
//	memset(videoData.data, 0, size);
//	memcpy(videoData.data, &data[10], size);
//	videoData.size = size;
//	videoData.fragmentID = (unsigned char)data[0];
//	return videoData;
//}
//
//Frame VideoStreamClient::ConstructFragment(char* data, int size)
//{
//	Fragment videoData = ConstructVideoData(data, size);
//
//
//
//	Frame frag;
//	frag.vFragments.push_back(videoData);
//	frag.fragmentTotal = (unsigned char)data[1];
//	memcpy(&frag.fragmentID, &data[2], 8);
//
//	return frag;
//}



bool VideoStreamClient::m_isInitialized = false;
