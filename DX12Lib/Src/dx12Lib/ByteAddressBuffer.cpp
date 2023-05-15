#include <pch.h>

#include <dx12Lib/ByteAddressBuffer.h>
#include <dx12Lib/Device.h>

using namespace dx12lib;


ByteAddressBuffer::ByteAddressBuffer( Device& device, const D3D12_RESOURCE_DESC& resDesc )
: Buffer( device, resDesc )
{}

ByteAddressBuffer::ByteAddressBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource )
: Buffer( device, resource )
{}

