#include "scpch.h"
#include <glad/glad.h>
#include "Decoder.h"


using namespace SC;

Decoder::Decoder() { pixels = new unsigned char[1920 * 1080 * 3]; memset(pixels, 0, 1920 * 1080 * 3); }

Decoder::~Decoder()
{
    av_parser_close(m_parser);
    avcodec_free_context(&m_context);
    av_frame_free(&m_frame);
    av_packet_free(&m_packet);
}

void Decoder::InitializeDecoder()
{
    m_packet = av_packet_alloc();
    if (!m_packet)
    {
        throw std::exception("ERROR::Could not allocate packet");
    }

    //memset(m_inputBuffer + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    //memset(m_inputBuffer, 0, sizeof(m_inputBuffer));

    m_codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!m_codec) {
        throw std::exception("ERROR::Codec not found");
    }

    m_parser = av_parser_init(m_codec->id);
    if (!m_parser) {
        throw std::exception("ERROR::Parser not found");
    }

    m_context = avcodec_alloc_context3(m_codec);
    if (!m_context) {
        throw std::exception("ERROR::Could not allocate context");
    }
    m_context->thread_type = FF_THREAD_FRAME;

    if (avcodec_open2(m_context, m_codec, NULL) < 0) {
        throw std::exception("ERROR::Could not open codec");
    }

    m_frame = av_frame_alloc();
    if (!m_frame) {
        throw std::exception("ERROR::Could not allocate frame buffer");
    }
}

int Decoder::Decode(uint8_t* data, int dataSize, unsigned char* pixels)
{
    //DecodedFrameData* dfd = new DecodedFrameData;
    //dfd->size = 3 * m_context->width * m_context->height;
    //dfd->data = new unsigned char[dfd->size];

    //int paddedSize = dataSize + AV_INPUT_BUFFER_PADDING_SIZE;
    //uint8_t* inbuf = new uint8_t[paddedSize];
    //uint8_t* bufPtr;
    //memcpy(inbuf, data, dataSize);
    //memset(inbuf + dataSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    //bufPtr = inbuf;

    //int dataLeft = dataSize;

   // while (dataLeft > 0)
   // {
    
        int parsedLen = av_parser_parse2(m_parser, m_context, &m_packet->data, &m_packet->size, data, dataSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        if (parsedLen < 0)
        {
            throw std::exception("ERROR::Could not parse data");
        }

        //data += parsedLen;
        //dataSize -= parsedLen;

        if (m_packet->data)
        {
            DecodeFrame(pixels);
            //return dfd;
        }

        return parsedLen;
    //}

    //return nullptr;
}

void Decoder::DecodeFrame(unsigned char* pixels)
{
    int ret = avcodec_send_packet(m_context, m_packet);
    if (ret < 0)
    {
        throw std::exception("ERROR::Error sending packet for decoding");
    }

    struct SwsContext* swsCtx = NULL;
    swsCtx = sws_getContext(m_context->width, m_context->height, m_context->pix_fmt, m_context->width, m_context->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    if (!swsCtx)
    {
        throw std::exception("ERROR::Could not get SWS context");
    }

    AVFrame* pRGBFrame = av_frame_alloc();
    pRGBFrame->format = AV_PIX_FMT_RGB24;
    pRGBFrame->width = m_context->width;
    pRGBFrame->height = m_context->height;

    int sts = av_frame_get_buffer(pRGBFrame, 0);

    if (sts < 0)
    {
        throw std::exception("ERROR::Could not get frame buffer");
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(m_context, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return;
        }

        sts = sws_scale(swsCtx,                //struct SwsContext* c,
            m_frame->data,            //const uint8_t* const srcSlice[],
            m_frame->linesize,        //const int srcStride[],
            0,                      //int srcSliceY, 
            m_frame->height,          //int srcSliceH,
            pRGBFrame->data,        //uint8_t* const dst[], 
            pRGBFrame->linesize);   //const int dstStride[]);

        if (sts != m_frame->height)
        {
            throw std::exception("ERROR::Generic error");
        }

        memcpy(pixels, pRGBFrame->data[0], 3 * 1920 * 1080);

        av_frame_free(&pRGBFrame);
        sws_freeContext(swsCtx);
    }



}


//void Decoder::NVIDIAInitialize(ThreadVector* vectorPtr)
//{
//    if (!(m_fCtx = avformat_alloc_context()))
//    {
//        throw std::exception("ERROR::Could not allocate AVFormat context.");
//    }
//
//    uint8_t* aviocBuffer = NULL;
//    int aviocBufferSize = 504; //UDP packet size
//    aviocBuffer = (uint8_t*)av_malloc(aviocBufferSize);
//
//    if (!aviocBuffer)
//    {
//        throw std::exception("ERROR::Could not allocate AVIO Context buffer");
//    }
//
//    m_avioCtx = avio_alloc_context(aviocBuffer, aviocBufferSize, 0, NULL/*ToChange*/, &ReadPacket, NULL, NULL);
//
//    if (!m_avioCtx)
//    {
//        throw std::exception("ERROR::Could not allocate AVIO Context");
//    }
//
//    m_fCtx->pb = m_avioCtx;
//
//    int ret = avformat_open_input(&m_fCtx, NULL, NULL, NULL);
//
//    if (ret < 0)
//    {
//        throw std::exception("ERROR::Could not open input");
//    }
//}

int Decoder::ReadPacket(void* opaquePointer, uint8_t* pBuf, int nBuf)
{
    return 0;
}
