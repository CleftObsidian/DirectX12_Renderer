#pragma once

#include "GameSample.h"

struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiAnimation;
struct aiBone;
struct aiNode;
struct aiNodeAnim;

namespace Assimp
{
    class Importer;
}

namespace DX12Library
{
	class MoonSample : public GameSample
	{
	public:
		MoonSample(_In_ PCWSTR pszGameName);
		~MoonSample();

        virtual void InitDevice();
        virtual void CleanupDevice();
        virtual void Update(_In_ FLOAT deltaTime);
        virtual void Render();

    protected:
        static std::unique_ptr<Assimp::Importer> sm_pImporter;

    private:
        static const UINT TEXTURE_WIDTH = 256;
        static const UINT TEXTURE_HEIGHT = 256;
        static const UINT TEXTURE_PIXEL_SIZE = 4;   // The number of bytes used to represent a pixel in the texture.

        static constexpr const UINT INVALID_MATERIAL = (0xFFFFFFFF);

        struct Vertex
        {
            XMFLOAT3 Position;
            XMFLOAT2 TexCoord;
            XMFLOAT3 Normal;
        };

        struct ConstantBuffer
        {
            XMMATRIX World = XMMatrixIdentity();
            XMMATRIX View = XMMatrixIdentity();
            XMMATRIX Projection = XMMatrixIdentity();
            XMFLOAT3 CameraPos = XMFLOAT3();
        };

        struct BasicMeshEntry
        {
            BasicMeshEntry()
                : uNumIndices(0u)
                , uBaseVertex(0u)
                , uBaseIndex(0u)
                , uMaterialIndex(INVALID_MATERIAL)
            {
            }

            UINT uNumIndices;
            UINT uBaseVertex;
            UINT uBaseIndex;
            UINT uMaterialIndex;
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
        UINT m_srvDescriptorSize = 0;

        // App resources.
        ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
        ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        ComPtr<ID3D12Resource> m_depthBuffer;
        ComPtr<ID3D12Resource> m_texture;
        ComPtr<ID3D12Resource> m_heightmap;

        // Synchronization objects.
        UINT m_frameIndex = 0;
        HANDLE m_fenceEvent;
        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;

        ConstantBuffer m_constantBuffer;

        std::vector<Vertex> m_aVertices;
        std::vector<WORD> m_aIndices;

        const aiScene* m_pScene;

        std::vector<BasicMeshEntry> m_aMeshes;
	};
}