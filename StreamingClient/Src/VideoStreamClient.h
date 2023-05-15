#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include "Logger.h"
#include "Decoder.h"
#include "VideoData.h"

namespace SC
{

	class VideoStreamClient
	{
	public:
		VideoStreamClient();
		~VideoStreamClient();
		void Initialize(std::ofstream& fpOut);
		void Listen(std::ostream& fpOut);

		Logger m_Logger;

		unsigned char* GetPixels() { return m_Decoder.pixels; }
	private:

		void ClientThreadFunction(std::ostream& fpOut);
		//Fragment ConstructVideoData(char* data, int size);
		//Frame ConstructFragment(char* data, int size);

		uint64_t packetReceived = 0;

		static bool m_isInitialized;
		
		WSADATA m_wsaData;
		USHORT m_clientPort;
		SOCKET m_clientSocket = INVALID_SOCKET;
		sockaddr_in m_serverAddr;

		bool m_shouldStopListening = false;
		std::thread m_clientListenThread;

		uint64_t frameWritten = 0;
		//std::vector<Frame> m_vFragments;
		int m_MaxSize = 120;
		uint64_t m_lastFrameID = 0;

		Decoder m_Decoder;

		VideoData* m_VideoData = nullptr;

		bool m_sent = false;

	};
}

