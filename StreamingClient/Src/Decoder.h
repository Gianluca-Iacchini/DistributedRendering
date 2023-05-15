#pragma once


extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avconfig.h>
#include <libswscale/swscale.h>
}

#define INBUF_SIZE 1024

namespace SC {

	class Decoder
	{
	public:
		typedef struct DecodedFrameData
		{
			unsigned char* data;
			int size;
		} DecodedFrameData;
	public:
		Decoder();
		void InitializeDecoder();
		~Decoder();
		int Decode(uint8_t* data, int dataSize, unsigned char* pixel);
		unsigned char* pixels = nullptr;
	private:
		void DecodeFrame(unsigned char* pixels);
		//void NVIDIAInitialize(ThreadVector<Frame>* vectorPtr);
		static int ReadPacket(void* opaquePointer, uint8_t* pBuf, int nBuf);

		const AVCodec* m_codec = NULL;
		AVCodecParserContext* m_parser = NULL;
		AVCodecContext* m_context = NULL;
		AVFrame* m_frame = NULL;
		AVPacket* m_packet = NULL;



		AVFormatContext* m_fCtx = NULL;
		AVIOContext* m_avioCtx = NULL;


	};
}

