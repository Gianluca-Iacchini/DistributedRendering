#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windowsx.h>

#include <D3D12/d3dx12.h>
#include <d3dcompiler.h>

#include <comdef.h>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <DirectxTex/DirectXTex.h>

// Functions from https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/UWP/D3D12xGPU/src/DXSampleHelper.h
inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};


inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        // Set a breakpoint on this line to catch DirectX API errors
        throw std::exception();
    }
}