#pragma once

enum class LoadTexError
{
    Success,
    InitError,
    PathConvertError,
    CreateFormatConverterError,
    CreateDecoderError,
    GetFrameError,
    GetPixelFormatError,
    GetSizeError,
    FormatNotSupportError,
    ConvertFormatError,
    BitsPerPixelZeroError,
    DataCopyError,
};

LoadTexError LoadImageDataFromFile(std::unique_ptr<BYTE[]>& imgData, D3D12_RESOURCE_DESC& resDesc, std::string path, int& bytesPerRow, int& imageSize);
