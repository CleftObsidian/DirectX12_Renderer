#pragma once

#include "DXSampleHelper.h"

using namespace DirectX;

namespace DX12Library
{
	HRESULT InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow);
    void InitDevice();
    void CleanupDevice();
    void Render();
}