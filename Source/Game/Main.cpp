#include "DXSampleHelper.h"
#include "Game/Game.h"

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(DX12Library::InitWindow(hInstance, nCmdShow)))
	{
		return 0;
	}

    DX12Library::InitDevice();

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
            DX12Library::Render();
        }
    }

    // Destroy
    DX12Library::CleanupDevice();

    return static_cast<INT>(msg.wParam);
}