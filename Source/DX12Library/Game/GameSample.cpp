#include "GameSample.h"

namespace DX12Library
{
    GameSample::GameSample(_In_ PCWSTR pszGameName)
        : m_pszGameName(pszGameName)
        , m_mainWindow(std::make_unique<MainWindow>())
    {
    }

    GameSample::~GameSample()
    {
    }

    HRESULT GameSample::Initialize(HINSTANCE hInstance, INT nCmdShow)
    {
        if (FAILED(m_mainWindow->Initialize(hInstance, nCmdShow, m_pszGameName)))
        {
            return E_FAIL;
        }

        InitDevice();

        return S_OK;
    }

    INT GameSample::Run()
    {
        // Main message loop
        MSG msg = { 0 };
        while (WM_QUIT != msg.message)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                // Call WndProc Function
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                // Render
                Update();
                Render();
            }
        }

        // Destroy
        CleanupDevice();

        return static_cast<INT>(msg.wParam);
    }

    PCWSTR GameSample::GetGameName() const
    {
        return m_pszGameName;
    }

    std::unique_ptr<MainWindow>& GameSample::GetWindow()
    {
        return m_mainWindow;
    }
}