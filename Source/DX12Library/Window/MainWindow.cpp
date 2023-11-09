#include "MainWindow.h"

namespace DX12Library
{
    MainWindow::MainWindow()
        : BaseWindow()
        , m_gameMode(1)
    {
    }

    HRESULT MainWindow::Initialize(_In_ HINSTANCE hInstance, _In_ INT nCmdShow, _In_ PCWSTR pszWindowName)
    {
        return initialize(hInstance, nCmdShow, pszWindowName, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
    }

    PCWSTR MainWindow::GetWindowClassName() const
    {
        return m_pszWindowName;
    }

    LRESULT MainWindow::HandleMessage(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        PAINTSTRUCT ps;
        HDC hdc;

        switch (uMsg)
        {
        case WM_KEYDOWN:
        {
            // Determine directions True
            switch (wParam)
            {
            case 0x31:
            case VK_NUMPAD1:
                m_gameMode = 1;
                break;

            case 0x32:
            case VK_NUMPAD2:
                m_gameMode = 2;
                break;

            case 0x33:
            case VK_NUMPAD3:
                m_gameMode = 3;
                break;

            default:
                break;
            }
            break;
        }

        case WM_PAINT:
        {
            hdc = BeginPaint(m_hWnd, &ps);
            EndPaint(m_hWnd, &ps);
            break;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }

        case WM_CLOSE:
        {
            if (MessageBox(m_hWnd,
                L"Really quit?",
                PSZ_TITLE,
                MB_OKCANCEL) == IDOK)
            {
                DestroyWindow(m_hWnd);
            }
            // else: user canceled. Do nothing.
            return S_OK;
        }

        default:
            return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
        }

        return S_OK;
    }

    UINT MainWindow::GetGameMode() const
    {
        return m_gameMode;
    }
}