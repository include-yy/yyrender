#include "stdafx.h"
#include "Helper.h"
#include "MyRender.h"
#include "W32.h"
#include "StepTimer.h"
#include "SimpleCamera.h"
#include "Resource.h"

MyRender::MyRender(UINT width, UINT height, std::wstring name, std::string initobj) :
    Framework(width, height, name),
    m_useMSAA(false),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_objpath(initobj)
{
}

void MyRender::OnInit()
{
    LoadPipeline();
    LoadAssetsOnce();
    // If resources load failed at initialziation time. Just pass the
    // exception to Main's top-level exception catch.
    // This means yy-render will show reasons using MessageBox and exit.
    LoadAssets();
}

void MyRender::OnUpdate()
{
    m_timer.Tick(NULL);

    if (m_frameCounter == 500)
    {
        wchar_t fps[64];
        swprintf_s(fps, L"%ufps", m_timer.GetFramesPerSecond());
        SetCustomWindowText(fps);
        m_frameCounter = 0;
    }
    m_frameCounter++;
    m_camera.Update(static_cast<float>(m_timer.GetElapsedSeconds()));
    // get MVP matrix
    XMFLOAT4X4 mvp{}, light_mvp{};
    XMMATRIX model = XMLoadFloat4x4(&m_model);
    XMFLOAT3 eyepos = m_camera.GetEyePos();
    XMMATRIX view = m_camera.GetViewMatrix();
    XMMATRIX projection = m_camera.GetProjectionMatrix(0.8f, m_aspectRatio);
    XMMATRIX lightview = m_camera.GetLightViewMatrix(m_aspectRatio);
    XMMATRIX lightproj = m_camera.GetLightProjMatrix();
    XMStoreFloat4x4(&mvp, XMMatrixTranspose(model * view * projection));
    XMStoreFloat4x4(&light_mvp, XMMatrixTranspose(model * lightview * lightproj));
    //XMStoreFloat4x4(&light_mvp, XMMatrixTranspose(model * lightview * projection));
    memcpy(&m_pConstBuffer->mvp, &mvp, sizeof(mvp));
    memcpy(&m_pConstBuffer->lighteye_mvp, &light_mvp, sizeof(light_mvp));
    XMFLOAT3 light = m_camera.GetLight();
    //int uselight = m_MenuOptions.use_light;
    int usetexture = m_MenuOptions.use_default_texture;
    memcpy(&m_pConstBuffer->light, &light, sizeof(XMFLOAT3));
    //memcpy(&m_pConstBuffer->uselight, &uselight, sizeof(UINT));
    memcpy(&m_pConstBuffer->useDefTex, &usetexture, sizeof(UINT));
    memcpy(&m_pConstBuffer->eyepos, &eyepos, sizeof(XMFLOAT3));
}

void MyRender::OnPaint()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void MyRender::OnDestroy()
{

}

void MyRender::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
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

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));
    }

    // Check MSAA support
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels{};
    msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    if (S_OK == m_device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)))
    {
        m_useMSAA = true;
    }
    else
    {
        m_useMSAA = false;
    }
    // m_useMSAA = false;
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        W32::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(W32::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount + m_useMSAA;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        // Describe and create a depth/stencil view (DSV) descriptor heap.
        // one for normal render, another for shadowmap
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 2;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

        // Describe and create a sampler descriptor heap.
        // one for normal texture, another for shadowmap texture
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = 2;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_samplerHeap)));

        // Describe and create a shader resource view (SRV) and constant
        // buffer view (CBV) descriptor heap.
        /* We'll create it based on total number of Material and texture */
        //D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
        //cbvSrvHeapDesc.NumDescriptors = 2;
        //cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        //cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        //cbvSrvHeapDesc.NodeMask = 0;
        //ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_smpDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
        // Create offScreenBuffer for MSAA if we can
        if (m_useMSAA)
        {
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 4, 0);
            bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            ThrowIfFailed(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                nullptr,
                IID_PPV_ARGS(&m_offScrrenBuffer)
            ));
            m_device->CreateRenderTargetView(m_offScrrenBuffer.Get(), nullptr, rtvHandle);
        }
    }
    // Create Coomand Allocator
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
}

void MyRender::LoadAssetsOnce()
{
    // Create the root signature.
    CreateRootSignature();
    // Create the pipeline state, which includes compiling and loading shaders.
    CreatePipelineState(YYMID_SHADER_NORMAL_COLOR, L"normal_color.hlsl", L"normal_color.hlsl");
    CreatePipelineState(YYMID_SHADER_MAPKD_COLOR, L"mapKd_color.hlsl", L"mapKd_color.hlsl");
    CreatePipelineState(YYMID_SHADER_KAKDKS, L"KaKdKs.hlsl", L"KaKdKs.hlsl");
    CreatePipelineState(YYMID_SHADER_KE_COLOR, L"Ke.hlsl", L"Ke.hlsl");
    CreatePipelineState(YYMID_SHADER_PHONG_SHADOWMAP, L"Final_ShaderMap.hlsl", L"Final_ShaderMap.hlsl");
    // PSO for shadermap
    CreateShadowMapPSO(L"Final_ShaderMap.hlsl");

    // Create the depth stencil view
    CreateDepthStencil();
    // Create Sampler
    CreateSampler();
    // Create synchronization objects.
    CreateFence();
    
    // MENU for yy-render
    {
        auto hMenu = CreateMenu();
        AppendMenuW(hMenu, MF_STRING, YYMID_OPEN, L"Load");
        // Shader selection popup
        auto hMenuShaders = CreateMenu();
        AppendMenuW(hMenuShaders, MF_STRING, YYMID_SHADER_NORMAL_COLOR, L"normal vector as color");
        AppendMenuW(hMenuShaders, MF_STRING, YYMID_SHADER_MAPKD_COLOR, L"Kd as color");
        AppendMenuW(hMenuShaders, MF_STRING, YYMID_SHADER_KE_COLOR, L"Ke as color");
        AppendMenuW(hMenuShaders, MF_STRING, YYMID_SHADER_KAKDKS, L"ambient, diffuse and specular");
        AppendMenuW(hMenuShaders, MF_STRING, YYMID_SHADER_PHONG_SHADOWMAP, L"phong with shadowmap");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hMenuShaders, L"Shaders");
        // Options
        auto hMenuOptions = CreateMenu();
        //AppendMenuW(hMenuOptions, MF_STRING, YYMID_OPTION_USE_LIGHT, L"use light");
        AppendMenuW(hMenuOptions, MF_STRING, YYMID_OPTION_USE_DEFAULT_TEXTURE, L"use default diffuse texture");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hMenuOptions, L"Options");
        // About
        AppendMenuW(hMenu, MF_STRING, YYMID_ABOUT, L"About");
        SetMenu(W32::GetHwnd(), hMenu);

        CheckMenuItem(hMenu, YYMID_OPTION_USE_DEFAULT_TEXTURE, MF_CHECKED);
        //CheckMenuItem(hMenu, YYMID_OPTION_USE_LIGHT, MF_CHECKED);

        // select default pipelinestate
        m_pipelineState = m_pipeStates[YYMID_SHADER_PHONG_SHADOWMAP].Get();
        CheckMenuItem(hMenu, YYMID_SHADER_PHONG_SHADOWMAP, MF_CHECKED);
        m_useShadowMap = true;
        
        //m_pipelineState = m_pipeStates[YYMID_SHADER_KAKDKS].Get();
        //CheckMenuItem(hMenu, YYMID_SHADER_KAKDKS, MF_CHECKED);
    }
}

void MyRender::LoadAssets()
{
    // OBJ WAVE data
    std::unique_ptr<yyobj::Mesh> pMesh{};
    std::vector<yyobj::Vertex> vertices{};
    std::vector<std::unique_ptr<BYTE[]>> texDatas{};
    std::vector<yyobj::Material> materials;
    std::vector<TextureInfo> tInfos{};
    // Upload Heaps
    ComPtr<ID3D12Resource> vertexBufferUploadHeap;
    ComPtr<ID3D12Resource> textureUploadHeap;
    // Load OBJ Data
    LoadObjData(pMesh, vertices, materials);
    // Load Textures
    LoadTexData(pMesh, tInfos);
    /***********************************************************************************************/
    /* If load resources(vertex and texture) fails, they'll signal an exceiption.                  */
    /* And the Resoucre update command below won't executing, thus reverse a normal previous model */
    /***********************************************************************************************/
    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_commandAllocator.Get(), m_pipelineState, IID_PPV_ARGS(&m_commandList)));
    // Create Vertex Buffer
    if (m_vertexBuffer != nullptr)
        m_vertexBuffer.Reset();
    // set vertex and materials
    CreateVertexBuffer(vertices.data(), vertices.size(), vertexBufferUploadHeap);
    m_materials = std::move(materials);
    // Adjust model matrix and camera
    AdjustCamera(pMesh.get());
    // Update cbvSrvheap
    UpdatecbvSrvHeap(static_cast<int>(2 + pMesh->materials.size() + pMesh->textures.size()));
    // Create the MVP matrix buffer
    if (m_cbvUploadHeap != nullptr)
    {
        m_cbvUploadHeap->Unmap(0, nullptr);
        m_cbvUploadHeap.Reset();
        m_pConstBuffer = nullptr;
    }
    CreateConstbuffer(pMesh.get());
    // Create the textures and sampler
    if (m_chanTextures.empty() == false)
    {
        m_chanTextures.clear();
    }
    CreateTexture(tInfos, textureUploadHeap);
    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
    // Wait for the command list to execute; we are reusing the same command
    // list in our main loop but for now, we just want to wait for setup to
    // complete before continuing.
    WaitForPreviousFrame();
}

void MyRender::UpdatecbvSrvHeap(UINT num)
{
    if (m_cbvSrvHeap != nullptr)
    {
        m_cbvSrvHeap.Reset();
    }

    D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
    cbvSrvHeapDesc.NumDescriptors = num;
    cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvSrvHeapDesc.NodeMask = 0;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

    m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void MyRender::CreateRootSignature()
{
    if (m_rootSignature != nullptr)
    {
        m_rootSignature.Reset();
    }
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 ranges[13] = {};
    // MVP + one material
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    // map_ka, map_kd, map_Ks, ... map_bump, total number is NINE
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[9].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[10].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 8, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    // one for sampler
    ranges[11].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 2, 0);
    // one for shadowmap depth texture
    ranges[12].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 9);
    // FIXME: UAV
    CD3DX12_ROOT_PARAMETER1 rootParameters[13] = {};
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[4].InitAsDescriptorTable(1, &ranges[4], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[5].InitAsDescriptorTable(1, &ranges[5], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[6].InitAsDescriptorTable(1, &ranges[6], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[7].InitAsDescriptorTable(1, &ranges[7], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[8].InitAsDescriptorTable(1, &ranges[8], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[9].InitAsDescriptorTable(1, &ranges[9], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[10].InitAsDescriptorTable(1, &ranges[10], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[11].InitAsDescriptorTable(1, &ranges[11], D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[12].InitAsDescriptorTable(1, &ranges[12], D3D12_SHADER_VISIBILITY_PIXEL);
    //rootParameters[3].InitAsConstants(1, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)));
}

void MyRender::CreatePipelineState(int id, LPCWSTR vs, LPCWSTR ps)
{
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> computeShader;
    ComPtr<ID3D12PipelineState> tempPS{};

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(vs).c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(ps).c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
    };

    // enable RGB-ALPHA
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    //blendDesc.RenderTarget[0].BlendEnable = true;
    //blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    //blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    //blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    //blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    //blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    //blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    //blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // enable multi-sample
    CD3DX12_RASTERIZER_DESC rasterDesc(D3D12_DEFAULT);
    rasterDesc.MultisampleEnable = m_useMSAA;
    rasterDesc.FrontCounterClockwise = TRUE;
    // rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = rasterDesc; //CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = blendDesc; //CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = m_useMSAA ? 4 : 1;
    psoDesc.SampleDesc.Quality = 0;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&tempPS)));

    m_pipeStates[id] = std::move(tempPS);
}

void MyRender::CreateShadowMapPSO(LPCWSTR vs)
{
    ComPtr<ID3DBlob> vShader;
    ComPtr<ID3D12PipelineState> ps;
#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(vs).c_str(), nullptr, nullptr, "VSShadowMap", "vs_5_0", compileFlags, 0, &vShader, nullptr));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA },
    };
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    CD3DX12_RASTERIZER_DESC rasterDesc(D3D12_DEFAULT);
    rasterDesc.FrontCounterClockwise = TRUE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vShader.Get());
    psoDesc.RasterizerState = rasterDesc; //CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = blendDesc; //CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    //psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_ShadowMapPSO)));
}

void MyRender::CreateConstbuffer(yyobj::Mesh* pMesh)
{
    // Create an upload heap for the constant buffers.
    const UINT matSize = 256 * (1 + pMesh->materials.size());
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(matSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_cbvUploadHeap)));

    MTLparameters* pMTLBuffer{};
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_cbvUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&pMTLBuffer)));
    for (int i = 0; i < pMesh->materials.size(); i++)
    {
        pMTLBuffer[i + 1].fallback = pMesh->materials[i].fallback;
        memcpy(pMTLBuffer[i + 1].Ka, pMesh->materials[i].Ka, 12);
        memcpy(pMTLBuffer[i + 1].Kd, pMesh->materials[i].Kd, 12);
        memcpy(pMTLBuffer[i + 1].Ks, pMesh->materials[i].Ks, 12);
        memcpy(pMTLBuffer[i + 1].Ke, pMesh->materials[i].Ke, 12);
        memcpy(pMTLBuffer[i + 1].Kt, pMesh->materials[i].Kt, 12);
        pMTLBuffer[i + 1].Ns = pMesh->materials[i].Ns;
        pMTLBuffer[i + 1].Ni = pMesh->materials[i].Ni;
        memcpy(pMTLBuffer[i + 1].Tf, pMesh->materials[i].Tf, 12);
        pMTLBuffer[i + 1].d = pMesh->materials[i].d;
        pMTLBuffer[i + 1].illum = pMesh->materials[i].illum;
        pMTLBuffer[i + 1].map_Ka = pMesh->materials[i].map_Ka;
        pMTLBuffer[i + 1].map_Kd = pMesh->materials[i].map_Kd;
        pMTLBuffer[i + 1].map_Ks = pMesh->materials[i].map_Ks;
        pMTLBuffer[i + 1].map_Ke = pMesh->materials[i].map_Ke;
        pMTLBuffer[i + 1].map_Kt = pMesh->materials[i].map_Kt;
        pMTLBuffer[i + 1].map_Ns = pMesh->materials[i].map_Ns;
        pMTLBuffer[i + 1].map_Ni = pMesh->materials[i].map_Ni;
        pMTLBuffer[i + 1].map_d = pMesh->materials[i].map_d;
        pMTLBuffer[i + 1].map_bump = pMesh->materials[i].map_bump;
    }
    m_cbvUploadHeap->Unmap(0, &readRange);

    // Map the constant buffers. Note that unlike D3D11, the resource
    // does not need to be unmapped for use by the GPU. In this sample,
    // the resource stays 'permenantly' mapped to avoid overhead with
    // mapping/unmapping each frame.
    //CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_cbvUploadHeap->Map(0, &readRange, reinterpret_cast<void**>(&m_pConstBuffer)));

    // Describe and create a constant buffer view (CBV).
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_cbvUploadHeap->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = 256;
        m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
    }
    // CBV for Materials
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
        cbvSrvHandle.Offset((2 + pMesh->textures.size()) * m_cbvSrvDescriptorSize);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.SizeInBytes = 256;
        for (int i = 0; i < pMesh->materials.size(); i++)
        {
            cbvDesc.BufferLocation = m_cbvUploadHeap->GetGPUVirtualAddress() + (size_t)(256 * (1 + i));
            m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
            cbvSrvHandle.Offset(m_cbvSrvDescriptorSize);
        }
    }
}

void MyRender::CreateVertexBuffer(yyobj::Vertex* pVertices, size_t numOfVertex, ComPtr<ID3D12Resource>& vertexBufferUploadHeap)
{
    const size_t vertexBufferSize = numOfVertex * sizeof(yyobj::Vertex);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)));

    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBufferUploadHeap)));

    // Copy data to the intermediate upload heap and then schedule a copy
    // from the upload heap to the vertex buffer.
    D3D12_SUBRESOURCE_DATA vertexData = {};
    vertexData.pData = pVertices;
    vertexData.RowPitch = vertexBufferSize;
    vertexData.SlicePitch = vertexData.RowPitch;

    UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer.Get(), vertexBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
    m_commandList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            m_vertexBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(yyobj::Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;
}

void MyRender::CreateTexture(std::vector<TextureInfo>& tInfos, ComPtr<ID3D12Resource>& textureUploadHeap)
{
    m_chanTextures.resize(tInfos.size());

    UINT64 uploadBufferSize{};
    std::vector<UINT> uploadSteps;
    for (size_t i = 0; i < tInfos.size(); i++)
    {
        auto& tDesc = tInfos[i].textureDesc;
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &tDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_chanTextures[i])));
        const UINT subresourceCount = tDesc.DepthOrArraySize * tDesc.MipLevels;
        const UINT uploadBufferStep = GetRequiredIntermediateSize(m_chanTextures[i].Get(), 0, subresourceCount);
        uploadSteps.push_back(uploadBufferStep);
        uploadBufferSize += uploadBufferStep;
    }
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));
    UINT offset = 0;
    for (size_t i = 0; i < tInfos.size(); i++)
    {
        D3D12_SUBRESOURCE_DATA textureData{};
        textureData.pData = tInfos[i].pData.get();
        textureData.RowPitch = static_cast<LONG_PTR>(tInfos[i].bytesPerRow);
        textureData.SlicePitch = tInfos[i].imgSize;
        UpdateSubresources(m_commandList.Get(), m_chanTextures[i].Get(),
            textureUploadHeap.Get(), offset, 0, 1, &textureData);
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_chanTextures[i].Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        offset += uploadSteps[i];
    }
    // Create SRVs for each texture
    // Position ZERO reserved for ConstBuffer that contains MVP and light
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
        m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_cbvSrvDescriptorSize);
    srvHandle.Offset(m_cbvSrvDescriptorSize * 2);
    for (int i = 0; i < tInfos.size(); i++)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc{};
        SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        SrvDesc.Format = tInfos[i].textureDesc.Format;
        SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SrvDesc.Texture2D.MipLevels = tInfos[i].textureDesc.MipLevels;
        m_device->CreateShaderResourceView(m_chanTextures[i].Get(), &SrvDesc, srvHandle);
        srvHandle.Offset(m_cbvSrvDescriptorSize);
    }
    // create shadowmap texture from shadowmap depth buffer
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle2(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
    D3D12_SHADER_RESOURCE_VIEW_DESC ShaDesc{};
    ShaDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ShaDesc.Format = DXGI_FORMAT_R32_FLOAT;
    ShaDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    ShaDesc.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(m_ShadowMapDepth.Get(), &ShaDesc, srvHandle2);
}

void MyRender::CreateDepthStencil()
// Create the depth stencil view.
{
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = m_useMSAA ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 1, m_useMSAA ? 4 : 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE),
        // Performance tip: Deny shader resource access to resources that don't need shader resource views.
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0), // Performance tip: Tell the runtime at resource creation the desired clear value.
        IID_PPV_ARGS(&m_depthStencil)
    ));

    NAME_D3D12_OBJECT(m_depthStencil);

    m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    // depth buffer for shadow map, don't use MSAA
    depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Texture2D.MipSlice = 0;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, m_width, m_height, 1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        // here we use GENERIC_READ
        D3D12_RESOURCE_STATE_GENERIC_READ,
        //D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &CD3DX12_CLEAR_VALUE(DXGI_FORMAT_D32_FLOAT, 1.0f, 0),
        IID_PPV_ARGS(&m_ShadowMapDepth)
    ));
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_dsvDescriptorSize);

    m_device->CreateDepthStencilView(m_ShadowMapDepth.Get(), &depthStencilDesc, dsvHandle);
}

void MyRender::WaitForPreviousFrame()
{
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

void MyRender::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated
    // command lists have finished execution on the GPU; apps should use
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command
    // list, that command list can then be reset at any time and must be before
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);
    ID3D12DescriptorHeap* descHeaps[] = { m_cbvSrvHeap.Get(), m_samplerHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descHeaps), descHeaps);
    // ZERO MVP
    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvSrvHeap.Get()->GetGPUDescriptorHandleForHeapStart());
    // ELEVEN SAMPLE
    m_commandList->SetGraphicsRootDescriptorTable(11, m_samplerHeap.Get()->GetGPUDescriptorHandleForHeapStart());
    // TWELVE shadowmap texture
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
    m_commandList->SetGraphicsRootDescriptorTable(12, cbvSrvHandle);
    cbvSrvHandle.Offset(m_cbvSrvDescriptorSize);
    // Set it per merged mesh
    //m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle);

    // Indicate that the back buffer will be used as a render target.
    if (!m_useMSAA)
    {
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_renderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET));
    }
    else
    {
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_offScrrenBuffer.Get(),
                D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));
    }
    if (m_useShadowMap)
    {
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_ShadowMapDepth.Get(),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                D3D12_RESOURCE_STATE_DEPTH_WRITE));
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvShadow(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), m_dsvDescriptorSize);
        m_commandList->ClearDepthStencilView(dsvShadow, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        //m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->SetPipelineState(m_ShadowMapPSO.Get());
        m_commandList->OMSetRenderTargets(0, nullptr, false, &dsvShadow);
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        for (auto& re : m_indicesRecoder)
        {
            m_commandList->DrawInstanced(re.count, 1, re.offset, 0);
        }
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_ShadowMapDepth.Get(),
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                D3D12_RESOURCE_STATE_GENERIC_READ));
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_useMSAA ? FrameCount : m_frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
    m_commandList->SetPipelineState(m_pipelineState);
    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    auto cptr = cbvSrvHandle.ptr;
    for (auto& re : m_indicesRecoder)
    {
        // use default material
        if (m_materials.size() == 1 && m_materials[0].fallback == 1)
        {
            cbvSrvHandle.ptr = cptr + m_chanTextures.size() * m_cbvSrvDescriptorSize;
            m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle); //mtl
            cbvSrvHandle.ptr = cptr;
            m_commandList->SetGraphicsRootDescriptorTable(3, cbvSrvHandle); //map_Kd
            m_commandList->DrawInstanced(re.count, 1, re.offset, 0);
            break;
        }
        else
        {
            size_t idx;
            cbvSrvHandle.ptr = cptr + (m_chanTextures.size() + re.mtl_index) * m_cbvSrvDescriptorSize;
            m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle);
            if (idx = m_materials[re.mtl_index].map_Ka)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHandle);
            }
            idx = m_materials[re.mtl_index].map_Kd;
            cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
            m_commandList->SetGraphicsRootDescriptorTable(3, cbvSrvHandle);
            if (idx = m_materials[re.mtl_index].map_Ks)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(4, cbvSrvHandle);
            }
            if (idx = m_materials[re.mtl_index].map_Ke)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(5, cbvSrvHandle);
            }
            if (idx = m_materials[re.mtl_index].map_Kt)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(6, cbvSrvHandle);
            }
            if (idx = m_materials[re.mtl_index].map_Ns)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(7, cbvSrvHandle);
            }
            if (idx = m_materials[re.mtl_index].map_Ni)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(8, cbvSrvHandle);
            }
            if (idx = m_materials[re.mtl_index].map_d)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(9, cbvSrvHandle);
            }
            if (idx = m_materials[re.mtl_index].map_bump)
            {
                cbvSrvHandle.ptr = cptr + idx * m_cbvSrvDescriptorSize;
                m_commandList->SetGraphicsRootDescriptorTable(10, cbvSrvHandle);
            }
        }
        m_commandList->DrawInstanced(re.count, 1, re.offset, 0);
        //cbvSrvHandle.Offset(m_cbvSrvDescriptorSize);
    }
    if (!m_useMSAA)
    {
        // Indicate that the back buffer will now be used to present.
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_renderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT));
    }
    else
    {
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_offScrrenBuffer.Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_RESOLVE_SOURCE));

        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_renderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RESOLVE_DEST));
        m_commandList->ResolveSubresource(
            m_renderTargets[m_frameIndex].Get(), 0,
            m_offScrrenBuffer.Get(), 0,
            DXGI_FORMAT_R8G8B8A8_UNORM);
        m_commandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_renderTargets[m_frameIndex].Get(),
                D3D12_RESOURCE_STATE_RESOLVE_DEST,
                D3D12_RESOURCE_STATE_PRESENT));
    }

    ThrowIfFailed(m_commandList->Close());
}

void MyRender::AdjustCamera(yyobj::Mesh* pMesh)
{
    float xmax{}, xmin{}, ymax{}, ymin{}, zmax{}, zmin{};
    xmax = ymax = zmax = -std::numeric_limits<float>::infinity();
    xmin = ymin = zmin = std::numeric_limits<float>::infinity();
    for (size_t i = 3; i < pMesh->positions.size(); i += 3)
    {
        xmin = std::min(pMesh->positions[i], xmin);
        xmax = std::max(pMesh->positions[i], xmax);
        ymin = std::min(pMesh->positions[i + 1], ymin);
        ymax = std::max(pMesh->positions[i + 1], ymax);
        zmin = std::min(pMesh->positions[i + 2], zmin);
        zmax = std::max(pMesh->positions[i + 2], zmax);
    }
    std::array<float, 3> center = { 0.0f, 0.0f, 0.0f };
    center[0] = (xmax + xmin) / 2.0f;
    center[1] = (ymax + ymin) / 2.0f;
    center[2] = (zmax + zmin) / 2.0f;
    m_model = {};
    m_model.m[0][0] = m_model.m[1][1] = m_model.m[2][2] = m_model.m[3][3] = 1.0f;
    m_model.m[3][0] = -center[0];
    m_model.m[3][1] = -center[1];
    m_model.m[3][2] = -center[2];
    float distancey = ymax - ymin;
    float distancex = xmax - xmin;
    float distancez = zmax - zmin;
    float distance = std::max(distancey, distancex);
    m_camera.Init({ 0.0f, 0.0f, distance / 0.8f + distancez / 2.0f}, { 0.0f, 0.0f, 1.0f }, {distancex / 2.0f, distancey / 2.0f, distancez / 2.0f});
}

void MyRender::CreateFence()
{
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    // Create an event handle to use for frame synchronization.
    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void MyRender::LoadObjData(std::unique_ptr<yyobj::Mesh>& pMesh,
    std::vector<yyobj::Vertex>& vertices,
    std::vector<yyobj::Material>& materials)
{
    auto result = yyobj::ParseObjFile(m_objpath);
    if (result.error)
    {
        std::string info = std::format("Error description: {}\nFile: {}\nLine {}:{}",
            result.error.code.message(), result.error.message, result.error.line_num, result.error.line);
        throw info;
    }
    pMesh = std::move(result.mesh);
    yyobj::Obj2Data(pMesh.get(), vertices, m_indicesRecoder);
    // default map_Kd map, get from https://casual-effects.com/data/ Cube
    pMesh->textures[0].path = GetAssetFullPathA("assets/mapKd_default.png");
    // default fallback material
    if (pMesh->materials.size() == 0)
    {
        pMesh->materials.push_back({});
        pMesh->materials[0].fallback = 1;
    }
    materials = pMesh->materials;
    return;
}

void MyRender::LoadTexData(std::unique_ptr<yyobj::Mesh>& pMesh, std::vector<TextureInfo>& tInfos)
{
    TextureInfo inf{};
    for (size_t i = 0; i < pMesh->textures.size(); i++)
    {
        inf = {};
        auto res = LoadImageDataFromFile(inf.pData, inf.textureDesc, pMesh->textures[i].path, inf.bytesPerRow, inf.imgSize);
        if (res != LoadTexError::Success)
        {
            throw std::string("Load texture fail");
        }
        tInfos.push_back(std::move(inf));
    }
}

void MyRender::CreateSampler()
{
    // Describe and create a sampler.
    D3D12_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    m_device->CreateSampler(&samplerDesc, m_samplerHeap->GetCPUDescriptorHandleForHeapStart());

    samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    //samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    CD3DX12_CPU_DESCRIPTOR_HANDLE smpHandle(m_samplerHeap->GetCPUDescriptorHandleForHeapStart(), m_smpDescriptorSize);
    m_device->CreateSampler(&samplerDesc, smpHandle);
}

void MyRender::ChangeModel(std::string path)
{
    m_objpath = path;
    try
    {
        LoadAssets();
    }
    catch (std::string& message)
    {
        RunWithModal([&message]() {
            MessageBoxA(W32::GetHwnd(), message.c_str(), "Load Failed", MB_OK);
        });
    }
}

void MyRender::OnCommand(WORD id)
{
    switch (id)
    {
    case YYMID_OPEN:
    {
        RunWithModal([this]() {
            OPENFILENAMEA ofn{};
            char szFile[1024] = {};
            // Initialize OPENFILENAME
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = W32::GetHwnd();
            ofn.lpstrFile = szFile;
            // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
            // use the contents of szFile to initialize itself.
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "ojbwave\0*.OBJ\0ALL\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = nullptr;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = nullptr;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (!GetOpenFileNameA(&ofn))
                return;
            std::string path = ofn.lpstrFile;
            std::string ext = path.substr(path.size() - 3);
            if (ext != "obj" && ext != "OBJ")
            {
                MessageBox(W32::GetHwnd(), L"File seems not objwave format", L"load failed", MB_OK);
                return;
            }
            ChangeModel(path);
        });
    }
    break;
    case YYMID_SHADER_MAPKD_COLOR:
    case YYMID_SHADER_NORMAL_COLOR:
    case YYMID_SHADER_KAKDKS:
    case YYMID_SHADER_KE_COLOR:
    case YYMID_SHADER_PHONG_SHADOWMAP:
    {
        auto hMenu = GetMenu(W32::GetHwnd());
        std::vector shaders{
            YYMID_SHADER_MAPKD_COLOR,
            YYMID_SHADER_NORMAL_COLOR,
            YYMID_SHADER_KAKDKS,
            YYMID_SHADER_KE_COLOR,
            YYMID_SHADER_PHONG_SHADOWMAP,
        };
        m_useShadowMap = false;
        for (auto& sid : shaders)
        {
            CheckMenuItem(hMenu, sid, MF_UNCHECKED);
        }
        CheckMenuItem(hMenu, id, MF_CHECKED);
        RunWithModal([this, &id]() {
            m_pipelineState = m_pipeStates[id].Get();
            if (id == YYMID_SHADER_PHONG_SHADOWMAP)
                m_useShadowMap = true;
        });
    }
    break;
    //case YYMID_OPTION_USE_LIGHT:
    //{
    //    auto hMenu = GetMenu(W32::GetHwnd());
    //    auto state = GetMenuState(hMenu, YYMID_OPTION_USE_LIGHT, 0);
    //    CheckMenuItem(hMenu, YYMID_OPTION_USE_LIGHT, state & MF_CHECKED ? MF_UNCHECKED : MF_CHECKED);
    //    m_MenuOptions.use_light = 1 - m_MenuOptions.use_light;
    //}
    //break;
    case YYMID_OPTION_USE_DEFAULT_TEXTURE:
    {
        auto hMenu = GetMenu(W32::GetHwnd());
        auto state = GetMenuState(hMenu, YYMID_OPTION_USE_DEFAULT_TEXTURE, 0);
        CheckMenuItem(hMenu, YYMID_OPTION_USE_DEFAULT_TEXTURE, state & MF_CHECKED ? MF_UNCHECKED : MF_CHECKED);
        m_MenuOptions.use_default_texture = 1 - m_MenuOptions.use_default_texture;
    }
    break;
    case YYMID_ABOUT:
    {
        RunWithModal([]() {
            MessageBoxW(W32::GetHwnd(), L"yy render -- A simple OBJ model render\nBy include-yy 2024-08-19 18:02 UTC+9", L"About", MB_OK);
        });
    }
    break;
    }
}
