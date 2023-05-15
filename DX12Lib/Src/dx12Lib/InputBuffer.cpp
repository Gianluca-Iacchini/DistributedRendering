#include "pch.h"
#include "InputBuffer.h"

using namespace dx12lib;

InputBuffer::InputBuffer(Device& device, const D3D12_RESOURCE_DESC& resDesc) : Buffer(device, resDesc)
{
	m_Width = resDesc.Width;
	m_Height = resDesc.Height;
	m_TexFormat = resDesc.Format;
}

InputBuffer::InputBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource) : Buffer(device, resource)
{
	auto resDesc = resource->GetDesc();

	m_Width = resDesc.Width;
	m_Height = resDesc.Height;
	m_TexFormat = resDesc.Format;
}