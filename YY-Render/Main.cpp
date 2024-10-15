#include "stdafx.h"
#include "MyRender.h"
#include "Resource.h"
#include "test.h"
#include "Helper.h"
int main()
{
#ifndef YY_NOTEST
    // Add test function call here

#endif
    return 0;
}

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    wchar_t* titleName = L"YY-Render";
    const char* defaultModels[] = {
        "assets/teapot/teapot.obj",
        "assets/bunny.obj",
    };
    std::unique_ptr<MyRender>mr = std::make_unique<MyRender>(1280, 720, titleName, defaultModels[1]);
#ifdef _DEBUG
    return W32::Run(mr.get(), hInstance, nCmdShow);
#else
    int res{};
    std::unordered_map<HRESULT, PCSTR> errorHelper{};
    errorHelper[0x80004005] = "Possible reasons:\n(1) Shader compile failed";
    try {
        res = W32::Run(mr.get(), hInstance, nCmdShow);
    } catch (std::string e) {
        MessageBoxA(nullptr, std::format("ERROR: {}", e).c_str(),
            "Internal Error", MB_ICONWARNING);
        res = -1;
    } catch (HrException &hre) {
        auto helper = errorHelper.contains(hre.Error()) ? errorHelper[hre.Error()] : "";
        MessageBoxA(nullptr,
            std::format("DX12 ThrowIfFailed Exception:\n{}\n{}",
                hre.what(), helper).c_str(),
            "Internal Error", MB_ICONASTERISK);
        res = -2;
    } catch (...) {
        res = -3;
    }
    return res;
#endif
    //return W32::Run(&W32Handler::DefHandler, hInstance, nCmdShow);
}
