// partially from https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12DynamicIndexing/src/SimpleCamera.h

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

using namespace DirectX;

class SimpleCamera
{
public:
    SimpleCamera();

    void Init(XMFLOAT3 position, XMFLOAT3 light, XMFLOAT3 xyzbound);
    void Update(float elapsedSeconds);
    XMMATRIX GetViewMatrix();
    XMMATRIX GetLightViewMatrix(float aspectRatio);
    XMMATRIX GetProjectionMatrix(float fov, float aspectRatio);
    XMMATRIX GetLightProjMatrix();
    XMFLOAT3 GetLight();
    XMFLOAT3 GetEyePos();
    void SetMoveSpeed(float unitsPerSecond);
    void SetTurnSpeed(float radiansPerSecond);

    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);
    // mouse message
    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y);
    void OnMouseWheel(int delta);

private:
    void Reset();

    // use keyboard or use mouse
    enum {use_keyboard, use_mouse};
    int m_Key0Mouse1{use_mouse};

    // eight vertices for bounding box
    XMVECTOR m_CubeVertices[8]{};
    // plane distance for calculating projection matrix
    float m_farplane{};
    float m_nearplane{};
    float m_light_farplane{};
    float m_light_nearplane{};
    float m_light_width{};
    float m_light_height{};

    struct KeysPressed
    {
        // keyboards mode movement
        bool w{};
        bool a{};
        bool s{};
        bool d{};
        // keyboard mode 
        bool left{};
        bool right{};
        bool up{};
        bool down{};

        // rotate of light
        bool h{};
        bool j{};
        bool k{};
        bool l{};

        // scale-in and scale-out for mouse op
        bool q{};
        bool e{};

        // switch of keyboard/mouse
        bool t{};
    };

    // camera initial position
    XMFLOAT3 m_initialPosition{};
    // camera current position
    XMFLOAT3 m_position{};
    float m_yaw{};                  // Relative to the +z axis.
    float m_pitch{};                // Relative to the xz plane.
    XMFLOAT3 m_lookDirection{};
    XMFLOAT3 m_upDirection{};
    float m_moveSpeed{};            // Speed at which the camera moves, in units per second.
    float m_turnSpeed{};            // Speed at which the camera turns, in radians per second.
    KeysPressed m_keysPressed{};

    // light
    XMFLOAT3 m_light{}; // original light
    float m_xzRotateAngle{}; // [0, 2*pi]
    float m_yUpdownAngle{};  // [-0.5pi, 0.5pi]
    float m_lightDistance{}; // light distance to origin

    // mouse info
    XMFLOAT3 m_MousePosTowards{}; // Camera towards to for mouse mode
    XMINT2 m_LastMousePos{};
    XMFLOAT2 m_MousePosDelta{};
    float m_MouseXZ{}; // rotate angle relative to +z axis. [0, 2*pi]
    float m_MouseY{};  // rotate angle relative to xz plane.[-pi/2, pi/2]
    float m_MouseDistance{}; // distance to the original point
    float m_MouseDistanceMax{}; // max distance
    float m_MouseDistanceMin{}; // min distance
    int m_MouseWheelDelta{};
};
