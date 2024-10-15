// Partially from https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/DXSample.h

//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
#include "W32.h"

class Framework : public W32Handler
{
public:
    Framework(UINT width, UINT height, std::wstring name);
    virtual ~Framework();

    //Accessors.
    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    const WCHAR* GetTitle() const { return m_title.c_str(); }

protected:
    // Get assets path related to EXE's directory
    std::wstring GetAssetFullPath(LPCWSTR assetName) const;
    // The same as GetAssetFullPath, but UTF-8 (not UTF-16)
    std::string GetAssetFullPathA(LPCSTR assetName) const;
    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);
    // Set Window Title Text
    void SetCustomWindowText(LPCWSTR text) const;

    // Viewport dimensions
    UINT m_width{};
    UINT m_height{};
    float m_aspectRatio{};

    // Adapter info
    bool m_useWarpDevice{};
private:
    // Root assets path
    std::wstring m_assetsPath;
    std::string m_assetsPathA;
    // Window title.
    std::wstring m_title;
};
