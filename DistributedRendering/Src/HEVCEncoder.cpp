#include "drpch.h"
#include "HEVCEncoder.h"
#include "Application.h"
#include <dxgidebug.h>
#include <d3d12sdklayers.h>
#include <dx12Lib/InputBuffer.h>
#include <dx12Lib/Device.h>
#include <dx12Lib/CommandQueue.h>
#include <dx12Lib/CommandList.h>
#include "VideoStream.h"

using namespace DR;
using namespace Microsoft::WRL;
using namespace dx12lib;

HEVCEncoder::HEVCEncoder(std::shared_ptr<dx12lib::Device> device) : m_Device(device)
{

	NV_ENC_BUFFER_FORMAT bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;

	LoadNvidiaEncoderAPI();

	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS encodeSessionExParams = { NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
	encodeSessionExParams.device = (void*)device->GetD3D12Device().Get();
	encodeSessionExParams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	encodeSessionExParams.apiVersion = NVENCAPI_VERSION;

	void* hEncoder = nullptr;
	
	NVENC_API_CALL(m_nvenc.nvEncOpenEncodeSessionEx(&encodeSessionExParams, &hEncoder));
	m_hEncoder = hEncoder;

	Application* app = Application::GetInstance();
	int sWidth = app->GetWidth();
	int sHeight = app->GetHeight();
	
	m_videoStream = new VideoStream();
	m_videoStream->InitializeAsServer(32333);
	CreateNvidiaEncoder(sWidth, sHeight);

}

HEVCEncoder::~HEVCEncoder() {}

void HEVCEncoder::CopyTexture(const NvEncInputFrame* inputFrame, ID3D12Resource* textSrc, ID3D12Fence* inputFence, uint64_t* inputFenceVal)
{
	ID3D12Resource* inpRes = (ID3D12Resource*)inputFrame->inputPtr;
	
	auto& commandQueue = m_Device->GetCommandQueue();
	auto commandList = commandQueue.GetCommandList();
	


	commandList->CopyResource(inpRes, textSrc);
	commandList->TransitionBarrier(inpRes, D3D12_RESOURCE_STATE_COMMON);

	commandQueue.ExecuteCommandList(commandList);

	InterlockedIncrement(inputFenceVal);

	commandQueue.GetD3D12CommandQueue()->Signal(inputFence, *inputFenceVal);


}

void HEVCEncoder::EncodeFrame(ID3D12Resource* src, std::ofstream& fpOut)
{
	totalFrames = (totalFrames + 1);

	m_vVPacket.clear();

	NvEncInputFrame* nextFrame = GetNextInputFrame();

	CopyTexture(nextFrame, src, m_InputFence.Get(), &m_inputFenceVal);
	
	int bfrIdx = m_nextIndex % m_nEncodeBuffer;

	MapResources(bfrIdx);

	InterlockedIncrement(&m_outputFenceVal);

	m_vOutputResNV[bfrIdx]->pOutputBuffer = m_vMappedOutputBuffers[bfrIdx];
	m_vOutputResNV[bfrIdx]->outputFencePoint.signalValue = m_outputFenceVal;
	m_vOutputResNV[bfrIdx]->outputFencePoint.bSignal = true;

	m_vInputResNV[bfrIdx]->pInputBuffer = m_vMappedInputBuffers[bfrIdx];
	m_vInputResNV[bfrIdx]->inputFencePoint.waitValue = m_inputFenceVal;
	m_vInputResNV[bfrIdx]->inputFencePoint.bWait = true;

	NVENCSTATUS nvStatus = Encode(m_vInputResNV[bfrIdx], m_vOutputResNV[bfrIdx]);

	if (nvStatus == NV_ENC_SUCCESS || nvStatus == nvStatus == NV_ENC_ERR_NEED_MORE_INPUT)
	{
		m_nextIndex++;
		EncodePacket(m_vOutputResNV, true);

		if (true)
		{
			for (std::vector<uint8_t>& packet : m_vVPacket)
			{
				m_videoStream->Stream(packet, totalFrames, fpOut);
			}
		}



		encodedFrames += m_vVPacket.size();
	}
	else
	{
		NVENC_THROW_ERROR("NVEncode failure");
	}


}

NVENCSTATUS HEVCEncoder::Encode(NV_ENC_INPUT_PTR inputBuffer, NV_ENC_OUTPUT_PTR outputBuffer, NV_ENC_PIC_PARAMS* pPicParams)
{
	NV_ENC_PIC_PARAMS picParams = {};
	if (pPicParams)
	{
		picParams = *pPicParams;
	}

	picParams.version = NV_ENC_PIC_PARAMS_VER;
	picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	picParams.inputBuffer = inputBuffer;
	picParams.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;
	picParams.inputWidth = m_initParams.encodeWidth;
	picParams.inputHeight = m_initParams.encodeHeight;
	picParams.outputBitstream = outputBuffer;
	picParams.completionEvent = m_vpCompletionEvent[m_nextIndex % m_nEncodeBuffer];
	NVENCSTATUS nvStatus = m_nvenc.nvEncEncodePicture(m_hEncoder, &picParams);

	return nvStatus;
}

void HEVCEncoder::EndEncode(std::ofstream& fpOut)
{
	totalFrames = (totalFrames + 1);
	m_vVPacket.clear();

	NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
	picParams.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
	picParams.completionEvent = m_vpCompletionEvent[m_nextIndex % m_nEncodeBuffer];
	NVENC_API_CALL(m_nvenc.nvEncEncodePicture(m_hEncoder, &picParams));

	EncodePacket(m_vOutputResNV, false);


	if (!streamed)
	{
		for (std::vector<uint8_t>& packet : m_vVPacket)
		{
			m_videoStream->Stream(packet, totalFrames, fpOut);
			streamed = true;
		}
	}



	encodedFrames += m_vVPacket.size();
}

void HEVCEncoder::EncodePacket(std::vector<NV_ENC_OUTPUT_RESOURCE_D3D12*>& vOutputBuffer, bool bOutputDelay)
{
	unsigned int i = 0;
	int iEnd = bOutputDelay ? m_nextIndex - m_nOutputDelay : m_nextIndex;

	for (; m_currentIndex < iEnd; m_currentIndex++)
	{
		DWORD waitStatus = WaitForSingleObject(m_vpCompletionEvent[m_currentIndex % m_nEncodeBuffer], 3000);
		if (waitStatus == WAIT_FAILED || waitStatus == WAIT_TIMEOUT)
		{
			throw std::exception("ERROR::WAIT FOR OBJECT FAILED.");
		}

		NV_ENC_LOCK_BITSTREAM lockBistreamData = { NV_ENC_LOCK_BITSTREAM_VER };
		lockBistreamData.outputBitstream = vOutputBuffer[m_currentIndex % m_nEncodeBuffer];
		lockBistreamData.doNotWait = false;
		NVENC_API_CALL(m_nvenc.nvEncLockBitstream(m_hEncoder, &lockBistreamData));

		uint8_t* pData = (uint8_t*)lockBistreamData.bitstreamBufferPtr;
		if (m_vVPacket.size() < i + 1)
		{
			m_vVPacket.push_back(std::vector<uint8_t>());
		}
		m_vVPacket[i].clear();

		m_vVPacket[i].insert(m_vVPacket[i].end(), &pData[0], &pData[lockBistreamData.bitstreamSizeInBytes]);
		i++;

		NVENC_API_CALL(m_nvenc.nvEncUnlockBitstream(m_hEncoder, lockBistreamData.outputBitstream));

		if (m_vMappedInputBuffers[m_currentIndex % m_nEncodeBuffer])
		{
			NVENC_API_CALL(m_nvenc.nvEncUnmapInputResource(m_hEncoder, m_vMappedInputBuffers[m_currentIndex % m_nEncodeBuffer]));
			m_vMappedInputBuffers[m_currentIndex % m_nEncodeBuffer] = nullptr;
		}

		if (m_vMappedOutputBuffers[m_currentIndex % m_nEncodeBuffer])
		{
			NVENC_API_CALL(m_nvenc.nvEncUnmapInputResource(m_hEncoder, m_vMappedOutputBuffers[m_currentIndex % m_nEncodeBuffer]));
			m_vMappedOutputBuffers[m_currentIndex % m_nEncodeBuffer] = nullptr;
		}
	}


}

void HEVCEncoder::MapResources(uint32_t brfIdx)
{
	NV_ENC_MAP_INPUT_RESOURCE mapInputResource = { NV_ENC_MAP_INPUT_RESOURCE_VER };
	mapInputResource.registeredResource = m_vRegisteredInputResources[brfIdx];
	NVENC_API_CALL(m_nvenc.nvEncMapInputResource(m_hEncoder, &mapInputResource));
	m_vMappedInputBuffers[brfIdx] = mapInputResource.mappedResource;

	NV_ENC_MAP_INPUT_RESOURCE mapInputResourceBitstream = { NV_ENC_MAP_INPUT_RESOURCE_VER };
	mapInputResourceBitstream.registeredResource = m_vRegisteredOutputResources[brfIdx];
	NVENC_API_CALL(m_nvenc.nvEncMapInputResource(m_hEncoder, &mapInputResourceBitstream));
	m_vMappedOutputBuffers[brfIdx] = mapInputResourceBitstream.mappedResource;
}

NvEncInputFrame* HEVCEncoder::GetNextInputFrame()
{
	int i = m_nextIndex % m_nEncodeBuffer;
	return &m_vInputFrames[i];
}

void HEVCEncoder::LoadNvidiaEncoderAPI()
{


	uint32_t version = 0;
	uint32_t currentVersion = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
	NVENC_API_CALL(NvEncodeAPIGetMaxSupportedVersion(&version));

	if (currentVersion > version)
	{
		NVENC_THROW_ERROR("Current Driver Version does not support this NvEncodeAPI version, please upgrade driver", NV_ENC_ERR_INVALID_VERSION);
	}
	
	m_nvenc = { NV_ENCODE_API_FUNCTION_LIST_VER };
	NVENC_API_CALL(NvEncodeAPICreateInstance(&m_nvenc));
}

void HEVCEncoder::CreateNvidiaEncoder(int sWidth, int sHeight)
{
	m_initParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	NV_ENC_CONFIG encConfig = { NV_ENC_CONFIG_VER };
	m_initParams.encodeConfig = &encConfig;

	SetNVIDIAEncoderParams(&m_initParams);
	
	ThrowIfFailed(m_Device->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_InputFence)));
	ThrowIfFailed(m_Device->GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_OutputFence)));

	m_Event = CreateEvent(nullptr, FALSE, FALSE, nullptr);


	NVENC_API_CALL(m_nvenc.nvEncInitializeEncoder(m_hEncoder, &m_initParams));

	m_nEncodeBuffer = m_initParams.encodeConfig->frameIntervalP + m_initParams.encodeConfig->rcParams.lookaheadDepth + m_nExtraOutputDelay;
	m_nOutputDelay = m_nEncodeBuffer - 1;

	m_vpCompletionEvent.resize(m_nEncodeBuffer, nullptr);

	for (uint32_t i = 0; i < m_vpCompletionEvent.size(); i++)
	{
		m_vpCompletionEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	m_vMappedInputBuffers.resize(m_nEncodeBuffer, nullptr);

	AllocateInputBuffers();
	AllocateOutputBuffers();

	m_vMappedOutputBuffers.resize(m_nEncodeBuffer, nullptr);
}

void HEVCEncoder::SetNVIDIAEncoderParams(NV_ENC_INITIALIZE_PARAMS* initParams)
{
	Application* app = Application::GetInstance();
	int sWidth = app->GetWidth();
	int sHeight = app->GetHeight();



	memset(initParams->encodeConfig, 0, sizeof(NV_ENC_CONFIG));
	auto pEncodeConfig = initParams->encodeConfig;
	memset(initParams, 0, sizeof(NV_ENC_INITIALIZE_PARAMS));
	initParams->encodeConfig = pEncodeConfig;

	initParams->version = NV_ENC_INITIALIZE_PARAMS_VER;
	initParams->encodeConfig->version = NV_ENC_CONFIG_VER;

	initParams->encodeGUID = NV_ENC_CODEC_HEVC_GUID;
	initParams->presetGUID = NV_ENC_PRESET_P1_GUID;
	initParams->encodeWidth = sWidth;
	initParams->encodeHeight = sHeight;
	initParams->darWidth = initParams->encodeWidth;
	initParams->darHeight = initParams->encodeHeight;
	initParams->frameRateNum = 60;
	initParams->frameRateDen = 1;
	initParams->enablePTD = 1;
	initParams->reportSliceOffsets = 0;
	initParams->enableSubFrameWrite = 0;
	initParams->maxEncodeWidth = initParams->encodeWidth;
	initParams->maxEncodeHeight = initParams->encodeHeight;
	initParams->enableMEOnlyMode = false;
	initParams->enableOutputInVidmem = false;

	NV_ENC_CAPS_PARAM capsParam = { NV_ENC_CAPS_PARAM_VER };
	capsParam.capsToQuery = NV_ENC_CAPS_ASYNC_ENCODE_SUPPORT;
	int isAsync;
	NVENC_API_CALL(m_nvenc.nvEncGetEncodeCaps(m_hEncoder, initParams->encodeGUID, &capsParam, &isAsync));
	initParams->enableEncodeAsync = isAsync;

	NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, {NV_ENC_CONFIG_VER } };
	NVENC_API_CALL(m_nvenc.nvEncGetEncodePresetConfig(m_hEncoder, NV_ENC_CODEC_HEVC_GUID, NV_ENC_PRESET_P1_GUID, &presetConfig));

	memcpy(initParams->encodeConfig, &presetConfig.presetCfg, sizeof(NV_ENC_CONFIG));

	// Frame interval must be 1 for infinite GoP Length;
	initParams->encodeConfig->frameIntervalP = 1;
	// GoP length is suggested to be set to infinite for live streaming
	initParams->encodeConfig->gopLength = NVENC_INFINITE_GOPLENGTH;

	initParams->encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
	//initParams->encodeConfig->rcParams.averageBitRate = 8000000;
	


	initParams->tuningInfo = NV_ENC_TUNING_INFO_LOW_LATENCY;
	NV_ENC_PRESET_CONFIG presetConfig2 = { NV_ENC_PRESET_CONFIG_VER, {NV_ENC_CONFIG_VER} };
	NVENC_API_CALL(m_nvenc.nvEncGetEncodePresetConfigEx(m_hEncoder, NV_ENC_CODEC_HEVC_GUID, NV_ENC_PRESET_P1_GUID, NV_ENC_TUNING_INFO_LOW_LATENCY, &presetConfig2));
	memcpy(initParams->encodeConfig, &presetConfig2.presetCfg, sizeof(NV_ENC_CONFIG));


	initParams->encodeConfig->rcParams.averageBitRate = 10000000;
	initParams->encodeConfig->encodeCodecConfig.hevcConfig.pixelBitDepthMinus8 = 0;

	// For live streaming all I frames should be IDR frames
	initParams->encodeConfig->encodeCodecConfig.hevcConfig.idrPeriod = initParams->encodeConfig->gopLength;

	initParams->bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
}

void HEVCEncoder::AllocateInputBuffers()
{
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = m_initParams.maxEncodeWidth;
	resourceDesc.Height = m_initParams.maxEncodeHeight;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resourceDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	m_vInputBuffers.resize(m_nEncodeBuffer);

	for (uint32_t i = 0; i < m_nEncodeBuffer; i++)
	{
		ThrowIfFailed(m_Device->GetD3D12Device()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vInputBuffers[i])));
	}

	RegisterInputBuffers();

	for (uint32_t i = 0; i < m_vInputBuffers.size(); i++)
	{
		NV_ENC_INPUT_RESOURCE_D3D12* pInpRes = new NV_ENC_INPUT_RESOURCE_D3D12();
		memset(pInpRes, 0, sizeof(NV_ENC_INPUT_RESOURCE_D3D12));
		pInpRes->inputFencePoint.pFence = m_InputFence.Get();

		m_vInputResNV.push_back(pInpRes);
	}
}

void HEVCEncoder::RegisterInputBuffers()
{
	for (uint32_t i = 0; i < m_vInputBuffers.size(); i++)
	{
		NV_ENC_FENCE_POINT_D3D12 regInputBuffFence;

		memset(&regInputBuffFence, 0, sizeof(NV_ENC_FENCE_POINT_D3D12));
		regInputBuffFence.pFence = m_InputFence.Get();
		regInputBuffFence.waitValue = m_inputFenceVal;
		regInputBuffFence.bWait = true;

		// Atomic ++
		InterlockedIncrement(&m_inputFenceVal);

		regInputBuffFence.signalValue = m_inputFenceVal;
		regInputBuffFence.bSignal = true;


		NV_ENC_REGISTERED_PTR regPtr = RegisterResource(m_vInputBuffers[i].Get(), NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX, m_initParams.encodeWidth, m_initParams.encodeHeight,
			0, NV_ENC_BUFFER_FORMAT_ARGB, NV_ENC_INPUT_IMAGE, &regInputBuffFence);
		

		NvEncInputFrame inputFrame = {};
		auto resDesc = m_vInputBuffers[i].Get()->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT inputUploadFootprint[2];
		
		m_Device->GetD3D12Device()->GetCopyableFootprints(&resDesc, 0, 1, 0, inputUploadFootprint, nullptr, nullptr, nullptr);

		inputFrame.inputPtr = (void*)m_vInputBuffers[i].Get();
		inputFrame.numChromaPlanes = 0;
		inputFrame.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
		inputFrame.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
		inputFrame.pitch = inputUploadFootprint[0].Footprint.RowPitch;


		m_vRegisteredInputResources.push_back(regPtr);
		m_vInputFrames.push_back(inputFrame);

		ID3D12Fence* fence = (ID3D12Fence*)regInputBuffFence.pFence;

		if (fence->GetCompletedValue() < regInputBuffFence.signalValue)
		{
			if (fence->SetEventOnCompletion(regInputBuffFence.signalValue, m_Event) != S_OK)
			{
				NVENC_THROW_ERROR("SetEventOnCompletion failed", NV_ENC_ERR_INVALID_PARAM);
			}
			WaitForSingleObject(m_Event, INFINITE);
		}

	}
}

NV_ENC_REGISTERED_PTR HEVCEncoder::RegisterResource(void* pBuffer, NV_ENC_INPUT_RESOURCE_TYPE resType, int width, int height, int pitch, NV_ENC_BUFFER_FORMAT format,
	NV_ENC_BUFFER_USAGE usage, NV_ENC_FENCE_POINT_D3D12* inputFence)
{
	NV_ENC_REGISTER_RESOURCE regResource = { NV_ENC_REGISTER_RESOURCE_VER };
	regResource.resourceType = resType;
	regResource.resourceToRegister = pBuffer;
	regResource.width = width;
	regResource.height = height;
	regResource.pitch = pitch;
	regResource.bufferFormat = format;
	regResource.bufferUsage = usage;
	regResource.pInputFencePoint = inputFence;
	
	NVENC_API_CALL(m_nvenc.nvEncRegisterResource(m_hEncoder, &regResource));

	return regResource.registeredResource;
}

void HEVCEncoder::AllocateOutputBuffers()
{
	uint32_t outputBufferSize = ALIGN_UP((m_initParams.encodeWidth * m_initParams.encodeHeight * 4) * 2, 4);

	D3D12_RESOURCE_STATES  initialResourceState = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = outputBufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	m_vOutputBuffers.resize(m_nEncodeBuffer);

	for (uint32_t i = 0; i < m_nEncodeBuffer; i++)
	{
		ThrowIfFailed(m_Device->GetD3D12Device()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialResourceState, nullptr, IID_PPV_ARGS(&m_vOutputBuffers[i])));
	}

	for (uint32_t i = 0; i < m_vOutputBuffers.size(); i++)
	{
		NV_ENC_REGISTERED_PTR regPtr = RegisterResource(m_vOutputBuffers[i].Get(), NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX, outputBufferSize, 1, 0, 
			NV_ENC_BUFFER_FORMAT_U8, NV_ENC_OUTPUT_BITSTREAM);
		
		m_vRegisteredOutputResources.push_back(regPtr);
	}

	for (uint32_t i = 0; i < m_vOutputBuffers.size(); ++i)
	{
		NV_ENC_OUTPUT_RESOURCE_D3D12* pOutRsrc = new NV_ENC_OUTPUT_RESOURCE_D3D12();
		memset(pOutRsrc, 0, sizeof(NV_ENC_OUTPUT_RESOURCE_D3D12));
		pOutRsrc->outputFencePoint.pFence = m_OutputFence.Get();
		m_vOutputResNV.push_back(pOutRsrc);
	}
}