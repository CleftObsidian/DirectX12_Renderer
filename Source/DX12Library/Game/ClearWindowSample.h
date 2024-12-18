#pragma once

#include "GameSample.h"

namespace DX12Library
{
	class ClearWindowSample final : public GameSample
	{
	public:
		ClearWindowSample(_In_ PCWSTR pszGameName);
		virtual ~ClearWindowSample();

		virtual void InitDevice();
		virtual void CleanupDevice();
        virtual void Update(_In_ FLOAT deltaTime);
		virtual void Render();

	private:
        // Pipeline objects.
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12Resource> m_renderTargets[FRAMECOUNT];
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize;

        // Synchronization objects.
        UINT m_frameIndex;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;
	};
}