#pragma once

#include "GameSample.h"

namespace DX12Library
{
	class TriangleControlSample final : public GameSample
	{
	public:
		TriangleControlSample(_In_ PCWSTR pszGameName);
		virtual ~TriangleControlSample();

        virtual void InitDevice();
        virtual void CleanupDevice();
        virtual void Update();
        virtual void Render();

	private:
        struct Vertex
        {
            XMFLOAT3 position;
        };

        struct ColorConstantBuffer
        {
            XMFLOAT4 color;
            float padding[60]; // Padding so the constant buffer is 256-byte aligned.
        };
        static_assert((sizeof(ColorConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

        // Pipeline objects.
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12Resource> m_renderTargets[FRAMECOUNT];
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize = 0;

        // App resources.
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        ComPtr<ID3D12Resource> m_constantBuffer;
        ColorConstantBuffer m_constantBufferData;
        UINT8* m_pCbvDataBegin;

        // Synchronization objects.
        UINT m_frameIndex = 0;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;
	};
}