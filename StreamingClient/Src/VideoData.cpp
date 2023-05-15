#include "scpch.h"
#include "VideoData.h"
#include <fstream>
#include <Decoder.h>

extern "C"
{
#include <libavformat/avformat.h>
}

using namespace SC;



Frame* VideoData::ProcessData(char* rawData, size_t size)
{
	uint64_t fpsID = GetFrameID(rawData);

	auto frame = std::find_if(m_frameVector.begin(), m_frameVector.end(), [&](Frame frame) { return frame.frameID == fpsID;	});
	// If frame wasn't present we construct a new one
	if (frame == m_frameVector.end())
	{
		if (fpsID <= m_LastFrameID) {
			m_Logger.Info("Discarding frame {0}, lesser than {1}", fpsID, m_LastFrameID); 
			return nullptr; 
		}

		Frame frame = ConstructFrame(rawData, size);
		std::lock_guard<std::mutex> guard(m_vFrameLock);
		m_frameVector.push_back(frame);
	}
	// If it was present then we add data to it
	else
	{
		if (fpsID <= m_LastFrameID)
		{
			m_Logger.Info("Discarding frame {0}, lesser than {1}", fpsID, m_LastFrameID);
			{
				std::lock_guard<std::mutex> guard(m_vFrameLock);
				m_frameVector.erase(frame);
			}
			return nullptr;
		}

		Fragment frag = ConstructFragment(rawData[0], &rawData[10], size - 10);
		{
			std::lock_guard<std::mutex> guard(m_vFrameLock);
			frame->vFragments.push_back(frag);
		}

		if (frame->fragmentTotal == frame->vFragments.size())
		{
			Frame* rFrame = new Frame();
			{
				std::lock_guard<std::mutex> guard(m_vFrameLock);
				std::sort(frame->vFragments.begin(), frame->vFragments.end());

				rFrame->frameID = frame->frameID;
				rFrame->vFragments = frame->vFragments;
				frame->fragmentTotal = frame->fragmentTotal;
				m_frameVector.erase(frame);
			}

			if (rFrame != nullptr)
			{
				m_LastFrameID += 1;
				return rFrame;
			}
		} 
	}

	return nullptr;
}



Fragment VideoData::ConstructFragment(unsigned char fID, char* data, int size)
{
	Fragment videoData;
	videoData.data = new char[size];
	memset(videoData.data, 0, size);
	memcpy(videoData.data, data, size);
	videoData.size = size;
	videoData.fragmentID = fID;
	return videoData;
}

Frame VideoData::ConstructFrame(char* data, int size)
{
	Fragment videoData = ConstructFragment(data[0], &data[10], size - 10);

	Frame frag;
	frag.vFragments.push_back(videoData);
	frag.fragmentTotal = (unsigned char)data[1];
	memcpy(&frag.frameID, &data[2], 8);

	return frag;
}

void VideoData::PushData(char* data, size_t size)
{
	std::lock_guard<std::mutex> pushLock(m_pushDataLock);
	m_vInputData.push_back(RawVideoData(data, size));
}

void VideoData::ProcessLoop(std::ofstream& fpOut)
{
	while (!m_ShouldStopProcessing)
	{
		bool foundData = false;
		RawVideoData rawData;
		{
			std::lock_guard<std::mutex> pushLock(m_pushDataLock);
			if (m_vInputData.size() > 0)
			{
				rawData = m_vInputData[0];
				m_vInputData.erase(m_vInputData.begin());
				foundData = true;
			}
		}

		if (foundData)
		{
			auto frame = ProcessData(rawData.data, rawData.size);
			if (frame)
			{
				int bufferSize = 0;

				for (Fragment d : frame->vFragments)
				{
					bufferSize += d.size;
				}

				int dataSize = bufferSize;

				uint8_t* inbuf = new uint8_t[bufferSize + AV_INPUT_BUFFER_PADDING_SIZE];
				memset(inbuf, 0, bufferSize);
				memset(inbuf + bufferSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
				uint8_t* data;


				int bufferIndex = 0;
				for (Fragment d : frame->vFragments)
				{
					memcpy(inbuf + bufferIndex, d.data, d.size);
					bufferIndex += d.size;
				}

				data = inbuf;
				while (dataSize > 0)
				{
					int len = m_Decoder->Decode(data, dataSize, m_Decoder->pixels);

					data += len;
					dataSize -= len;
				}

				delete[] inbuf;
				frame->Free();
				delete frame;
			}

			delete rawData.data;
		}
	}
}

VideoData::~VideoData()
{
	m_processLoopThread.join();
}

void VideoData::StartDecoding(std::ofstream& fpOut)
{
	m_ShouldStopProcessing = false;
	m_processLoopThread = std::thread(&VideoData::ProcessLoop, this, std::ref(fpOut));
}

void VideoData::StopDecoding()
{
	m_ShouldStopProcessing = true;
	m_processLoopThread.join();
}