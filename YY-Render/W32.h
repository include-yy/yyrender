// partially from https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12DynamicIndexing/src/Win32Application.h

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

class W32Handler;

class W32
{
public:
    static int Run(W32Handler* handler, HINSTANCE hInst, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }

protected:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
};

class W32Handler
{
public:
    // Called after CreateWindow
    virtual void OnInit();
    // Called after Message loop finish
    virtual void OnDestroy();
    // Called after get WM_PAINT message
    virtual void OnUpdate();
    // Called after OnUpdate
    virtual void OnPaint();

    // Various Window Message
    virtual void OnKeyDown(UINT8 key);
    virtual void OnKeyUp(UINT8 key);
    virtual void OnMouseDown(WPARAM btnState, int x, int y);
    virtual void OnMouseUp(WPARAM btnState, int x, int y);
    virtual void OnMouseMove(WPARAM btnState, int x, int y);
    virtual void OnMouseWheel(int delta);
    // Message from menu
    virtual void OnCommand(WORD id);

    // Accessers
    virtual UINT GetWidth() const;
    virtual UINT GetHeight() const;
    virtual const WCHAR* GetTitle() const;

    // ModalState Runner
    template<typename F>
    void RunWithModal(F f)
    {
        m_ModalStateCount = 1;
        f();
        m_ModalStateCount = 0;
        InvalidateRect(W32::GetHwnd(), nullptr, false);
    }
    bool IsModal() const;

    // Default Windows Message Handler
    static W32Handler DefHandler;
private:
    static INT m_ModalStateCount;
};
