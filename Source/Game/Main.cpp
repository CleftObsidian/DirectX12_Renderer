#include "Game/ClearWindowSample.h"
#include "Game/PushConstantSample.h"
#include "Game/TriangleControlSample.h"
#include "Game/SimpleCubeSample.h"
#include "Game/TextureCubeSample.h"
#include "Game/DirectionalLightSample.h"

INT WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    // std::unique_ptr<DX12Library::ClearWindowSample> game = std::make_unique<DX12Library::ClearWindowSample>(PSZ_TITLE);
    // std::unique_ptr<DX12Library::PushConstantSample> game = std::make_unique<DX12Library::PushConstantSample>(PSZ_TITLE);
    // std::unique_ptr<DX12Library::TriangleControlSample> game = std::make_unique<DX12Library::TriangleControlSample>(PSZ_TITLE);
    // std::unique_ptr<DX12Library::SimpleCubeSample> game = std::make_unique<DX12Library::SimpleCubeSample>(PSZ_TITLE);
    // std::unique_ptr<DX12Library::TextureCubeSample> game = std::make_unique<DX12Library::TextureCubeSample>(PSZ_TITLE);
    std::unique_ptr<DX12Library::DirectionalLightSample> game = std::make_unique<DX12Library::DirectionalLightSample>(PSZ_TITLE);

	if (FAILED(game->Initialize(hInstance, nCmdShow)))
	{
		return 0;
	}

    return game->Run();
}