#include "DXSampleHelper.h"
#include "Game/ClearWindowSample.h"
#include "Game/PushConstantSample.h"

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    // DX12Library::ClearWindowSample game;
    DX12Library::PushConstantSample game;

	if (FAILED(game.InitWindow(hInstance, nCmdShow)))
	{
		return 0;
	}

    game.InitDevice();

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
            game.Render();
        }
    }

    // Destroy
    game.CleanupDevice();

    return static_cast<INT>(msg.wParam);
}