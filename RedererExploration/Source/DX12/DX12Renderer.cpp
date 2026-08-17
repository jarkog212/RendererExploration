#include "DX12Renderer.h"
#include "DX12DescriptorHeap.h"
#include "DX12RenderSurface.h"
#include "DX12Shaders.h"
#include "DX12GPass.h"
#include "DX12PostProcessPass.h"
#include "DX12Utils.h"

namespace
{
	static Microsoft::WRL::ComPtr<ID3D12Device8> COMRendererDevice = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIFactory7> COMDxgiFactory = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIAdapter4> COMDxgiAdapter = nullptr;

	static DX12::RenderPassBase* g_rootRenderNode = nullptr;
	static std::unique_ptr<DX12::RenderSurface> g_mainRenderSurface = nullptr;
	static D3D_FEATURE_LEVEL g_expectedMinimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

	namespace DefferedFree
	{
		// This should be connected with DX12::RendererCommand - likely inside it.

		static int g_deferredResourceFreeingFlag = 0;
		static_assert(sizeof(g_deferredResourceFreeingFlag) * sizeof(char) > DX12::CC::k_frameBuffers, "Flag bit-buffer needs to be wide enough for frame buffer count.");
		static std::vector<IUnknown*> g_deferredCOMReleases[DX12::CC::k_frameBuffers];

		static void FreePendingResources(const int currentFrame = DX12::CC::k_forAllFrames)
		{
			static std::mutex mutex;
			std::lock_guard lock(mutex);

			::DefferedFree::g_deferredResourceFreeingFlag &= ~(1ull << currentFrame);

			DX12::GetDSVHeap()->ProcessPendingResourceFreeing(currentFrame);
			DX12::GetRTVHeap()->ProcessPendingResourceFreeing(currentFrame);
			DX12::GetSRVHeap()->ProcessPendingResourceFreeing(currentFrame);
			DX12::GetUAVHeap()->ProcessPendingResourceFreeing(currentFrame);

			static auto cleaner = [](const unsigned int frame)
				{
					std::vector<IUnknown*>& pendingCOMList = g_deferredCOMReleases[frame];
					for (auto& comObj : pendingCOMList)
						Utils::COM::SafeRelease(comObj);

					pendingCOMList.clear();
				};

			if (currentFrame == DX12::CC::k_forAllFrames)
				for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
					cleaner(i);
			else
				cleaner(currentFrame);
		}
	} // DeferredFree

	namespace ResourceBarriers
	{
		static D3D12_RESOURCE_BARRIER g_bulkResourceBarriers[DX12::CC::k_maxResourceBarriers];
		static unsigned int g_pendingBulkResourceBarriers = 0;
	} // ResourceBarriers

	void BuildRenderGraph()
	{
		g_rootRenderNode = DX12::GPassSpace::GetHeadNode();
		DX12::GPassSpace::GetTailNode()->SetAutoBarrierSubmits(true, true);
	}
}

void UtilsImpl::COM::Release(IUnknown* resource)
{
	resource->Release();
}

void UtilsImpl::COM::DeferredRenderThreadRelease(IUnknown* resource)
{
	unsigned int currentFrame = DX12::GetMainRendererCommand()->GetCurrentFrame();
	::DefferedFree::g_deferredCOMReleases[currentFrame].push_back(resource);
	DX12::SetPendingResourceFreeingFlag();
}

ID3D12Device8* DX12::GetRendererDevice()
{
	ID3D12Device8* device = ::COMRendererDevice.Get();
	if (device) [[likely]]
		return device;

	HRESULT_CHECK_CRITICAL(
		D3D12CreateDevice(DX12::GetDxgiAdapter(), g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device8), (void**)&device),
		"Could not create Dx12 device with selected adapter.")

	// Technically adapter interface is no longer needed
	//::COMDxgiAdapter.Reset();

	HRESULT_CHECK_FALLBACK(device->SetName(L"MainDevice"),,, "Could not set Dx12 device's name.");

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECK_CRITICAL(device->QueryInterface(IID_PPV_ARGS(&infoQueueInterface)), "Couldn't gather queue interface.");
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
	if (factory) [[likely]]
		return factory;

	unsigned int flags = 0;

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12Debug6> debugInterface = nullptr;
		HRESULT_CHECK_CRITICAL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)), "Failed to get debug interface.");
		debugInterface->EnableDebugLayer();

		flags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	HRESULT_CHECK_CRITICAL(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)), "DXGI Factory issue.");

	::COMDxgiFactory.Attach(factory);
	return factory;
}

IDXGIAdapter4* DX12::GetDxgiAdapter()
{
	IDXGIAdapter4* adapter = ::COMDxgiAdapter.Get();
	if (adapter) [[likely]]
		return adapter;

	for (unsigned int i = 0;
		DX12::GetDxgiFactory()->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter4), (void**)&adapter) != DXGI_ERROR_NOT_FOUND;
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

	FALLBACK_ASSERT(adapter, return nullptr, "No good GPU adapter.");

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
	HRESULT_CHECK_CRITICAL(
		D3D12CreateDevice(adapter, ::g_expectedMinimumFeatureLevel, IID_PPV_ARGS(&device)),
		"Could not create Dx12 device with selected adapter.");
	
	HRESULT_CHECK_CRITICAL(
		device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData)),
		"Could not access Dx12 device's feature support information.");

	FALLBACK_ASSERT(
		featureLevelsData.MaxSupportedFeatureLevel >= ::g_expectedMinimumFeatureLevel,
		{
			Utils::COM::SafeRelease(adapter);
			return nullptr;
		},
		"Dx12 device's feature level support is lower than minimum requirement.");

	::g_expectedMinimumFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
	::COMDxgiAdapter.Attach(adapter);
	return adapter;
}

DX12::RenderSurface* DX12::GetMainRenderSurface(
	HWND window,
	DXGI_FORMAT format,
	DX12::RendererCommand* rendererCommand)
{
	if (::g_mainRenderSurface) [[likely]]
		return ::g_mainRenderSurface.get();

	CRITICAL_ASSERT(
		window != NULL && rendererCommand,
		"Uninitialized render surface requires legitimate arguments so it can be initialized.");

	::g_mainRenderSurface = std::make_unique<DX12::RenderSurface>(window, format, *rendererCommand);
	return ::g_mainRenderSurface.get();
}

DX12::RendererCommand* DX12::GetMainRendererCommand()
{
	static DX12::RendererCommand instance(D3D12_COMMAND_LIST_TYPE_DIRECT);
	return &instance;
}

void DX12::SetPendingResourceFreeingFlag()
{
	::DefferedFree::g_deferredResourceFreeingFlag |= 1ull << DX12::GetMainRendererCommand()->GetCurrentFrame();
}

bool DX12::TestPendingResourceFreeingFlag(const unsigned int frame)
{
	return (bool)(::DefferedFree::g_deferredResourceFreeingFlag & (1ull << frame));
}

void DX12::PendingTransitionResourceBarrier(
	ID3D12Resource* resource,
	D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after,
	D3D12_RESOURCE_BARRIER_FLAGS flags,
	unsigned int subresource)
{
	CRITICAL_ASSERT(resource, "No resource to create a barrier for.");
	CRITICAL_ASSERT(::ResourceBarriers::g_pendingBulkResourceBarriers < DX12::CC::k_maxResourceBarriers, "No more space for resource barrier.");

	D3D12_RESOURCE_BARRIER& barrier = ::ResourceBarriers::g_bulkResourceBarriers[::ResourceBarriers::g_pendingBulkResourceBarriers++];
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = flags;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = subresource;
}

void DX12::PendingUAVResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
	CRITICAL_ASSERT(resource, "No resource to create a barrier for.");
	CRITICAL_ASSERT(::ResourceBarriers::g_pendingBulkResourceBarriers < DX12::CC::k_maxResourceBarriers, "No more space for resource barrier.");

	D3D12_RESOURCE_BARRIER& barrier = ::ResourceBarriers::g_bulkResourceBarriers[::ResourceBarriers::g_pendingBulkResourceBarriers++];
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = flags;
	barrier.UAV.pResource = resource;
}

void DX12::PendingAliasingResourceBarrier(ID3D12Resource* resourceBefore, ID3D12Resource* resourceAfter, D3D12_RESOURCE_BARRIER_FLAGS flags)
{
	CRITICAL_ASSERT(resourceBefore, "No resource to alias FROM.");
	CRITICAL_ASSERT(resourceAfter, "No resource to alias INTO.");
	CRITICAL_ASSERT(::ResourceBarriers::g_pendingBulkResourceBarriers < DX12::CC::k_maxResourceBarriers, "No more space for resource barrier.");

	D3D12_RESOURCE_BARRIER& barrier = ::ResourceBarriers::g_bulkResourceBarriers[::ResourceBarriers::g_pendingBulkResourceBarriers++];
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	barrier.Flags = flags;
	barrier.Aliasing.pResourceBefore = resourceBefore;
	barrier.Aliasing.pResourceAfter = resourceAfter;
}

void DX12::SubmitPendingBarriers(ID3D12GraphicsCommandList6* commandList)
{
	if (::ResourceBarriers::g_pendingBulkResourceBarriers == 0)
		return;

	commandList->ResourceBarrier(::ResourceBarriers::g_pendingBulkResourceBarriers, ::ResourceBarriers::g_bulkResourceBarriers);
	::ResourceBarriers::g_pendingBulkResourceBarriers = 0;
}

bool DX12::Initialize(HWND hwnd)
{
	// Things are written lazy init style mostly (encapsulation purposes),
	// warmup efectively moakes the initialization controlled.
	DX12::Shaders::WarmUp();
	DX12::GPassSpace::WarmUp();
	DX12::PostProcessSpace::WarmUp();

	// Build the pass tree
	::BuildRenderGraph();

	DX12::GetMainRenderSurface(
		hwnd,
		DX12::CC::k_defaultRenderTargetFormat,
		DX12::GetMainRendererCommand());

	HRESULT_CHECK_FALLBACK(DX12::GetRTVHeap()->GetHeap()->SetName(L"RTV Heap"),,, "Could not name RTV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"DSV Heap"),,, "Could not name DSV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"SRV Heap"),,, "Could not name SRV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"UAV Heap"),,, "Could not name UAV Heap.");

	return true;
}

void DX12::Render(DX12::RenderSurface& surface, DX12::RenderPassBase* root)
{
	surface.PreRender();
	
	surface.UpdateViewport();

	DX12::RendererCommand* rendererCommand = surface.GetLinkedRendererCommand();
	ID3D12GraphicsCommandList6* commandList = rendererCommand->GetCommandList();

#ifdef _DEBUG
	// HUGE ASSUMPTION!!! THE CURRENT DEFERRED CLEANUP ASSUMES ONLY ONE RENDERER COMMAND
	// USING ANOTHER ONE WILL WREAK HAVOC ONTO HELD RESOURCES
	// MOVING COM POINTER RELEASES IS SIMPLE ENOUGH: MOVE THEIR PENDING RELASE MANAGEMENT INTO THE RENDERER COMMAND OBJECT.
	// MOVING HEAP RELEASES REQUIRES A BIT MORE WORK IN HEAPS TO SOMEHOW LET THEM KNOW HANDLES BEING USED BY WHICH COMMAND.
	static DX12::RendererCommand* mainCommand = DX12::GetMainRendererCommand();
	CRITICAL_ASSERT(rendererCommand == mainCommand, "You're about to start freeing resources which may be protected. Currently only one render command is supported.");
#endif

	const unsigned int currentFrame = rendererCommand->GetCurrentFrame();
	if (TestPendingResourceFreeingFlag(currentFrame))
		::DefferedFree::FreePendingResources(currentFrame);

	DX12::Math::Vector2u32 size{ surface.GetWidth(), surface.GetHeight() };
	DX12::GPassSpace::SetDimensions(size);

	DX12::PendingTransitionResourceBarrier(
		surface.GetBackbuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	if (g_rootRenderNode)
		g_rootRenderNode->AccummulateCommands(commandList);

	// Post Process

	DX12::PendingTransitionResourceBarrier(
		surface.GetBackbuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	DX12::SubmitPendingBarriers(commandList);

	surface.PostRender();
}

void DX12::Render()
{
	DX12::Render(*DX12::GetMainRenderSurface() /*, root pass will go here */);
}

void DX12::Release()
{
	// intended to be called only once at shutdown, reinit is not possible due to static nature of things?
	static auto cleaner = []()
		{
			DX12::GPassSpace::Release();
			DX12::PostProcessSpace::Release();
			DX12::Shaders::Release();

			::g_mainRenderSurface.reset();

			DX12::GetMainRendererCommand()->Release();

			::DefferedFree::FreePendingResources(DX12::CC::k_forAllFrames);

			DX12::GetRTVHeap()->Release();
			DX12::GetDSVHeap()->Release();
			DX12::GetSRVHeap()->Release();
			DX12::GetUAVHeap()->Release();

			// Heap releases add to the deferred pile
			::DefferedFree::FreePendingResources(DX12::CC::k_forAllFrames);

			::COMDxgiFactory.Reset();
			::COMDxgiAdapter.Reset();
			::COMRendererDevice.Reset();
		};
			
#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECK_CRITICAL(::COMRendererDevice->QueryInterface(IID_PPV_ARGS(&infoQueueInterface)), "Couldn't gather info queue interface.");
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
		infoQueueInterface->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);

		Microsoft::WRL::ComPtr<ID3D12DebugDevice2> debugInterface = nullptr;
		HRESULT_CHECK_CRITICAL(::COMRendererDevice->QueryInterface(IID_PPV_ARGS(&debugInterface)), "Couldn't gather debug device interface.");

		cleaner();

		debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	}
#else
	cleaner();
#endif
}

DX12::RendererCommand::RendererCommand(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC cq;
	cq.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cq.NodeMask = 0;
	cq.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cq.Type = type;

	const wchar_t* commandQueueName = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command Queue" :
		type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"GFX Compute Queue" :
		L"Command Queue";

	const wchar_t* commandListName = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command List" :
		type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"GFX Compute List" :
		L"Command List";

	ID3D12Device8* device = DX12::GetRendererDevice();

	HRESULT_CHECK_FALLBACK(
		device->CreateCommandQueue(&cq, IID_PPV_ARGS(&_commandQueue)),,
		goto _error,
		"Could not create command queue.");

	for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
		_frame[i].Setup(type, i);

	HRESULT_CHECK_FALLBACK(
		device->CreateCommandList(0, type, _frame[0].GetAllocator(), nullptr, IID_PPV_ARGS(&_commandList)),,
		goto _error,
		"Could not create command list.");

	HRESULT_CHECK_FALLBACK(
		_commandList->Close(),,
		goto _error,
		"Could not close the command list.")

	HRESULT_CHECK_FALLBACK(_commandQueue->SetName(commandQueueName),,, "Could not set (%ls) to command queue.", commandQueueName);
	HRESULT_CHECK_FALLBACK(_commandList->SetName(commandListName),,, "Could not set (%ls) to command list.", commandListName);

	HRESULT_CHECK_FALLBACK(
		device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)),,
		goto _error,
		"Could not create fence.");

	HRESULT_CHECK_FALLBACK(_commandList->SetName(L"D3D12 Fence"),,, "Could not set (%ls) to command list.", commandListName);

	_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	FALLBACK_ASSERT(_fenceEvent,,"Could not create fence event.");

	return;

_error:

	Release();
}

void DX12::RendererCommand::PreRender()
{
	DX12::RendererCommand::GfxCommandFrame& currentFrame = _frame[_currentFrame];
	currentFrame.WaitForGPU(_fenceEvent, _fence.Get());
	HRESULT_CHECK_CRITICAL(currentFrame.GetAllocator()->Reset(), "Could not clear/reset commandAllocator");
	HRESULT_CHECK_CRITICAL(_commandList->Reset(currentFrame.GetAllocator(), nullptr), "Could not reset command list.");
}

void DX12::RendererCommand::PostRender()
{
	_commandList->Close();
	ID3D12CommandList* const lists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(lists), lists);

	_issuableFenceValue++;
	_frame[_currentFrame].IssueFenceValue(_issuableFenceValue);
	_commandQueue->Signal(_fence.Get(), _issuableFenceValue);

	_currentFrame = ++_currentFrame % DX12::CC::k_frameBuffers;
}

void DX12::RendererCommand::Flush()
{
	for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
		_frame[i].WaitForGPU(_fenceEvent, _fence.Get());

	_issuableFenceValue = 0;
}

void DX12::RendererCommand::Release()
{
	if (_fence)
	{
		Flush();
		_fence.Reset();
		if (_fenceEvent)
		{
			CloseHandle(_fenceEvent);
			_fenceEvent = 0;
		}
	}

	_commandList.Reset();
	_commandQueue.Reset();
	for (int i = 0; i < DX12::CC::k_frameBuffers; i++)
		_frame[i].Reset();

	_currentFrame = 0;
}

bool DX12::RendererCommand::GfxCommandFrame::Setup(D3D12_COMMAND_LIST_TYPE type, int index)
{
	HRESULT_CHECK_FALLBACK(
		DX12::GetRendererDevice()->CreateCommandAllocator(type, IID_PPV_ARGS(&_allocator)),,
		return false,
		"Could not create command allocator.");

	wchar_t buffer[250];
	swprintf(buffer, 250, L"Command Allocator %u", index);
	HRESULT_CHECK_FALLBACK(_allocator->SetName(buffer),,,"Could not set name (%ls) to command buffer.", buffer);

	return true;
}

void DX12::RendererCommand::GfxCommandFrame::Reset()
{
	_allocator.Reset();
	_fenceValue = 0;
}

void DX12::RendererCommand::GfxCommandFrame::WaitForGPU(HANDLE fenceEvent, ID3D12Fence1* fence)
{
	assert(fenceEvent && fence);

	if (fence->GetCompletedValue() >= _fenceValue)
		return;
	
	HRESULT_CHECK_CRITICAL(fence->SetEventOnCompletion(_fenceValue, fenceEvent), "Could not assign fence signal to event handle.");

	WaitForSingleObject(fenceEvent, INFINITE);
}
