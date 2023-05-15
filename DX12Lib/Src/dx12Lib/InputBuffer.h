#pragma once
#include "Buffer.h"

namespace dx12lib
{
	class InputBuffer : public Buffer
	{
	protected:
		InputBuffer(Device& device, const D3D12_RESOURCE_DESC& resDesc);
		InputBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource);
		virtual ~InputBuffer() = default;

		int m_Width;
		int m_Height;
		DXGI_FORMAT m_TexFormat;
	};
}

