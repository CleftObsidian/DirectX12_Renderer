#include "GameSample.h"

namespace DX12Library
{
    LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        PAINTSTRUCT ps;
        HDC hdc;

        switch (uMsg)
        {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            if (MessageBox(hWnd,
                L"Really quit?",
                PSZ_TITLE,
                MB_OKCANCEL) == IDOK)
            {
                DestroyWindow(hWnd);
            }
            // Else: user canceled. Do nothing.
            return S_OK;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

        return S_OK;
    }

    GameSample::GameSample()
        : m_hInst()
        , m_hWnd()
    {
    }

    GameSample::~GameSample()
    {
    }

    HRESULT GameSample::InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow)
    {
        // Register class
        WNDCLASSEX wcex =
        {
            .cbSize = static_cast<UINT>(sizeof(WNDCLASSEX)),
            .style = static_cast<UINT> (CS_HREDRAW | CS_VREDRAW),
            .lpfnWndProc = WindowProc,
            .cbClsExtra = 0,
            .hInstance = hInstance,
            .hIcon = LoadIcon(hInstance, reinterpret_cast<LPCTSTR>(IDI_TUTORIAL1)),
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
            .lpszMenuName = nullptr,
            .lpszClassName = L"DX12Renderer",
            .hIconSm = LoadIcon(wcex.hInstance, reinterpret_cast<LPCTSTR>(IDI_TUTORIAL1))
        };

        if (!RegisterClassEx(&wcex))
        {
            DWORD dwError = GetLastError();

            MessageBox(
                nullptr,
                L"Call to RegisterClassEx failed!",
                PSZ_TITLE,
                NULL
            );

            if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            {
                return HRESULT_FROM_WIN32(dwError);
            }

            return E_FAIL;
        }

        // Create window
        m_hInst = hInstance;
        RECT rc = { 0, 0, 1280, 720 };
        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 1280.0f, 720.0f);
        m_scissorRect = CD3DX12_RECT(0, 0, 1280, 720);
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        m_hWnd = CreateWindow(L"DX12Renderer", PSZ_TITLE,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
            nullptr);
        if (!m_hWnd)
        {
            DWORD dwError = GetLastError();

            MessageBox(
                nullptr,
                L"Call to CreateWindowEx failed!",
                PSZ_TITLE,
                NULL
            );

            if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            {
                return HRESULT_FROM_WIN32(dwError);
            }

            return E_FAIL;
        }

        ShowWindow(m_hWnd, nCmdShow);

        return S_OK;
    }
}