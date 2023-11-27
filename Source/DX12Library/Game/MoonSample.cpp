#include "MoonSample.h"
#include <ResourceUploadBatch.h>
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

#include "assimp/Importer.hpp"	// C++ importer interface
#include "assimp/scene.h"		// output data structure
#include "assimp/postprocess.h"	// post processing flags

namespace DX12Library
{
	std::unique_ptr<Assimp::Importer> MoonSample::sm_pImporter = std::make_unique<Assimp::Importer>();

	MoonSample::MoonSample(_In_ PCWSTR pszGameName)
		: GameSample(pszGameName)
		, m_fenceEvent()
		, m_fenceValue()
		, m_frameIndex(0)
		, m_rtvDescriptorSize(0)
		, m_vertexBufferView()
		, m_indexBufferView()
		, m_constantBuffer()
		, m_pScene(nullptr)
	{
	}

	MoonSample::~MoonSample()
	{
	}

	void MoonSample::InitDevice()
	{
        RECT rc;
        GetClientRect(m_mainWindow->GetWindow(), &rc);
        UINT uWidth = static_cast<UINT>(rc.right - rc.left);
        UINT uHeight = static_cast<UINT>(rc.bottom - rc.top);

        {
            m_viewport.TopLeftX = 0.0f;
            m_viewport.TopLeftY = 0.0f;
            m_viewport.Width = static_cast<FLOAT>(uWidth);
            m_viewport.Height = static_cast<FLOAT>(uHeight);

            m_scissorRect.left = 0;
            m_scissorRect.top = 0;
            m_scissorRect.right = static_cast<LONG>(uWidth);
            m_scissorRect.bottom = static_cast<LONG>(uHeight);
        }

        // Load the rendering pipeline dependencies.
        UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug5> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                debugController->SetEnableAutoName(TRUE);

                // Enable additional debug layers.
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif

        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter;
        ComPtr<IDXGIFactory6> factory6;
        if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory6))))
        {
            for (UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    DXGI_GPU_PREFERENCE_UNSPECIFIED,
                    IID_PPV_ARGS(&hardwareAdapter)));
                ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                hardwareAdapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    // If you want a software adapter, pass in "/warp" on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        if (hardwareAdapter.Get() == nullptr)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(factory->EnumAdapters1(adapterIndex, &hardwareAdapter)); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                hardwareAdapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    // If you want a software adapter, pass in "/warp" on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc =
        {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
        };

        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

        // Describe and create the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
        {
            .Width = uWidth,
            .Height = uHeight,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = {.Count = 1 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = FRAMECOUNT,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
        };

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
            m_mainWindow->GetWindow(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        ));

        // This sample does not support fullscreen transitions.
        ThrowIfFailed(factory->MakeWindowAssociation(m_mainWindow->GetWindow(), DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain.As(&m_swapChain));
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Create descriptor heaps.
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc =
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = FRAMECOUNT,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
            };
            ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

            m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // Describe and create a shader resource view (SRV) heap for the texture.
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc =
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 2,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
            };
            ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

            m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            // Describe and create a depth stencil view (DSV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc =
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
                .NumDescriptors = 1,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
            };
            ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
        }

        // Create frame resources.
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < FRAMECOUNT; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_rtvDescriptorSize);
            }
        }

        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

        // Load the sample assets.
        // Create a root signature.
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData =
            {
                // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
                .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1
            };

            if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
            {
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
            CD3DX12_ROOT_PARAMETER1 rootParameters[3];

            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

            rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
            rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_DOMAIN);
            rootParameters[2].InitAsConstants(sizeof(ConstantBuffer) / 4, 0, 0, D3D12_SHADER_VISIBILITY_ALL);

            CD3DX12_STATIC_SAMPLER_DESC samplers[2];

            samplers[0].Init(0, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT);
            samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
            samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
            samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, _countof(samplers), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
            ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        }

        // Create the pipeline state, which includes compiling and loading shaders.
        {
            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> pixelShader;
            //ComPtr<ID3DBlob> domainShader;
            //ComPtr<ID3DBlob> hullShader;

#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT compileFlags = 0;
#endif
            
            ThrowIfFailed(D3DCompileFromFile(L"Shaders/shadersDirectionalLight.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));
            ThrowIfFailed(D3DCompileFromFile(L"Shaders/shadersDirectionalLight.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, nullptr));
            //ThrowIfFailed(D3DCompileFromFile(L"Shaders/shaderMoonDS.hlsl", nullptr, nullptr, "DSMain", "ds_5_1", compileFlags, 0, &domainShader, nullptr));
            //ThrowIfFailed(D3DCompileFromFile(L"Shaders/shaderMoonHS.hlsl", nullptr, nullptr, "HSMain", "hs_5_1", compileFlags, 0, &hullShader, nullptr));

            // Define the vertex input layout.
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc =
            {
                .pRootSignature = m_rootSignature.Get(),
                .VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
                .PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get()),
                //.DS = CD3DX12_SHADER_BYTECODE(domainShader.Get()),
                //.HS = CD3DX12_SHADER_BYTECODE(hullShader.Get()),
                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
                .SampleMask = UINT_MAX,
                .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
                .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
                .InputLayout = { .pInputElementDescs = inputElementDescs,
                                 .NumElements = _countof(inputElementDescs) },
                //.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH,
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM, },
                .DSVFormat = DXGI_FORMAT_D32_FLOAT,
                .SampleDesc = { .Count = 1 }
            };

            ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
        }

        // Create the command list.
        ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

        // Create the vertex and index buffer.
        {
            // Create the vertex buffer.
            // Read the 3D model file
            m_pScene = sm_pImporter->ReadFile(
                "Contents/Sphere/sphere.obj",
                ASSIMP_LOAD_FLAGS
            );

            // Application is now responsible of deleting this scene
            m_pScene = sm_pImporter->GetOrphanedScene();

            // Initialize the model
            if (m_pScene)
            {
                m_aMeshes.resize(m_pScene->mNumMeshes);

                UINT uNumVertices = 0u;
                UINT uNumIndices = 0u;
                for (UINT i = 0u; i < m_pScene->mNumMeshes; ++i)
                {
                    m_aMeshes[i].uNumIndices = m_pScene->mMeshes[i]->mNumFaces * 3u;
                    m_aMeshes[i].uBaseVertex = uNumVertices;
                    m_aMeshes[i].uBaseIndex = uNumIndices;
                    m_aMeshes[i].uMaterialIndex = m_pScene->mMeshes[i]->mMaterialIndex;

                    uNumVertices += m_pScene->mMeshes[i]->mNumVertices;
                    uNumIndices += m_aMeshes[i].uNumIndices;
                }

                m_aVertices.reserve(uNumVertices);
                m_aIndices.reserve(uNumIndices);

                for (UINT i = 0u; i < m_aMeshes.size(); ++i)
                {
                    const aiMesh* pMesh = m_pScene->mMeshes[i];
                    const aiVector3D zero3d(0.0f, 0.0f, 0.0f);

                    // Populate the vertex attribute vector
                    for (UINT j = 0u; j < pMesh->mNumVertices; ++j)
                    {
                        const aiVector3D& position = pMesh->mVertices[j];
                        const aiVector3D& normal = pMesh->mNormals[j];
                        const aiVector3D& texCoord = pMesh->HasTextureCoords(0u) ? pMesh->mTextureCoords[0][j] : zero3d;

                        Vertex vertex =
                        {
                            .Position = XMFLOAT3(position.x, position.y, position.z),
                            .TexCoord = XMFLOAT2(texCoord.x, texCoord.y),
                            .Normal = XMFLOAT3(normal.x, normal.y, normal.z),
                        };

                        m_aVertices.push_back(vertex);
                    }

                    // Populate the index buffer
                    for (UINT j = 0u; j < pMesh->mNumFaces; ++j)
                    {
                        const aiFace& face = pMesh->mFaces[j];
                        assert(face.mNumIndices == 3u);

                        WORD aIndices[3] =
                        {
                            static_cast<WORD>(face.mIndices[0]),
                            static_cast<WORD>(face.mIndices[1]),
                            static_cast<WORD>(face.mIndices[2]),
                        };

                        m_aIndices.push_back(aIndices[0]);
                        m_aIndices.push_back(aIndices[1]);
                        m_aIndices.push_back(aIndices[2]);
                    }
                }
            }
            else
            {
                OutputDebugString(L"Error parsing ");
                OutputDebugString(L"Contents/Sphere/sphere.obj");
                OutputDebugString(L": ");
                OutputDebugStringA(sm_pImporter->GetErrorString());
                OutputDebugString(L"\n");
            }

            const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * m_aVertices.size());
            {
                CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
                CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
                // create default heap
                // default heap is memory on the GPU. Only the GPU has access to this memory
                // To get data into this heap, we will have to upload the data using
                // an upload heap
                ThrowIfFailed(m_device->CreateCommittedResource(
                    &heapProperties, // a default heap
                    D3D12_HEAP_FLAG_NONE, // no flags
                    &resourceDesc, // resource description for a buffer
                    D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
                    nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
                    IID_PPV_ARGS(&m_vertexBuffer)));
                m_vertexBuffer->SetName(L"VertexBuffer");
            }

            {
                CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
                CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
                // create upload heap
                // upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
                // We will upload the vertex buffer using this heap to the default heap
                ID3D12Resource* vBufferUploadHeap;
                ThrowIfFailed(m_device->CreateCommittedResource(
                    &heapProperties, // upload heap
                    D3D12_HEAP_FLAG_NONE, // no flags
                    &resourceDesc, // resource description for a buffer
                    D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
                    nullptr,
                    IID_PPV_ARGS(&vBufferUploadHeap)));

                // store vertex buffer in upload heap
                D3D12_SUBRESOURCE_DATA vertexData = {};
                vertexData.pData = m_aVertices.data();
                vertexData.RowPitch = vertexBufferSize;
                vertexData.SlicePitch = vertexBufferSize;

                // we are now creating a command with the command list to copy the data from
                // the upload heap to the default heap
                UpdateSubresources(m_commandList.Get(), m_vertexBuffer.Get(), vBufferUploadHeap, 0, 0, 1, &vertexData);

                // transition the vertex buffer data from copy destination state to vertex buffer state
                CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                m_commandList->ResourceBarrier(1, &barrier);
            }

            // Initialize the vertex buffer view.
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = vertexBufferSize;

            UINT indexBufferSize = static_cast<UINT>(sizeof(WORD) * m_aIndices.size());
            // Create the index buffer.
            {
                CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
                CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
                // create default heap to hold index buffer
                ThrowIfFailed(m_device->CreateCommittedResource(
                    &heapProperties, // a default heap
                    D3D12_HEAP_FLAG_NONE, // no flags
                    &resourceDesc, // resource description for a buffer
                    D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
                    nullptr, // optimized clear value must be null for this type of resource
                    IID_PPV_ARGS(&m_indexBuffer)));
                m_indexBuffer->SetName(L"IndexBuffer");
            }

            // create upload heap to upload index buffer
            {
                CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
                CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
                ID3D12Resource* iBufferUploadHeap;
                ThrowIfFailed(m_device->CreateCommittedResource(
                    &heapProperties, // upload heap
                    D3D12_HEAP_FLAG_NONE, // no flags
                    &resourceDesc, // resource description for a buffer
                    D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
                    nullptr,
                    IID_PPV_ARGS(&iBufferUploadHeap)));

                // store vertex buffer in upload heap
                D3D12_SUBRESOURCE_DATA indexData = {};
                indexData.pData = m_aIndices.data(); // pointer to our index array
                indexData.RowPitch = indexBufferSize; // size of all our index buffer
                indexData.SlicePitch = indexBufferSize; // also the size of our index buffer

                // we are now creating a command with the command list to copy the data from
                // the upload heap to the default heap
                UpdateSubresources(m_commandList.Get(), m_indexBuffer.Get(), iBufferUploadHeap, 0, 0, 1, &indexData);

                // transition the vertex buffer data from copy destination state to vertex buffer state
                CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                m_commandList->ResourceBarrier(1, &barrier);
            }

            m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
            m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
            m_indexBufferView.SizeInBytes = indexBufferSize;
        }

        // Create the texture.
        {
            ResourceUploadBatch resourceUpload(m_device.Get());

            resourceUpload.Begin();

            ThrowIfFailed(CreateWICTextureFromFile(m_device.Get(), resourceUpload, L"Contents/Texture/lroc_color_poles.tif", m_texture.ReleaseAndGetAddressOf()));

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
            {
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Texture2D = {.MipLevels = 1 }
            };
            m_device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
            m_texture->SetName(L"Texture");

            auto uploadResourcesFinished = resourceUpload.End(m_commandQueue.Get());

            uploadResourcesFinished.wait();
        }

        // Create the displacement map.
        {
            ResourceUploadBatch resourceUpload(m_device.Get());

            resourceUpload.Begin();

            ThrowIfFailed(CreateWICTextureFromFile(m_device.Get(), resourceUpload, L"Contents/Texture/ldem_64.tif", m_heightmap.ReleaseAndGetAddressOf()));

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
            {
                .Format = DXGI_FORMAT_R32_FLOAT,
                .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                .Texture2D = {.MipLevels = 1 }
            };
            D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_srvDescriptorSize);
            m_device->CreateShaderResourceView(m_heightmap.Get(), &srvDesc, handleCPU);
            m_heightmap->SetName(L"Displacement");

            auto uploadResourcesFinished = resourceUpload.End(m_commandQueue.Get());

            uploadResourcesFinished.wait();
        }

        // Create a depth buffer.
        {
            D3D12_CLEAR_VALUE optimizedClearValue =
            {
                .Format = DXGI_FORMAT_D32_FLOAT,
                .DepthStencil = { 1.0f, 0 }
            };

            CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
            CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, uWidth, uHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            ThrowIfFailed(m_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &optimizedClearValue,
                IID_PPV_ARGS(&m_depthBuffer)));
            m_depthBuffer->SetName(L"DepthBuffer");

            // Create a depth-stencil view
            {
                D3D12_DEPTH_STENCIL_VIEW_DESC dsv =
                {
                    .Format = DXGI_FORMAT_D32_FLOAT,
                    .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
                    .Flags = D3D12_DSV_FLAG_NONE,
                    .Texture2D = {.MipSlice = 0 }
                };
                m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
            }
        }

        // Close the command list and execute it to begin the initial GPU setup.
        ThrowIfFailed(m_commandList->Close());
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
            m_fenceValue = 1;

            // Create an event handle to use for frame synchronization.
            m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_fenceEvent == nullptr)
            {
                ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }

            // Wait for the command list to execute; we are reusing the same command 
            // list in our main loop but for now, we just want to wait for setup to 
            // complete before continuing.
            // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
            // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
            // sample illustrates how to use fences for efficient resource usage and to
            // maximize GPU utilization.

            // Signal and increment the fence value.
            const UINT64 fence = m_fenceValue;
            ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
            m_fenceValue++;

            // Wait until the previous frame is finished.
            if (m_fence->GetCompletedValue() < fence)
            {
                ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
                WaitForSingleObject(m_fenceEvent, INFINITE);
            }

            m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        }
	}

	void MoonSample::CleanupDevice()
	{
		// Ensure that the GPU is no longer referencing resources that are about to be
		// cleaned up by the destructor.

		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
		// sample illustrates how to use fences for efficient resource usage and to
		// maximize GPU utilization.

		// Signal and increment the fence value.
		const UINT64 fence = m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
		++m_fenceValue;

		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		CloseHandle(m_fenceEvent);
	}

	void MoonSample::Update(FLOAT deltaTime)
	{
        UNREFERENCED_PARAMETER(deltaTime);
        // Update the model matrix.
        //m_constantBuffer.World *= XMMatrixScaling(1.001f, 1.001f, 1.001f);

		// Update the view matrix.
		const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
		const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
		const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
		m_constantBuffer.View = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
		XMStoreFloat3(&m_constantBuffer.CameraPos, eyePosition);

		RECT rc;
		GetClientRect(m_mainWindow->GetWindow(), &rc);
		FLOAT uWidth = static_cast<FLOAT>(rc.right - rc.left);
		FLOAT uHeight = static_cast<FLOAT>(rc.bottom - rc.top);
		// Update the projection matrix.
		float aspectRatio = uWidth / uHeight;
		m_constantBuffer.Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);
	}

	void MoonSample::Render()
	{
        // Record all the commands we need to render the scene into the command list.
        // Command list allocators can only be reset when the associated 
        // command lists have finished execution on the GPU; apps should use 
        // fences to determine GPU execution progress.
        ThrowIfFailed(m_commandAllocator->Reset());

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

        // Set necessary state.
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
        m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_srvDescriptorSize);
        m_commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

        // Update the transform matrix
        m_commandList->SetGraphicsRoot32BitConstants(2, sizeof(ConstantBuffer) / 4, &m_constantBuffer, 0);

        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);

        // Indicate that the back buffer will be used as a render target.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &barrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // Record commands.
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        //m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_indexBufferView);
        m_commandList->DrawIndexedInstanced(static_cast<UINT>(m_aIndices.size()), 1, 0, 0, 0);

        // Indicate that the back buffer will now be used to present.
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(m_commandList->Close());

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        ThrowIfFailed(m_swapChain->Present(1, 0));

        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
        // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
        // sample illustrates how to use fences for efficient resource usage and to
        // maximize GPU utilization.

        // Signal and increment the fence value.
        const UINT64 fence = m_fenceValue;
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
        ++m_fenceValue;

        // Wait until the previous frame is finished.
        if (m_fence->GetCompletedValue() < fence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}
}