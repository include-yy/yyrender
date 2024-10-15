
/**
* Image Texture loader by Joon1221
* https://github.com/Joon1221/DX12-object-loader
* This function comes from https://github.com/Joon1221/DX12-object-loader/blob/master/DX12%20Object%20Loader/BzTuts/d3dApp.cpp#L746
* Originally I write this by myself, but Joon1221's(also, maybe he/she/them got it from Frank Luna, the author of
* Introduction to 3D Game Programming with DirectX 12) is better.
* My original code is on the buttom of this file.
**/

#include "stdafx.h"
#include "LoadTexture.h"

using Microsoft::WRL::ComPtr;

static DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
    auto& type = wicFormatGUID;
    if (type == GUID_WICPixelFormat128bppRGBAFloat)
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    else if (type == GUID_WICPixelFormat64bppRGBAHalf)
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
    else if (type == GUID_WICPixelFormat64bppRGBA)
        return DXGI_FORMAT_R16G16B16A16_UNORM;
    else if (type == GUID_WICPixelFormat32bppRGBA)
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (type == GUID_WICPixelFormat32bppBGRA)
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    else if (type == GUID_WICPixelFormat32bppBGR)
        return DXGI_FORMAT_B8G8R8X8_UNORM;
    else if (type == GUID_WICPixelFormat32bppRGBA1010102XR)
        return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
    else if (type == GUID_WICPixelFormat32bppRGBA1010102)
        return DXGI_FORMAT_R10G10B10A2_UNORM;

    else if (type == GUID_WICPixelFormat16bppBGRA5551)
        return DXGI_FORMAT_B5G5R5A1_UNORM;
    else if (type == GUID_WICPixelFormat16bppBGR565)
        return DXGI_FORMAT_B5G6R5_UNORM;
    else if (type == GUID_WICPixelFormat32bppGrayFloat)
        return DXGI_FORMAT_R32_FLOAT;
    else if (type == GUID_WICPixelFormat16bppGrayHalf)
        return DXGI_FORMAT_R16_FLOAT;
    else if (type == GUID_WICPixelFormat16bppGray)
        return DXGI_FORMAT_R16_UNORM;
    else if (type == GUID_WICPixelFormat8bppGray)
        return DXGI_FORMAT_R8_UNORM;
    else if (type == GUID_WICPixelFormat8bppAlpha)
        return DXGI_FORMAT_A8_UNORM;
    else
        return DXGI_FORMAT_UNKNOWN;
}

// get a dxgi compatible wic format from another wic format
static WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
    if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
    else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
    else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

    else return GUID_WICPixelFormatDontCare;
}

// get the number of bits per pixel for a dxgi format
static int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
    if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
    else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

    else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
    else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
    else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
    else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
    else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
    else return 0;
}

LoadTexError LoadImageDataFromFile(std::unique_ptr<BYTE[]>& imgData, D3D12_RESOURCE_DESC& resDesc, std::string path, int& bytesPerRow, int& imageSize)
{
    HRESULT hr{};
    static ComPtr<IWICImagingFactory> wicFactory{};

    ComPtr<IWICBitmapDecoder> wicDecoder{};
    ComPtr<IWICBitmapFrameDecode> wicFrame{};
    ComPtr<IWICFormatConverter> wicConverter{};

    bool imageConverted = false;
    if (wicFactory == nullptr)
    {
        // Initialize the COM library
        hr = CoInitialize(nullptr);
        if (FAILED(hr))
            return LoadTexError::InitError;
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&wicFactory));
        if (FAILED(hr))
            return LoadTexError::InitError;
    }

    // convert utf-8 path to utf-16 wpath (UTF-8 to WCHAR)
    // https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar
    std::wstring wpath;
    int convertResult = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.size()), nullptr, 0);
    if (convertResult <= 0)
    {
        return LoadTexError::PathConvertError;
    }
    else
    {
        wpath.resize(convertResult + 10);
        convertResult = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), static_cast<int>(path.size()), &wpath[0], static_cast<int>(wpath.size()));
        if (convertResult <= 0)
        {
            return LoadTexError::PathConvertError;
        }
    }

    hr = wicFactory->CreateFormatConverter(&wicConverter);
    if (FAILED(hr))
        return LoadTexError::CreateFormatConverterError;

    // load a decoder for the image
    hr = wicFactory->CreateDecoderFromFilename(
        wpath.c_str(),
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &wicDecoder);
    if (FAILED(hr))
        return LoadTexError::CreateDecoderError;

    // get image from decoder
    hr = wicDecoder->GetFrame(0, &wicFrame);
    if (FAILED(hr))
        return LoadTexError::GetFrameError;

    // get wic pixel format of image
    WICPixelFormatGUID pixelFormat{};
    hr = wicFrame->GetPixelFormat(&pixelFormat);
    if (FAILED(hr))
        return LoadTexError::GetPixelFormatError;

    // get size of image
    UINT textureWidth{}, textureHeight{};
    hr = wicFrame->GetSize(&textureWidth, &textureHeight);
    if (FAILED(hr))
        return LoadTexError::GetSizeError;

    // convert wic pixel format to dxgi pixel format
    DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

    // if the format of the image is not a supported dxgi format, try to convert it
    if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
    {
        WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);
        if (convertToPixelFormat == GUID_WICPixelFormatDontCare)
            return LoadTexError::FormatNotSupportError;
        dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);
        BOOL canConvert = FALSE;
        hr = wicConverter->Initialize(wicFrame.Get(), convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
            return LoadTexError::ConvertFormatError;
        imageConverted = true;
    }
    int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat);
    if (bitsPerPixel == 0)
        return LoadTexError::BitsPerPixelZeroError;
    bytesPerRow = (textureWidth * bitsPerPixel) / 8;
    imageSize = bytesPerRow * textureHeight;
    //*imgData = (BYTE*)malloc(imageSize);
    imgData = std::make_unique<BYTE[]>(imageSize);

    // copy (decoded) raw image data into the newly allocated memory
    if (imageConverted)
    {
        hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, imgData.get());
        if (FAILED(hr))
            return LoadTexError::DataCopyError;
    }
    else
    {
        hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, imgData.get());
        if (FAILED(hr))
            return LoadTexError::DataCopyError;
    }

    // set texture description
    resDesc = {};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resDesc.Alignment = 0;
    resDesc.Width = textureWidth;
    resDesc.Height = textureHeight;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Format = dxgiFormat;
    resDesc.SampleDesc.Count = 1;
    resDesc.SampleDesc.Quality = 0;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    return LoadTexError::Success;
}

// Old version by myself
//void OBJLoadImage(std::wstring path, std::unique_ptr<BYTE[]>& p, UINT& size, UINT& width, UINT& height)
//{
//    auto pWIC = GetWIC();
//    if (!pWIC)
//    {
//        std::cout << "GetWIC failed";
//        exit(1);
//    }
//    ComPtr<IWICBitmapDecoder> decoder;
//    HRESULT hr = pWIC->CreateDecoderFromFilename(path.c_str(),
//        nullptr,
//        GENERIC_READ,
//        WICDecodeMetadataCacheOnDemand,
//        decoder.GetAddressOf());
//    if (FAILED(hr))
//    {
//        std::cout << "CreateDecoder failed" << hr;
//        exit(1);
//    }
//    ComPtr<IWICBitmapFrameDecode> frame;
//    hr = decoder->GetFrame(0, frame.GetAddressOf());
//    if (FAILED(hr))
//    {
//        std::cout << "GetFrame failed" << hr;
//        exit(1);
//    }
//    hr = frame->GetSize(&width, &height);
//    if (FAILED(hr))
//    {
//        std::cout << "GetSize failed" << hr;
//        exit(1);
//    }
//    WICPixelFormatGUID pixelFormat;
//    hr = frame->GetPixelFormat(&pixelFormat);
//    if (FAILED(hr))
//    {
//        std::cout << "getpixelformat failed";
//        exit(1);
//    }
//    WICPixelFormatGUID convertGUID;
//    memcpy_s(&convertGUID, sizeof(WICPixelFormatGUID), &GUID_WICPixelFormat32bppRGBA, sizeof(GUID));
//    size_t bpp = 32;
//    DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    const uint64_t rowBytes = width * 4;
//    const uint64_t numBytes = height * rowBytes;
//    size = numBytes;
//    auto const rowPitch = static_cast<size_t>(rowBytes);
//    auto const imageSize = static_cast<size_t>(numBytes);
//    p = std::make_unique<BYTE[]>(numBytes);
//
//    if (memcmp(&convertGUID, &pixelFormat, sizeof(GUID)) == 0)
//    {
//        hr = frame->CopyPixels(nullptr, static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize), p.get());
//        if (FAILED(hr))
//        {
//            std::cout << "CopyPixels failed" << hr;
//        }
//    }
//    else
//    {
//        ComPtr<IWICFormatConverter> FC;
//        hr = pWIC->CreateFormatConverter(FC.GetAddressOf());
//        if (FAILED(hr))
//        {
//            std::cout << "CreateFormatConverter failed" << hr;
//            exit(1);
//        }
//        BOOL canConvert = FALSE;
//        hr = FC->CanConvert(pixelFormat, convertGUID, &canConvert);
//        if (FAILED(hr) || !canConvert)
//        {
//            std::cout << "Cannot convert" << hr;
//            exit(1);
//        }
//        hr = FC->Initialize(frame.Get(), convertGUID, WICBitmapDitherTypeErrorDiffusion, nullptr, 0, WICBitmapPaletteTypeMedianCut);
//        if (FAILED(hr))
//        {
//            std::cout << "FC initialize failed" << hr;
//            exit(1);
//        }
//        hr = FC->CopyPixels(nullptr, static_cast<UINT>(rowPitch), static_cast<UINT>(imageSize), p.get());
//        if (FAILED(hr))
//        {
//            std::cout << "CopyPixels failed" << hr;
//        }
//    }
//}
