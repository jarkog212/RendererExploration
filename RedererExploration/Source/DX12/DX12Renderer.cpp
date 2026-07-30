#include "DX12Renderer.h"

#include "Utils.h"
#include <cstdio>

namespace
{
	static Microsoft::WRL::ComPtr<ID3D12Device8> COMRendererDevice = nullptr;

	static Microsoft::WRL::ComPtr<IDXGIFactory7> COMDxgiFactory = nullptr;

	static Microsoft::WRL::ComPtr<IDXGIAdapter4> COMDxgiAdapter = nullptr;
}

ID3D12Device8* DX12::GetRendererDevice()
{
	ID3D12Device8* device = ::COMRendererDevice.Get();
	if (device)
		return device;

	HRESULT_CHECKED(
		D3D12CreateDevice(GetDxgiAdapter(), expectedMinimumFeatureLevel, __uuidof(ID3D12Device), (void**)device),
		"Could not create Dx12 device with selected adapter.")

	// Technically adapter interface is no longer needed
	//::COMDxgiAdapter.Reset();

	HRESULT_CHECKED(device->SetName(L"MainDevice"), "Could not set Dx12 device's name.")

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECKED(device->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)infoQueueInterface.GetAddressOf()), "Couldn't gather queue interface.")
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif

	::COMRendererDevice.Attach(device);
	return device;
}

IDXGIFactory7* DX12::GetDxgiFactory()
{	
	IDXGIFactory7* factory = ::COMDxgiFactory.Get();
	if (factory)
		return factory;

	unsigned int flags = 0;

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12Debug6> debugInterface = nullptr;
		HRESULT_CHECKED(D3D12GetDebugInterface(__uuidof(ID3D12Debug6), (void**)debugInterface.GetAddressOf()), "Failed to get debug interface.")
		debugInterface->EnableDebugLayer();

		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT_CHECKED(CreateDXGIFactory2(flags, __uuidof(IDXGIFactory7), (void**)&factory), "DXGI Factory issue.")

	::COMDxgiFactory.Attach(factory);
	return factory;
}

IDXGIAdapter4* DX12::GetDxgiAdapter()
{
	IDXGIAdapter4* adapter = ::COMDxgiAdapter.Get();
	if (adapter)
		return adapter;

	IDXGIFactory7* factory = GetDxgiFactory();
	for (unsigned int i = 0;
		factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter4), (void**)&adapter) != DXGI_ERROR_NOT_FOUND;
		++i)
	{
		if (SUCCEEDED(D3D12CreateDevice(adapter, expectedMinimumFeatureLevel, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
		else
		{
			Utils::COM::SafeRelease(adapter);
		}
	}

	if (!adapter)
	{
		DISPLAY_ERROR(L"No good GPU adapter.")
		return nullptr;
	}

	constexpr D3D_FEATURE_LEVEL featureLevels[4]
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsData;
	featureLevelsData.NumFeatureLevels = _countof(featureLevels);
	featureLevelsData.pFeatureLevelsRequested = featureLevels;

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	HRESULT_CHECKED(
		D3D12CreateDevice(adapter, expectedMinimumFeatureLevel, __uuidof(ID3D12Device), (void**)device.GetAddressOf()),
		"Could not create Dx12 device with selected adapter.")
	
	HRESULT_CHECKED(
		device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData)),
		"Could not access Dx12 device's feature support information.")

	if (featureLevelsData.MaxSupportedFeatureLevel < expectedMinimumFeatureLevel)
	{
		DISPLAY_ERROR(L"Dx12 device's feature level support is lover than minimum requirement.")
		Utils::COM::SafeRelease(adapter);
		return nullptr;
	}

	expectedMinimumFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
	::COMDxgiAdapter.Attach(adapter);
	return adapter;
}

void DX12::ResetAll()
{
	auto cleaner = []()
		{
			::COMDxgiFactory.Reset();
			::COMRendererDevice.Reset();
			::COMDxgiAdapter.Reset();
		};

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECKED(::COMRendererDevice->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)infoQueueInterface.GetAddressOf()), "Couldn't gather queue interface.")
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);

		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugInterface = nullptr;
		HRESULT_CHECKED(::COMRendererDevice->QueryInterface(__uuidof(ID3D12DebugDevice2), (void**)debugInterface.GetAddressOf()), "Couldn't gather debug interface.")

		cleaner();

		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	}
#else
	cleaner();
#endif
}
