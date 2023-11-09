#pragma once

#include "DXSampleHelper.h"

using namespace DirectX;

namespace DX12Library
{
	class GameSample
	{
	public:
		GameSample();
		virtual ~GameSample();

		HRESULT InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow);
		virtual void InitDevice() = 0;
		virtual void CleanupDevice() = 0;
		virtual void Render() = 0;

	protected:
		HINSTANCE m_hInst;
		HWND m_hWnd;

		// Pipeline objects.
		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;
	};

	
}