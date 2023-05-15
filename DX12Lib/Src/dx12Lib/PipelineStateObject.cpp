#include <pch.h>

#include <dx12Lib/PipelineStateObject.h>

#include <dx12Lib/Device.h>

using namespace dx12lib;

PipelineStateObject::PipelineStateObject(Device& device, const D3D12_PIPELINE_STATE_STREAM_DESC& desc)
    : m_Device(device)
{
    auto d3d12Device = device.GetD3D12Device();

    try {
        ThrowIfFailed(d3d12Device->CreatePipelineState(&desc, IID_PPV_ARGS(&m_d3d12PipelineState)));
    }
    catch (std::exception e)
    {
        printf(e.what());
    }
}