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
		D3D12CreateDevice(GetDxgiAdapter(), g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), (void**)device),
		"Could not create Dx12 device with selected adapter.")

	// Technically adapter interface is no longer needed
	//::COMDxgiAdapter.Reset();

	HRESULT_CHECKED_FALLBACK(device->SetName(L"MainDevice"),, "Could not set Dx12 device's name.");

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECKED(device->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)infoQueueInterface.GetAddressOf()), "Couldn't gather queue interface.");
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
		HRESULT_CHECKED(D3D12GetDebugInterface(__uuidof(ID3D12Debug6), (void**)debugInterface.GetAddressOf()), "Failed to get debug interface.");
		debugInterface->EnableDebugLayer();

		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT_CHECKED(CreateDXGIFactory2(flags, __uuidof(IDXGIFactory7), (void**)&factory), "DXGI Factory issue.");

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
		if (SUCCEEDED(D3D12CreateDevice(adapter, g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), nullptr)))
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
		DISPLAY_ERROR(L"No good GPU adapter.");
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
		D3D12CreateDevice(adapter, g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), (void**)device.GetAddressOf()),
		"Could not create Dx12 device with selected adapter.");
	
	HRESULT_CHECKED(
		device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData)),
		"Could not access Dx12 device's feature support information.");

	if (featureLevelsData.MaxSupportedFeatureLevel < g_expectedMinimumFeatureLevel)
	{
		DISPLAY_ERROR(L"Dx12 device's feature level support is lover than minimum requirement.");
		Utils::COM::SafeRelease(adapter);
		return nullptr;
	}

	g_expectedMinimumFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
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
		HRESULT_CHECKED(::COMRendererDevice->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)infoQueueInterface.GetAddressOf()), "Couldn't gather info queue interface.");
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);

		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugInterface = nullptr;
		HRESULT_CHECKED(::COMRendererDevice->QueryInterface(__uuidof(ID3D12DebugDevice2), (void**)debugInterface.GetAddressOf()), "Couldn't gather debug device interface.");

		cleaner();

		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	}
#else
	cleaner();
#endif
}

DX12::DX12Command::DX12Command(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC cq;
	cq.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cq.NodeMask = 0;
	cq.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cq.Type = type;

	HRESULT_CHECKED_FALLBACK(
		GetRendererDevice()->CreateCommandQueue(&cq, __uuidof(ID3D12CommandQueue), (void**)_commandQueue.GetAddressOf()),
		goto _error,
		"Could not create command queue.");

	for (int i = 0; i < k_frameBuffers; i++)
	{
		HRESULT_CHECKED_FALLBACK(
			GetRendererDevice()->CreateCommandAllocator(type, _uuidof(ID3D12CommandAllocator), (void**)_commandAllocators[i].GetAddressOf()),
			goto _error,
			"Could not create command allocator.");

		wchar_t buffer[1000];
		swprintf(buffer, 1000, L"Command Allocator %u", i);
		HRESULT_CHECKED_FALLBACK(_commandAllocators[i]->SetName(buffer), , "Could not set name (%ls) to command buffer.", buffer)
	}

	HRESULT_CHECKED_FALLBACK(
		GetRendererDevice()->CreateCommandList(0, type, _commandAllocators[0].Get(), nullptr, __uuidof(ID3D12GraphicsCommandList6), (void**)_commandList.GetAddressOf()),
		goto _error,
		"Could not create command list.");

	switch (type)
	{
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
		{
			constexpr wchar_t name[] = L"GFX Command Queue";
			HRESULT_CHECKED_FALLBACK(_commandQueue->SetName(name), , "Could not set (%ls) to command queue.", name);
			HRESULT_CHECKED_FALLBACK(_commandList->SetName(name), , "Could not set (%ls) to command list.", name);
			break;
		}
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		{
			constexpr wchar_t name[] = L"GFX Compute Queue";
			HRESULT_CHECKED_FALLBACK(_commandQueue->SetName(name), , "Could not set (%ls) to command queue.", name);
			HRESULT_CHECKED_FALLBACK(_commandList->SetName(name), , "Could not set (%ls) to command list.", name);
			break;
		}
		default:
		{
			constexpr wchar_t name[] = L"Command Queue";
			HRESULT_CHECKED_FALLBACK(_commandQueue->SetName(name), , "Could not set (%ls) to command queue.", name);
			HRESULT_CHECKED_FALLBACK(_commandList->SetName(name), , "Could not set (%ls) to command list.", name);
			break;
		}
	}

	return;

_error:

	_commandList.Reset();
	_commandQueue.Reset();
	for (int i = 0; i < k_frameBuffers; i++)
		_commandAllocators[i].Reset();
}

void DX12::DX12Command::PreRender()
{
	ID3D12CommandAllocator* currentAllocator = _commandAllocators[_currentFrame].Get();
	WaitForGPU();
	HRESULT_CHECKED(currentAllocator->Reset(), "Could not clear/reset commandAllocator");
	HRESULT_CHECKED(_commandList->Reset(currentAllocator, nullptr), "Could not reset command list.");
}

void DX12::DX12Command::PostRender()
{
	_commandList->Close();
	ID3D12CommandList* const lists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(lists), lists);
	_currentFrame = ++_currentFrame % k_frameBuffers;
}

void DX12::DX12Renderer::Render()
{
	PreRender();



	PostRender();
}

void DX12::DX12Renderer::PreRender()
{
}


