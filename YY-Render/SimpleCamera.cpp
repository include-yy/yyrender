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

#include "stdafx.h"
#include "SimpleCamera.h"

SimpleCamera::SimpleCamera() :
    m_initialPosition(0, 0, 0),
    m_position(m_initialPosition),
    m_yaw(XM_PI),
    m_pitch(0.0f),
    m_lookDirection(0, 0, -1),
    m_upDirection(0, 1, 0),
    m_moveSpeed(20.0f),
    m_turnSpeed(XM_PIDIV2),
    m_keysPressed{}
{
}

void SimpleCamera::Init(XMFLOAT3 position, XMFLOAT3 light, XMFLOAT3 xyzbound)
{
    m_initialPosition = position;
    m_light = light;
    m_Key0Mouse1 = use_mouse;
    auto& b = xyzbound;
    m_CubeVertices[0] = XMVectorSet(b.x, b.y, b.z, 1.0f);
    m_CubeVertices[1] = XMVectorSet(b.x, b.y, -b.z, 1.0f);
    m_CubeVertices[2] = XMVectorSet(b.x, -b.y, b.z, 1.0f);
    m_CubeVertices[3] = XMVectorSet(b.x, -b.y, -b.z, 1.0f);
    m_CubeVertices[4] = XMVectorSet(-b.x, b.y, b.z, 1.0f);
    m_CubeVertices[5] = XMVectorSet(-b.x, b.y, -b.z, 1.0f);
    m_CubeVertices[6] = XMVectorSet(-b.x, -b.y, b.z, 1.0f);
    m_CubeVertices[7] = XMVectorSet(-b.x, -b.y, -b.z, 1.0f);
    float distance = std::sqrtf(b.x * b.x + b.y * b.y + b.z * b.z);
    m_MouseDistanceMin = distance / 10.0f;
    m_MouseDistanceMax = distance * 10.0f;
    m_lightDistance = distance * 2.0f;
    SetMoveSpeed(distance / 4.0f);
    Reset();
}

void SimpleCamera::SetMoveSpeed(float unitsPerSecond)
{
    m_moveSpeed = unitsPerSecond;
}

void SimpleCamera::SetTurnSpeed(float radiansPerSecond)
{
    m_turnSpeed = radiansPerSecond;
}

void SimpleCamera::Reset()
{
    m_position = m_initialPosition;
    m_yaw = XM_PI;
    m_pitch = 0.0f;
    m_lookDirection = { 0, 0, -1 };

    // light
    m_xzRotateAngle = atan2f(m_light.z, m_light.x);
    float length = sqrtf(m_light.x * m_light.x + m_light.z * m_light.z);
    m_yUpdownAngle = atan2f(m_light.y, length);
    //m_keysPressed.h = true;

    // mouse
    m_MouseXZ = atan2f(m_position.z, m_position.x);
    length = sqrtf(m_position.x * m_position.x + m_position.z * m_position.z);
    m_MouseY = atan2f(m_position.y, length);
    m_MouseDistance = sqrtf(length * length + m_position.y * m_position.y);
    m_MousePosDelta.x = 0.0f, m_MousePosDelta.y = 0.0f;
    m_MouseWheelDelta = 0;
    m_MousePosTowards = {};
}

void SimpleCamera::Update(float elapsedSeconds)
{
    if (m_Key0Mouse1 == use_keyboard)
    {
        // Calculate the move vector in camera space.
        XMFLOAT3 move(0, 0, 0);

        if (m_keysPressed.a)
            move.x -= 1.0f;
        if (m_keysPressed.d)
            move.x += 1.0f;
        if (m_keysPressed.w)
            move.z -= 1.0f;
        if (m_keysPressed.s)
            move.z += 1.0f;
        if (fabs(move.x) > 0.1f && fabs(move.z) > 0.1f)
        {
            XMVECTOR vector = XMVector3Normalize(XMLoadFloat3(&move));
            move.x = XMVectorGetX(vector);
            move.z = XMVectorGetZ(vector);
        }

        float moveInterval = m_moveSpeed * elapsedSeconds;
        float rotateInterval = m_turnSpeed * elapsedSeconds;

        if (m_keysPressed.left)
            m_yaw += rotateInterval;
        if (m_keysPressed.right)
            m_yaw -= rotateInterval;
        if (m_keysPressed.up)
            m_pitch += rotateInterval;
        if (m_keysPressed.down)
            m_pitch -= rotateInterval;

        // Prevent looking too far up or down.
        m_pitch = std::min(m_pitch, XM_PIDIV2 - 0.01f);
        m_pitch = std::max(-XM_PIDIV2 + 0.01f, m_pitch);

        // Move the camera in model space.
        float x = move.x * -cosf(m_yaw) - move.z * sinf(m_yaw);
        float z = move.x * sinf(m_yaw) - move.z * cosf(m_yaw);
        float y = -move.z * tanf(m_pitch);
        m_position.x += x * moveInterval;
        m_position.z += z * moveInterval;
        m_position.y += y * moveInterval;
        // Determine the look direction.
        float r = cosf(m_pitch);
        m_lookDirection.x = r * sinf(m_yaw);
        m_lookDirection.y = sinf(m_pitch);
        m_lookDirection.z = r * cosf(m_yaw);
    }
    
    // Change light
    {
        float me_rotateInterval = 3 * m_turnSpeed * elapsedSeconds;
        if (m_keysPressed.h)
            m_xzRotateAngle += me_rotateInterval;
        if (m_keysPressed.l)
            m_xzRotateAngle -= me_rotateInterval;
        if (m_keysPressed.j)
            m_yUpdownAngle += me_rotateInterval;
        if (m_keysPressed.k)
            m_yUpdownAngle -= me_rotateInterval;
        m_yUpdownAngle = std::min(m_yUpdownAngle, XM_PIDIV2 - 0.1f);
        m_yUpdownAngle = std::max(m_yUpdownAngle, -XM_PIDIV2 + 0.1f);
    }

    if (m_Key0Mouse1 == use_mouse)
    // Change mouse
    {
        // Calculate the move vector in camera space.
        XMFLOAT3 move(0, 0, 0);

        if (m_keysPressed.a)
            move.x -= 1.0f;
        if (m_keysPressed.d)
            move.x += 1.0f;
        if (m_keysPressed.w)
            move.y += 1.0f;
        if (m_keysPressed.s)
            move.y -= 1.0f;

        float moveInterval = m_moveSpeed * elapsedSeconds;
        m_MousePosTowards.x += move.x * moveInterval;
        m_MousePosTowards.y += move.y * moveInterval;
        m_MousePosTowards.z += move.z * moveInterval;

        m_MouseXZ += m_MousePosDelta.x;
        m_MouseY += m_MousePosDelta.y;
        m_MouseY = std::min(m_MouseY, XM_PIDIV2 - 0.0001f);
        m_MouseY = std::max(m_MouseY, -XM_PIDIV2 + 0.0001f);
        m_MousePosDelta.x = 0.0f, m_MousePosDelta.y = 0.0f;

        // deal with Wheel
        if (m_MouseWheelDelta < 0 || m_keysPressed.q ) {
            m_MouseDistance *= 1.1f;
        }
        else if (m_MouseWheelDelta > 0 || m_keysPressed.e) {
            m_MouseDistance /= 1.1f;
        }
        m_MouseDistance = std::min(m_MouseDistance, m_MouseDistanceMax);
        m_MouseDistance = std::max(m_MouseDistanceMin, m_MouseDistance);
        m_MouseWheelDelta = 0;
    }
}

XMFLOAT3 SimpleCamera::GetLight() {
    float x = cosf(m_xzRotateAngle);
    float z = sinf(m_xzRotateAngle);
    float y = sinf(m_yUpdownAngle);
    XMFLOAT3 res = { x, y, z };
    return res;
}

XMFLOAT3 SimpleCamera::GetEyePos()
{
    if (m_Key0Mouse1 == use_keyboard)
    {
        return m_position;
    }
    else
    {
        XMFLOAT3 pos{};
        float len_xz = m_MouseDistance * cosf(m_MouseY);
        pos.x = len_xz * cosf(m_MouseXZ);
        pos.z = len_xz * sinf(m_MouseXZ);
        pos.y = m_MouseDistance * sinf(m_MouseY);
        return pos;
    }
}

XMMATRIX SimpleCamera::GetViewMatrix()
{
    XMMATRIX viewM{};
    if (m_Key0Mouse1 == 0)
    {
        viewM = XMMatrixLookToRH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_lookDirection), XMLoadFloat3(&m_upDirection));
        //return XMMatrixLookToLH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_lookDirection), XMLoadFloat3(&m_upDirection));
    }
    else if (m_Key0Mouse1 == 1)
    {
        XMFLOAT3 pos{};
        auto& v = m_MousePosTowards;
        XMVECTOR org = XMVectorSet(v.x, v.y, v.z, 0.0f);
        float len_xz = m_MouseDistance * cosf(m_MouseY);
        pos.x = len_xz * cosf(m_MouseXZ);
        pos.z = len_xz * sinf(m_MouseXZ);
        pos.y = m_MouseDistance * sinf(m_MouseY);
        viewM = XMMatrixLookAtRH(XMLoadFloat3(&pos), org, XMLoadFloat3(&m_upDirection));
    }
    else {
        //PLOGE << "WTF? m_Key0Mose1 is not 0 or 1";
        viewM = XMMatrixLookToRH(XMLoadFloat3(&m_position), XMLoadFloat3(&m_lookDirection), XMLoadFloat3(&m_upDirection));
    }
    XMVECTOR vertices[8]{};
    for (int i = 0; i < 8; i++)
    {
        vertices[i] = XMVector4Transform(m_CubeVertices[i], viewM);
    }
    float zmin = std::numeric_limits<float>::infinity();
    float zmax = -std::numeric_limits<float>::infinity();
    for (int i = 0; i < 8; i++)
    {
        float z = XMVectorGetZ(vertices[i]);
        zmin = std::min(zmin, z);
        zmax = std::max(zmax, z);
    }
    m_farplane = zmin < 0 ? -zmin + 0.01f : m_MouseDistanceMax;
    m_nearplane = zmax < 0 ? std::max(m_MouseDistanceMin / 10.0f,  - zmax - 0.01f) : m_MouseDistanceMin / 10.0f;
    return viewM;
}

XMMATRIX SimpleCamera::GetLightViewMatrix(float aspectRatio)
{
    // use light as eye
    // the light comes from the farset place that camera can go.
    XMFLOAT3 pos{};
    XMVECTOR org = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    float len_xz = m_lightDistance * cosf(m_yUpdownAngle);
    pos.x = len_xz * cosf(m_xzRotateAngle);
    pos.z = len_xz * sinf(m_xzRotateAngle);
    pos.y = m_lightDistance * sinf(m_yUpdownAngle);
    auto viewM = XMMatrixLookAtRH(XMLoadFloat3(&pos), org, XMLoadFloat3(&m_upDirection));
    XMVECTOR vertices[8]{};
    for (int i = 0; i < 8; i++)
    {
        vertices[i] = XMVector4Transform(m_CubeVertices[i], viewM);
    }
    float zmin = std::numeric_limits<float>::infinity();
    float zmax = -std::numeric_limits<float>::infinity();
    float xmax{ 0.0f }, ymax{ 0.0f };
    for (int i = 0; i < 8; i++)
    {
        float z = XMVectorGetZ(vertices[i]);
        zmin = std::min(zmin, z);
        zmax = std::max(zmax, z);
        float x = XMVectorGetX(vertices[i]);
        float y = XMVectorGetY(vertices[i]);
        xmax = std::max(std::abs(x), xmax);
        ymax = std::max(std::abs(y), ymax);
    }

    m_light_farplane = -zmin + 0.01f;
    m_light_nearplane = -zmax + 0.01f;
    m_light_width = 2 * xmax;
    m_light_height = 2 * ymax;
    if (m_light_width / m_light_height > aspectRatio)
    {
        m_light_height = m_light_width / aspectRatio;
    }
    else
    {
        m_light_width = m_light_height * aspectRatio;
    }
    return viewM;
}

XMMATRIX SimpleCamera::GetProjectionMatrix(float fov, float aspectRatio)
{
    //return XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane);
    return XMMatrixPerspectiveFovRH(fov, aspectRatio, m_nearplane, m_farplane);
    //return XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
}

XMMATRIX SimpleCamera::GetLightProjMatrix()
{
    return XMMatrixOrthographicRH(m_light_width, m_light_height, m_light_nearplane, m_light_farplane);
}

void SimpleCamera::OnKeyDown(WPARAM key)
{
    switch (key)
    {
    case 'W':
        m_keysPressed.w = true;
        break;
    case 'A':
        m_keysPressed.a = true;
        break;
    case 'S':
        m_keysPressed.s = true;
        break;
    case 'D':
        m_keysPressed.d = true;
        break;
    case 'H':
        m_keysPressed.h = true;
        break;
    case 'J':
        m_keysPressed.j = true;
        break;
    case 'K':
        m_keysPressed.k = true;
        break;
    case 'L':
        m_keysPressed.l = true;
        break;
    case 'T':
        m_Key0Mouse1 = 1 - m_Key0Mouse1;
        break;
    case 'Q':
        m_keysPressed.q = true;
        break;
    case 'E':
        m_keysPressed.e = true;
        break;
    case VK_LEFT:
        m_keysPressed.left = true;
        break;
    case VK_RIGHT:
        m_keysPressed.right = true;
        break;
    case VK_UP:
        m_keysPressed.up = true;
        break;
    case VK_DOWN:
        m_keysPressed.down = true;
        break;
    case VK_ESCAPE:
        Reset();
        break;
    }
}

void SimpleCamera::OnKeyUp(WPARAM key)
{
    switch (key)
    {
    case 'W':
        m_keysPressed.w = false;
        break;
    case 'A':
        m_keysPressed.a = false;
        break;
    case 'S':
        m_keysPressed.s = false;
        break;
    case 'D':
        m_keysPressed.d = false;
        break;
    case 'H':
        m_keysPressed.h = false;
        break;
    case 'J':
        m_keysPressed.j = false;
        break;
    case 'K':
        m_keysPressed.k = false;
        break;
    case 'L':
        m_keysPressed.l = false;
        break;
    case 'Q':
        m_keysPressed.q = false;
        break;
    case 'E':
        m_keysPressed.e = false;
        break;
    case VK_LEFT:
        m_keysPressed.left = false;
        break;
    case VK_RIGHT:
        m_keysPressed.right = false;
        break;
    case VK_UP:
        m_keysPressed.up = false;
        break;
    case VK_DOWN:
        m_keysPressed.down = false;
        break;
    }
}

void SimpleCamera::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_LastMousePos.x = x;
    m_LastMousePos.y = y;
    //PLOGD << x << " " << y;
}

void SimpleCamera::OnMouseUp(WPARAM btnState, int x, int y)
{
    // nothing need todo.
}

void SimpleCamera::OnMouseMove(WPARAM btnState, int x, int y)
{
    if ((btnState & MK_LBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_LastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_LastMousePos.y));
        m_MousePosDelta.x += dx;
        m_MousePosDelta.y += dy;
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        float dx = XMConvertToRadians(0.05f * static_cast<float>(x - m_LastMousePos.x));
        float dy = XMConvertToRadians(0.05f * static_cast<float>(y - m_LastMousePos.y));
        m_MousePosDelta.x += dx;
        m_MousePosDelta.y += dy;
    }
    m_LastMousePos.x = x;
    m_LastMousePos.y = y;
}

void SimpleCamera::OnMouseWheel(int delta)
{
    m_MouseWheelDelta += delta;
}
