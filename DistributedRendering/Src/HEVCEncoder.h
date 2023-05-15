#pragma once

#include <dxgi1_6.h>
#include <D3D12/d3d12video.h>
#include <D3D12/d3dx12.h>
#include <NVIDIACodec/nvEncodeAPI.h>

#include <fstream>

//Macros from NVIDIA Encoder Samples
#define NVENC_THROW_ERROR( errorStr)                                                         \
    do                                                                                                   \
    {                                                                                                    \
        throw std::exception(errorStr);														 \
    } while (0)


#define NVENC_API_CALL( nvencAPI )                                                                                 \
    do                                                                                                             \
    {                                                                                                              \
        NVENCSTATUS errorCode = nvencAPI;                                                                          \
        if( errorCode != NV_ENC_SUCCESS)                                                                           \
        {                                                                                                          \
			throw std::exception("NVENC ERROR: ");							   \
        }                                                                                                          \
    } while (0)

#define ALIGN_UP(s,a) (((s) + (a) - 1) & ~((a) - 1))

namespace dx12lib
{
	class InputBuffer;
	class Device;
}

namespace DR {
	class VideoStream;
	class Application;

	// From NVIDIA Codec Samples
	struct NvEncInputFrame
	{
		void* inputPtr = nullptr;
		uint32_t chromaOffsets[2];
		uint32_t numChromaPlanes;
		uint32_t pitch;
		uint32_t chromaPitch;
		NV_ENC_BUFFER_FORMAT bufferFormat;
		NV_ENC_INPUT_RESOURCE_TYPE resourceType;
	};

	class HEVCEncoder
	{
	public:
		HEVCEncoder(std::shared_ptr<dx12lib::Device> device);
		~HEVCEncoder();
		void EncodeFrame(ID3D12Resource* src, std::ofstream& fpOut);
		void EndEncode(std::ofstream& fpOut);
	private:
		void LoadNvidiaEncoderAPI();
		void CreateNvidiaEncoder(int sWidth, int sHeight);
		void SetNVIDIAEncoderParams(NV_ENC_INITIALIZE_PARAMS* initParams);
		void AllocateInputBuffers();
		void AllocateOutputBuffers();
		void RegisterInputBuffers();
		void CopyTexture(const NvEncInputFrame* inputFrame, ID3D12Resource* textSrc, ID3D12Fence* inputFence, uint64_t* inputFenceVal);
		void MapResources(uint32_t brfIdx);
		NVENCSTATUS Encode(NV_ENC_INPUT_PTR inputBuffer, NV_ENC_OUTPUT_PTR outputBuffer, NV_ENC_PIC_PARAMS* picParams = nullptr);
	
		void EncodePacket(std::vector<NV_ENC_OUTPUT_RESOURCE_D3D12*>& vOutputBuffer, bool bOutputDelay);


		NV_ENC_REGISTERED_PTR RegisterResource(void* pBuffer, NV_ENC_INPUT_RESOURCE_TYPE resType, int width, int height, int pitch, NV_ENC_BUFFER_FORMAT format,
			NV_ENC_BUFFER_USAGE usage, NV_ENC_FENCE_POINT_D3D12* inputFence = NULL);
		NvEncInputFrame* GetNextInputFrame();

		std::shared_ptr<dx12lib::Device> m_Device;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_InputFence;
		Microsoft::WRL::ComPtr<ID3D12Fence> m_OutputFence;
		uint64_t m_inputFenceVal = 0;
		uint64_t m_outputFenceVal = 0;
		HANDLE m_Event;

		Microsoft::WRL::ComPtr<ID3D12VideoEncoderHeap> m_EncoderHeap;

		void* m_hEncoder = nullptr;
		NV_ENCODE_API_FUNCTION_LIST m_nvenc;
		NV_ENC_INITIALIZE_PARAMS m_initParams;
		uint32_t m_nExtraOutputDelay = 3;
		int32_t m_nEncodeBuffer = 0;
		int32_t m_nOutputDelay = 0;

		std::vector<void*> m_vpCompletionEvent;
		std::vector<void*> m_vMappedInputBuffers;
		std::vector<void*> m_vMappedOutputBuffers;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_vInputBuffers;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_vOutputBuffers;
		std::vector<NV_ENC_INPUT_RESOURCE_D3D12*> m_vInputResNV;
		std::vector<NV_ENC_OUTPUT_RESOURCE_D3D12*> m_vOutputResNV;

		std::vector<void*> m_vRegisteredInputResources;
		std::vector<void*> m_vRegisteredOutputResources;
		std::vector<NvEncInputFrame> m_vInputFrames;
		int32_t m_nextIndex = 0;
		int32_t m_currentIndex = 0;

		std::vector<std::vector<uint8_t>> m_vVPacket;

		VideoStream* m_videoStream;

	public:
		uint64_t encodedFrames = 0;
		uint64_t totalFrames = 0;
		bool streamed = false;
	};


}

