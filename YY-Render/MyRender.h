// partially from https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HelloWorld/src/HelloTriangle/D3D12HelloTriangle.h

//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "Framework.h"
#include "yyobj.h"
#include "StepTimer.h"
#include "SimpleCamera.h"
#include "LoadTexture.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class MyRender : public Framework
{
public:
    MyRender(UINT width, UINT height, std::wstring name, std::string initobj);

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnPaint();
    virtual void OnDestroy();

    virtual void OnKeyDown(UINT8 keyValue) { m_camera.OnKeyDown(keyValue); }
    virtual void OnKeyUp(UINT8 keyValue) { m_camera.OnKeyUp(keyValue); }
    virtual void OnMouseDown(WPARAM btnState, int x, int y) { m_camera.OnMouseDown(btnState, x, y); }
    virtual void OnMouseUp(WPARAM btnState, int x, int y) { m_camera.OnMouseUp(btnState, x, y); }
    virtual void OnMouseMove(WPARAM btnState, int x, int y) { m_camera.OnMouseMove(btnState, x, y); }
    virtual void OnMouseWheel(int delta) { m_camera.OnMouseWheel(delta); }

    // menu message
    virtual void OnCommand(WORD id);
    // Change to a new Objwave Model
    void ChangeModel(std::string);

private:
    static const UINT FrameCount = 2;

    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;

    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    // heaps descriptors
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    ComPtr<ID3D12DescriptorHeap> m_samplerHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap;
    UINT m_rtvDescriptorSize{};
    UINT m_cbvSrvDescriptorSize{};
    UINT m_dsvDescriptorSize{};
    UINT m_smpDescriptorSize{};

    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    UINT m_frameIndex{};

    ComPtr<ID3D12RootSignature> m_rootSignature;
    // a collection of all pipelineState
    std::unordered_map<int, ComPtr<ID3D12PipelineState>> m_pipeStates{};
    // current pipelinestate
    ID3D12PipelineState* m_pipelineState{};
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    ComPtr<ID3D12Resource> m_depthStencil;

    // CBV Upload resources heap
    ComPtr<ID3D12Resource> m_cbvUploadHeap;

    // Synchronization objects.
    HANDLE m_fenceEvent{};
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue{};

    // Consts
    struct SceneConstantBuffer {
        XMFLOAT4X4 mvp;
        XMFLOAT3 light;
        UINT useDefTex;
        XMFLOAT3 eyepos;
        FLOAT pad[1];
        XMFLOAT4X4 lighteye_mvp;
        FLOAT padding[24];
    };
    XMFLOAT4X4 m_model{}; // Model transform matrix.
    SceneConstantBuffer* m_pConstBuffer{};

    //Obj-related data
    std::string m_objpath;
    struct MTLparameters
    {
        FLOAT Ka[3];
        UINT32 map_Ka;
        FLOAT Kd[3];
        UINT32 map_Kd;
        FLOAT Ks[3];
        UINT32 map_Ks;
        FLOAT Ke[3];
        UINT32 map_Ke;
        FLOAT Kt[3];
        UINT32 map_Kt;
        FLOAT Ns;
        UINT32 map_Ns;
        FLOAT Ni;
        UINT32 map_Ni;
        FLOAT Tf[3];
        FLOAT d;
        UINT32 map_d;
        UINT32 illum;
        UINT32 map_bump;
        UINT32 fallback;
        FLOAT padding[32];
    };
    struct TextureInfo
    {
        D3D12_RESOURCE_DESC textureDesc{};
        INT bytesPerRow{};
        INT imgSize{};
        std::unique_ptr<BYTE[]> pData{};
    };

    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
    std::vector<yyobj::Recoder> m_indicesRecoder;
    std::vector<yyobj::Material> m_materials;
    std::vector<ComPtr<ID3D12Resource>> m_chanTextures;

    // camera and timer
    SimpleCamera m_camera{};
    StepTimer m_timer{};
    UINT m_frameCounter{};

    // MSAA support
    UINT m_useMSAA{ 0 };
    ComPtr<ID3D12Resource> m_offScrrenBuffer;

    // Menu options
    struct MenuOption
    {
        //UINT use_light{1};
        UINT use_default_texture{1};
    };
    MenuOption m_MenuOptions{};

    // ShadowMap support
    ComPtr<ID3D12Resource> m_ShadowMapDepth;
    ComPtr<ID3D12PipelineState> m_ShadowMapPSO;
    bool m_useShadowMap{ false };
    // One time initializers
    void LoadPipeline();
    void LoadAssetsOnce();
    void CreateRootSignature();
    void CreateDepthStencil();
    void CreateFence();
    void CreateSampler();
    void CreatePipelineState(int id, LPCWSTR vs, LPCWSTR ps);
    void CreateShadowMapPSO(LPCWSTR vs);
    // Load resources
    void LoadAssets();
    void LoadObjData(std::unique_ptr<yyobj::Mesh>& pMesh, std::vector<yyobj::Vertex>& vertices, std::vector<yyobj::Material>& materials);
    void LoadTexData(std::unique_ptr<yyobj::Mesh>& pMesh, std::vector<TextureInfo>& tInfos);
    void UpdatecbvSrvHeap(UINT num);
    void CreateVertexBuffer(yyobj::Vertex* pVertices, size_t numOfVertex, ComPtr<ID3D12Resource>& vertexBufferUploadHeap);
    void AdjustCamera(yyobj::Mesh* pMesh);
    void CreateConstbuffer(yyobj::Mesh* pMesh);
    void CreateTexture(std::vector<TextureInfo>& tInfos, ComPtr<ID3D12Resource>& textureUploadHeap);
    // Sync
    void WaitForPreviousFrame();
    void PopulateCommandList();
};
