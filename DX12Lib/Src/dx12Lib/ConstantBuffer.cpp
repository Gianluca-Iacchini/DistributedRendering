#include <pch.h>

#include <dx12Lib/ConstantBuffer.h>
#include <dx12Lib/Device.h>

using namespace dx12lib;

ConstantBuffer::ConstantBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource )
: Buffer( device, resource )
{
    m_SizeInBytes = GetD3D12ResourceDesc().Width;
}

ConstantBuffer::~ConstantBuffer() {}
