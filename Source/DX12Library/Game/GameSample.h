#pragma once

#include "DXSampleHelper.h"
#include "Window/MainWindow.h"

using namespace DirectX;

namespace DX12Library
{
	class GameSample
	{
	public:
		GameSample(_In_ PCWSTR pszGameName);
		virtual ~GameSample();

		HRESULT Initialize(_In_ HINSTANCE hInstance, _In_ INT nCmdShow);
		INT Run();

		virtual void InitDevice() = 0;
		virtual void CleanupDevice() = 0;
		virtual void Update() = 0;
		virtual void Render() = 0;

		PCWSTR GetGameName() const;
		std::unique_ptr<MainWindow>& GetWindow();

	protected:
		static const UINT FRAMECOUNT = 2;

		PCWSTR m_pszGameName;
		std::unique_ptr<MainWindow> m_mainWindow;

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
	};
}