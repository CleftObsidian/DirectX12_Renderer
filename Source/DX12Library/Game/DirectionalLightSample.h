#pragma once

#include "GameSample.h"

namespace DX12Library
{
	class DirectionalLightSample : public GameSample
	{
	public:
		DirectionalLightSample(_In_ PCWSTR pszGameName);
		~DirectionalLightSample();

        virtual void InitDevice();
        virtual void CleanupDevice();
        virtual void Update(_In_ FLOAT deltaTime);
        virtual void Render();

    private:
        static const UINT TEXTURE_WIDTH = 256;
        static const UINT TEXTURE_HEIGHT = 256;
        static const UINT TEXTURE_PIXEL_SIZE = 4;   // The number of bytes used to represent a pixel in the texture.

        struct Vertex
        {
            XMFLOAT3 Position;
            XMFLOAT2 TexCoord;
            XMFLOAT3 Normal;
        };

        struct TransformMatrix
        {
            XMMATRIX World;
            XMMATRIX View;
            XMMATRIX Projection;
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
        ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize = 0;
        
        // App resources.
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        ComPtr<ID3D12Resource> m_depthBuffer;
        ComPtr<ID3D12Resource> m_texture;

        // Synchronization objects.
        UINT m_frameIndex = 0;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;

        TransformMatrix m_transformMatrix;
	};
}