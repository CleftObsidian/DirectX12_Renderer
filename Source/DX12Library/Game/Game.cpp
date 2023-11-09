#include "Game.h"

namespace DX12Library
{
    HINSTANCE g_hInst = nullptr;
    HWND g_hWnd = nullptr;

    struct Vertex
    {
        XMFLOAT3 position;
        // XMFLOAT4 color;
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT g_viewport;
    CD3DX12_RECT g_scissorRect;
    ComPtr<IDXGISwapChain3> g_swapChain;
    ComPtr<ID3D12Device> g_device;
    ComPtr<ID3D12Resource> g_renderTargets[2];
    ComPtr<ID3D12CommandAllocator> g_commandAllocator;
    ComPtr<ID3D12CommandQueue> g_commandQueue;
    ComPtr<ID3D12RootSignature> g_rootSignature;
    ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
    ComPtr<ID3D12PipelineState> g_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> g_commandList;
    UINT g_rtvDescriptorSize = 0;

    // App resources.
    ComPtr<ID3D12Resource> g_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView;
    UINT8* g_pCbvDataBegin;

    // Synchronization objects.
    UINT g_frameIndex = 0;
    HANDLE g_fenceEvent;
    ComPtr<ID3D12Fence> g_fence;
    UINT64 g_fenceValue;

    LRESULT CALLBACK WindowProc(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
    {
        PAINTSTRUCT ps;
        HDC hdc;

        switch (uMsg)
        {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_CLOSE:
            if (MessageBox(hWnd,
                L"Really quit?",
                PSZ_TITLE,
                MB_OKCANCEL) == IDOK)
            {
                DestroyWindow(hWnd);
            }
            // Else: user canceled. Do nothing.
            return S_OK;

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }

        return S_OK;
    }

    HRESULT InitWindow(_In_ HINSTANCE hInstance, _In_ INT nCmdShow)
    {
        // Register class
        WNDCLASSEX wcex =
        {
            .cbSize = static_cast<UINT>(sizeof(WNDCLASSEX)),
            .style = static_cast<UINT> (CS_HREDRAW | CS_VREDRAW),
            .lpfnWndProc = WindowProc,
            .cbClsExtra = 0,
            .hInstance = hInstance,
            .hIcon = LoadIcon(hInstance, reinterpret_cast<LPCTSTR>(IDI_TUTORIAL1)),
            .hCursor = LoadCursor(nullptr, IDC_ARROW),
            .hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
            .lpszMenuName = nullptr,
            .lpszClassName = L"DX12Renderer",
            .hIconSm = LoadIcon(wcex.hInstance, reinterpret_cast<LPCTSTR>(IDI_TUTORIAL1))
        };

        if (!RegisterClassEx(&wcex))
        {
            DWORD dwError = GetLastError();

            MessageBox(
                nullptr,
                L"Call to RegisterClassEx failed!",
                PSZ_TITLE,
                NULL
            );

            if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            {
                return HRESULT_FROM_WIN32(dwError);
            }

            return E_FAIL;
        }

        // Create window
        g_hInst = hInstance;
        RECT rc = { 0, 0, 1280, 720 };
        g_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 1280.0f, 720.0f);
        g_scissorRect = CD3DX12_RECT(0, 0, 1280, 720);
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        g_hWnd = CreateWindow(L"DX12Renderer", PSZ_TITLE,
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
            nullptr);
        if (!g_hWnd)
        {
            DWORD dwError = GetLastError();

            MessageBox(
                nullptr,
                L"Call to CreateWindowEx failed!",
                PSZ_TITLE,
                NULL
            );

            if (dwError != ERROR_CLASS_ALREADY_EXISTS)
            {
                return HRESULT_FROM_WIN32(dwError);
            }

            return E_FAIL;
        }

        ShowWindow(g_hWnd, nCmdShow);

        return S_OK;
    }

    void InitDevice()
    {
        // Load the rendering pipeline dependencies.
        UINT dxgiFactoryFlags = 0;

#ifdef _DEBUG
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();

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
            IID_PPV_ARGS(&g_device)
            ));

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc =
        {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
        };

        ThrowIfFailed(g_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue)));

        // Describe and create the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc =
        {
            .Width = 1280,
            .Height = 720,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { .Count = 1 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2, // frame count
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD
        };

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            g_commandQueue.Get(),   // Swap chain needs the queue so that it can force a flush on it.
            g_hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        ));

        // This sample does not support fullscreen transitions.
        ThrowIfFailed(factory->MakeWindowAssociation(g_hWnd, DXGI_MWA_NO_ALT_ENTER));

        ThrowIfFailed(swapChain.As(&g_swapChain));
        g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

        // Create descriptor heaps.
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc =
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                .NumDescriptors = 2,
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE
            };
            ThrowIfFailed(g_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvHeap)));

            g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // Create frame resources.
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create a RTV for each frame.
            for (UINT n = 0; n < 2; n++)
            {
                ThrowIfFailed(g_swapChain->GetBuffer(n, IID_PPV_ARGS(&g_renderTargets[n])));
                g_device->CreateRenderTargetView(g_renderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, g_rtvDescriptorSize);
            }
        }

        ThrowIfFailed(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocator)));

        // Load the sample assets.
        // Create a root signature consisting of a descriptor table with a single CBV.
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

            // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

            if (FAILED(g_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
            {
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            //CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            CD3DX12_ROOT_PARAMETER1 rootParameters[1];

            //ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            //rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
            rootParameters[0].InitAsConstants(4, 0);

            // Allow input layout and deny uneccessary access to certain pipeline stages.
            D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
                //D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
            
            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
            ThrowIfFailed(g_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature)));
        }

        // Create the pipeline state, which includes compiling and loading shaders.
        {
            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT compileFlags = 0;
#endif

            ThrowIfFailed(D3DCompileFromFile(L"Shaders/shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, nullptr));
            ThrowIfFailed(D3DCompileFromFile(L"Shaders/shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, nullptr));

            // Define the vertex input layout.
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                //{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc =
            {
                .pRootSignature = g_rootSignature.Get(),
                .VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
                .PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get()),
                .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
                .SampleMask = UINT_MAX,
                .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
                .DepthStencilState = { .DepthEnable = FALSE,
                                       .StencilEnable = FALSE},
                .InputLayout = { inputElementDescs, _countof(inputElementDescs) },
                .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                .NumRenderTargets = 1,
                .RTVFormats = { DXGI_FORMAT_R8G8B8A8_UNORM, },
                .SampleDesc = { .Count = 1 }
            };

            ThrowIfFailed(g_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState)));
        }

        // Create the command list.
        ThrowIfFailed(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_commandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        ThrowIfFailed(g_commandList->Close());

        // Create the vertex buffer.
        {
            // Define the geometry for a quad.
            Vertex quadVertices[] =
            {
                { .position = { 1.0f, 3.f, 0.0f }, },
                { .position = { 1.f, -1.0f, 0.0f }, },
                { .position = { -3.f, -1.f, 0.0f }, },
            };

            const UINT vertexBufferSize = sizeof(quadVertices);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
            ThrowIfFailed(g_device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&g_vertexBuffer)));

            // Copy the quad data to the vertex buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, quadVertices, sizeof(quadVertices));
            g_vertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
            g_vertexBufferView.StrideInBytes = sizeof(Vertex);
            g_vertexBufferView.SizeInBytes = vertexBufferSize;
        }

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            ThrowIfFailed(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));
            g_fenceValue = 1;

            // Create an event handle to use for frame synchronization.
            g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (g_fenceEvent == nullptr)
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
            const UINT64 fence = g_fenceValue;
            ThrowIfFailed(g_commandQueue->Signal(g_fence.Get(), fence));
            g_fenceValue++;

            // Wait until the previous frame is finished.
            if (g_fence->GetCompletedValue() < fence)
            {
                ThrowIfFailed(g_fence->SetEventOnCompletion(fence, g_fenceEvent));
                WaitForSingleObject(g_fenceEvent, INFINITE);
            }

            g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
        }
    }

    void CleanupDevice()
    {
        // Ensure that the GPU is no longer referencing resources that are about to be
        // cleaned up by the destructor.

        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
        // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
        // sample illustrates how to use fences for efficient resource usage and to
        // maximize GPU utilization.

        // Signal and increment the fence value.
        const UINT64 fence = g_fenceValue;
        ThrowIfFailed(g_commandQueue->Signal(g_fence.Get(), fence));
        ++g_fenceValue;

        // Wait until the previous frame is finished.
        if (g_fence->GetCompletedValue() < fence)
        {
            ThrowIfFailed(g_fence->SetEventOnCompletion(fence, g_fenceEvent));
            WaitForSingleObject(g_fenceEvent, INFINITE);
        }

        g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

        CloseHandle(g_fenceEvent);
    }

    void Render()
    {
        // Record all the commands we need to render the scene into the command list.
        // Command list allocators can only be reset when the associated 
        // command lists have finished execution on the GPU; apps should use 
        // fences to determine GPU execution progress.
        ThrowIfFailed(g_commandAllocator->Reset());

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ThrowIfFailed(g_commandList->Reset(g_commandAllocator.Get(), g_pipelineState.Get()));

        // Set necessary state.
        g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());
        struct ConstantColor
        {
            FLOAT r;
            FLOAT g;
            FLOAT b;
            FLOAT a;
        };
        ConstantColor cColor =
        {
            .r = 1.0f,
            .g = 0.5f,
            .b = 0.0f,
            .a = 1.0f
        };
        g_commandList->SetGraphicsRoot32BitConstants(0, 4, &cColor, 0);

        g_commandList->RSSetViewports(1, &g_viewport);
        g_commandList->RSSetScissorRects(1, &g_scissorRect);

        // Indicate that the back buffer will be used as a render target.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_renderTargets[g_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        g_commandList->ResourceBarrier(1, &barrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), g_frameIndex, g_rtvDescriptorSize);
        g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
        g_commandList->DrawInstanced(3, 1, 0, 0);

        // Indicate that the back buffer will now be used to present.
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_renderTargets[g_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_commandList->ResourceBarrier(1, &barrier);

        ThrowIfFailed(g_commandList->Close());

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { g_commandList.Get() };
        g_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        ThrowIfFailed(g_swapChain->Present(1, 0));

        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
        // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
        // sample illustrates how to use fences for efficient resource usage and to
        // maximize GPU utilization.

        // Signal and increment the fence value.
        const UINT64 fence = g_fenceValue;
        ThrowIfFailed(g_commandQueue->Signal(g_fence.Get(), fence));
        ++g_fenceValue;

        // Wait until the previous frame is finished.
        if (g_fence->GetCompletedValue() < fence)
        {
            ThrowIfFailed(g_fence->SetEventOnCompletion(fence, g_fenceEvent));
            WaitForSingleObject(g_fenceEvent, INFINITE);
        }

        g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
    }
}