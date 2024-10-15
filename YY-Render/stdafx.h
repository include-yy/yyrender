#pragma once

/// Standard C++ headers

// Utilities library
#include <chrono>
#include <tuple>
// Dynamic memory management
#include <memory>
// Numeric limits
#include <limits>
// Error handling
#include <exception>
#include <stdexcept>
#include <system_error>
// String library
#include <format>
#include <string>
#include <string_view>
// Containers library
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>
// Numerics library
#include <cmath>
#include <numbers>
// Input/output library
#include <iostream>
#include <sstream>
// Filesystem library
#include <filesystem>

/// Windows

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <wrl.h>
#include <shellapi.h>
#include <wincodec.h>

/// DirectX12
#include "directx/d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>


#undef min
#undef max
