#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

namespace DX12
{

	static D3D_FEATURE_LEVEL expectedMinimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

	ID3D12Device8* GetRendererDevice();

	IDXGIFactory7* GetDxgiFactory();

	IDXGIAdapter4* GetDxgiAdapter();

	void ResetAll();

};





