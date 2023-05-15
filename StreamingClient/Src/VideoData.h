#pragma once
#include <vector>
#include <algorithm>
#include <mutex>
#include "Logger.h"
#include "Decoder.h"

namespace SC {
	typedef struct Fragment
	{
		char* data;
		int size;
		unsigned char fragmentID;
		bool operator < (const Fragment& str) const
		{
			return (fragmentID < str.fragmentID);
		}
		Fragment() { data = nullptr; size = 0; fragmentID = 0; }
	} Fragment;

	typedef struct Frame
	{
		std::vector<Fragment> vFragments;
		unsigned char fragmentTotal;
		uint64_t frameID;

		bool operator < (const Frame& str) const
		{
			return (frameID < str.frameID);
		}

		void Free() { for (Fragment f : vFragments) { delete f.data; } }
	} Frame;

	class VideoData
	{
	private:
		typedef struct RawVideoData 
		{
			char* data;
			size_t size;

			RawVideoData(char* data, size_t size) : data(data), size(size) {}
			RawVideoData() { data = nullptr; size = 0; }

		} RawVideoData ;

	public:
		VideoData() {}
		~VideoData();

		Logger m_Logger;
		Decoder* m_Decoder = nullptr;

		Frame* ProcessData(char* rawData, size_t size);
		void PushData(char* data, size_t size);
		void StartDecoding(std::ofstream& fpOut);
		void StopDecoding();
		//void ThreadErase(std::vector<T>::const_iterator element) { std::lock_guard(vectorLock); m_Vector.erase(element); }
		//void ThreadSort() { std::lock_guard(vectorLock); std::sort(m_Vector.begin(), m_Vector.end()); }

		//void Push(const T& val) { m_Vector.push_back(val); }
		//void Erase(std::vector<T>::const_iterator element) { m_Vector.erase(element); }
		//size_t Size() { return m_Vector.size(); }
		//std::vector<T> GetRawVector() { return m_Vector; }



	private:
		Frame ConstructFrame(char* data, int size);
		Fragment ConstructFragment(unsigned char fID, char* data, int size);
		uint64_t GetFrameID(char* data) { uint64_t fps = 0; memcpy(&fps, &data[2], 8); return fps; }
		void ProcessLoop(std::ofstream& fpOut);


		std::vector<Frame> m_frameVector;
		std::mutex m_vFrameLock;
		std::mutex m_pushDataLock;

		uint64_t m_LastFrameID = 0;

		std::thread m_processLoopThread;

		std::vector<RawVideoData> m_vInputData;

		bool m_ShouldStopProcessing = false;
	};

}