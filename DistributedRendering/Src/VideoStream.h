#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>

namespace DR {

	class VideoStream
	{
	private:
		typedef struct VideoData
		{
			char* data;
			int size;
			unsigned char frameID;
			bool operator < (const VideoData& str) const
			{
				return (frameID < str.frameID);
			}

		} VideoData;

		typedef struct Fragment
		{
			std::vector<VideoData> vVideoData;
			unsigned char fragmentTotal;
			uint64_t fpsID;

			bool operator < (const Fragment& str) const
			{
				return (fpsID < str.fpsID);
			}
		} Fragment;

	public:
		VideoStream();
		~VideoStream();
		void Stream(std::vector<uint8_t> framePacket, uint64_t frameN, std::ostream& fpOut);

		void InitializeAsServer(USHORT port);
		void InitializeAsClient(USHORT port);

		void ClientListen(std::ostream& fpOut);
		void StopListening();



	private:
		void ClientThreadFunction(std::ostream& fpOut);
		void ServerThreadFunction();
		VideoData ConstructVideoData(char* data, int size);
		Fragment ConstructFragment(char* data, int size);


		WSADATA m_wsaData;
		SOCKET m_serverSocket = INVALID_SOCKET;
		SOCKET m_clientSocket = INVALID_SOCKET;

		sockaddr_in m_server, m_client;
		bool m_shouldStopListen = false;

		bool m_isClient = false;
		bool m_isInitialized = false;

		USHORT m_serverPort;
		USHORT m_clientPort;

		std::thread m_clientListenThread;
		std::thread m_serverListenThread;

		static bool m_ServerInitialized;
		


		uint64_t frameWritten = 0;
		std::vector<Fragment> m_vFragments;
		int m_MaxSize = 1200; 
		uint64_t m_lastFrameID = 0;

		uint64_t m_packetsSent = 0;
	};
}

