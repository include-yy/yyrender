#include "stdafx.h"
#include "W32.h"
#include "Resource.h"

HWND W32::m_hwnd = nullptr;
W32Handler W32Handler::DefHandler;
INT W32Handler::m_ModalStateCount = 0;

int W32::Run(W32Handler* handler, HINSTANCE hInstance, int nCmdShow)
{
    // Initialize the window class
    WNDCLASSEX ws = { 0 };
    ws.cbSize = sizeof(WNDCLASSEX);
    ws.style = CS_HREDRAW | CS_VREDRAW;
    ws.lpfnWndProc = WndProc;
    ws.hInstance = hInstance;
    ws.hCursor = LoadCursor(NULL, IDC_ARROW);
    ws.lpszClassName = L"yy";
    ws.hIcon = LoadIcon(hInstance, L"smile");
    RegisterClassExW(&ws);

    RECT windowRect = { 0, 0, static_cast<LONG>(handler->GetWidth()), static_cast<LONG>(handler->GetHeight()) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create window
    m_hwnd = CreateWindow(
        ws.lpszClassName,
        handler->GetTitle(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        handler);

    // Initialize handler's resource
    handler->OnInit();

    ShowWindow(m_hwnd, nCmdShow);

    // Main message loop
    MSG msg{};
    while (msg.message != WM_QUIT)
    {
        // Peek any message in the queue
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // Destroy handler
    handler->OnDestroy();

    // Return WM_QUIT's WPARAM to Windows
    return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK W32::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    W32Handler* pHander = reinterpret_cast<W32Handler*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        return 0;
    }
    case WM_COMMAND:
        if (pHander) pHander->OnCommand(LOWORD(wParam));
        return 0;
    case WM_KEYDOWN:
        if (pHander) pHander->OnKeyDown(static_cast<UINT8>(wParam));
        return 0;
    case WM_KEYUP:
        if (pHander) pHander->OnKeyUp(static_cast<UINT8>(wParam));
        return 0;
    case WM_MOUSEMOVE:
        if (pHander)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            pHander->OnMouseMove(wParam, xPos, yPos);
        }
        return 0;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        if (pHander)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            pHander->OnMouseDown(wParam, xPos, yPos);
            SetCapture(m_hwnd);
        }
        return 0;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
        if (pHander)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            pHander->OnMouseUp(wParam, xPos, yPos);
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (pHander)
        {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            zDelta = (zDelta) / WHEEL_DELTA;
            pHander->OnMouseWheel(zDelta);
        }
        return 0;
    case WM_PAINT:
        if (pHander)
        {
            // see https://stackoverflow.com/q/59471442
            // We need DefWindowProc to "eat" WM_PAINT to avoid dead message loop
            if (pHander->IsModal())
            {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
            pHander->OnUpdate();
            pHander->OnPaint();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    // Handle any messages the switch statement didn't
    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Members of W32Handler
void W32Handler::OnInit() {}
void W32Handler::OnDestroy() {}
void W32Handler::OnUpdate() {}
void W32Handler::OnPaint() {}
void W32Handler::OnKeyDown(UINT8 key) {}
void W32Handler::OnKeyUp(UINT8 key) {}
void W32Handler::OnMouseDown(WPARAM btnState, int x, int y) {}
void W32Handler::OnMouseUp(WPARAM btnState, int x, int y) {}
void W32Handler::OnMouseMove(WPARAM btnState, int x, int y) {}
void W32Handler::OnMouseWheel(int delta) {}
void W32Handler::OnCommand(WORD id) {}
UINT W32Handler::GetWidth() const { return 1280; }
UINT W32Handler::GetHeight() const { return 720; }
const WCHAR* W32Handler::GetTitle() const { return L"yy-render"; };
bool W32Handler::IsModal() const { return m_ModalStateCount != 0; }

/* templated function must in header file */

//template<typename F>
//void W32Handler::RunWithModal(F f)
//{
//    m_ModalStateCount = 1;
//    f();
//    m_ModalStateCount = 0;
//    InvalidateRect(W32::GetHwnd(), nullptr, false);
//}
