#include "stdafx.h"
#include "test.h"

#ifndef YY_NOTEST
void test()
{
    HANDLE hr = CreateFileA("测试.txt",
                GENERIC_READ, FILE_SHARE_READ, nullptr,
                OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
    //assert(hr != INVALID_HANDLE_VALUE);
    HANDLE hmap = CreateFileMappingA(hr,
        nullptr,
        PAGE_READONLY,
        0, 0, nullptr);
    //assert(hmap != NULL);
    if (hmap == 0)
        return;
    void* p = MapViewOfFile(hmap,
        FILE_MAP_READ,
        0, 0, 26);
    if (!p)
        return;
    char* cp = reinterpret_cast<char*>(p);
    size_t count = 0;

    for (size_t i = 0; i < 26; i++)
    {
        if (cp[i] == '\n') count++;
    }

    std::cout << count << std::endl;
    count = 0;
    if (p)
    {
        UnmapViewOfFile(p);
    }

    if (hmap && hr)
    {
        CloseHandle(hmap);
        CloseHandle(hr);
    }

}

void test1()
{
    const std::string input = "3.1416";
    double result = 0.0;
    auto t1 = std::chrono::high_resolution_clock::now();
    fast_float::from_chars_result answer;
    const char* p = input.data();
    const size_t size = input.size();
    for (int i = 0; i < 1000000; i++)
    {
        answer = fast_float::from_chars(p, p + size, result);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    auto ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << ms_int.count() << "ms\n";
    float res2;
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; i++)
    {
        //float res = strtof(p, nullptr);
        res2 = std::stof(input);
    }
    t2 = std::chrono::high_resolution_clock::now();
    ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << ms_int.count() << "ms\n";
    t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; i++)
    {
        res2 = strtof(p, nullptr);
    }
    t2 = std::chrono::high_resolution_clock::now();
    ms_int = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
    std::cout << ms_int.count() << "ms\n";
}

void test2(void)
{
    const char a[] = "       ";
    float v;
    auto result = fast_float::from_chars(a, a + sizeof(a), v);
    if (result.ec != std::errc())
    {
        std::cout << std::make_error_code(result.ec).message() << std::endl;
    }
    const char b[] = "\n";
    result = fast_float::from_chars(b, static_cast<char*>(nullptr), v);
    if (result.ec != std::errc())
    {
        std::cout << std::make_error_code(result.ec).message() << std::endl;
    }
    else
    {
        std::cout << v;
        std::cout << result.ptr - b;
    }
}

void test3(void)
{
    std::string path = "assets/pure_color/red.png";
    D3D12_RESOURCE_DESC desc{};
    std::unique_ptr<BYTE[]> data{};
    int bytesinRow{}, imageSize{};
    auto res = LoadImageDataFromFile(data, desc, path, bytesinRow, imageSize);
    if (res != LoadTexError::Success)
    {
        std::cout << "Load failed 1 " << static_cast<int>(res);
        return;
    }
    std::cout << "DXGI TYPE: " << desc.Format << '\n';
    std::cout << std::format("size: {}, row: {}\n", imageSize, bytesinRow);
}


#endif
