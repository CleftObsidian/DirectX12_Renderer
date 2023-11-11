#pragma once

#include "GameSample.h"

using namespace DirectX;

namespace DX12Library
{
	class PushConstantSample final : public GameSample
	{
	public:
        PushConstantSample(_In_ PCWSTR pszGameName);
		virtual ~PushConstantSample();

		virtual void InitDevice();
        virtual void CleanupDevice();
        virtual void Update(_In_ FLOAT deltaTime);
		virtual void Render();
		
	private:
        struct Vertex
        {
            XMFLOAT3 position;
        };

        // Pipeline objects.
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12Resource> m_renderTargets[FRAMECOUNT];
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize = 0;

        // App resources.
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        UINT8* m_pCbvDataBegin;

        // Synchronization objects.
        UINT m_frameIndex = 0;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;
	};
}