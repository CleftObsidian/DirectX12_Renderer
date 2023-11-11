#pragma once

#include "GameSample.h"

namespace DX12Library
{
	class SimpleCubeSample : public GameSample
	{
	public:
		SimpleCubeSample(_In_ PCWSTR pszGameName);
		~SimpleCubeSample();

        virtual void InitDevice();
        virtual void CleanupDevice();
        virtual void Update(_In_ FLOAT deltaTime);
        virtual void Render();

    private:
        struct VertexPosColor
        {
            XMFLOAT3 position;
            XMFLOAT3 color;
        };

        // Pipeline objects.
        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12Resource> m_renderTargets[FRAMECOUNT];
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize = 0;

        // App resources.
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        ComPtr<ID3D12Resource> m_depthBuffer;

        // Synchronization objects.
        UINT m_frameIndex = 0;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;

        XMMATRIX m_modelMatrix;
        XMMATRIX m_viewMatrix;
        XMMATRIX m_projectionMatrix;
	};
}