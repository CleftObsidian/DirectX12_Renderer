#include "Game.h"

namespace DX12Library
{
	void PrintHello()
	{
		OutputDebugString(L"Hello\n");
		MessageBox(nullptr, L"Hello", L"DirectX 12 Renderer", MB_OK);
	}
}