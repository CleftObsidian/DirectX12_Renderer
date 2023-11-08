#include "Game.h"

namespace DX12Library
{
    HINSTANCE g_hInst = nullptr;
    HWND g_hWnd = nullptr;

    // Pipeline objects.
    ComPtr<IDXGISwapChain3> g_swapChain;
    ComPtr<ID3D12Device> g_device;
    ComPtr<ID3D12Resource> g_renderTargets[2];
    ComPtr<ID3D12CommandAllocator> g_commandAllocator;
    ComPtr<ID3D12CommandQueue> g_commandQueue;
    ComPtr<ID3D12DescriptorHeap> g_rtvHeap;
    ComPtr<ID3D12PipelineState> g_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> g_commandList;
    UINT g_rtvDescriptorSize = 0;

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
                L"DX12 Renderer",
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
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        g_hWnd = CreateWindow(L"DX12Renderer", L"DX12 : Window",
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
            .SampleDesc = {
                .Count = 1},
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
        // Create the command list.
        ThrowIfFailed(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_commandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        ThrowIfFailed(g_commandList->Close());

        // Create synchronization objects.
        {
            ThrowIfFailed(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)));
            g_fenceValue = 1;

            // Create an event handle to use for frame synchronization.
            g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (g_fenceEvent == nullptr)
            {
                ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }
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

        // Indicate that the back buffer will be used as a render target.
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(g_renderTargets[g_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        g_commandList->ResourceBarrier(1, &barrier);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(), g_frameIndex, g_rtvDescriptorSize);

        // Record commands.
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

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