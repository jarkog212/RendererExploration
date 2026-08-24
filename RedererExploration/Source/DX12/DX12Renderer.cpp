#include "DX12Renderer.h"
#include "DX12DescriptorHeap.h"
#include "DX12RenderSurface.h"
#include "DX12Shaders.h"
#include "DX12GPass.h"
#include "DX12PostProcessPass.h"
#include "DX12MandelbrotPass.h"
#include "DX12Utils.h"

#include <string>
#include <format>

namespace
{
	static Microsoft::WRL::ComPtr<ID3D12Device8> COMRendererDevice = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIFactory7> COMDxgiFactory = nullptr;
	static Microsoft::WRL::ComPtr<IDXGIAdapter4> COMDxgiAdapter = nullptr;

	static std::unique_ptr<DX12::RenderSurface> g_mainRenderSurface = nullptr;
	static D3D_FEATURE_LEVEL g_expectedMinimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

	namespace DefferedFree
	{
		// This should be connected with DX12::RendererCommand - likely inside it.

		static int g_deferredResourceFreeingFlag = 0;
		static_assert(sizeof(g_deferredResourceFreeingFlag) * sizeof(char) > DX12::CC::k_commandFrameBuffers, "Flag bit-buffer needs to be wide enough for frame buffer count.");
		static std::vector<IUnknown*> g_deferredCOMReleases[DX12::CC::k_commandFrameBuffers];

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
				for (int i = 0; i < DX12::CC::k_commandFrameBuffers; i++)
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

	namespace RenderGraph
	{
		DX12::RenderPassBase*& GetHead()
		{
			static DX12::RenderPassBase* root = nullptr; //raw non-owning
			if (root) [[likely]]
				return root;

			// mo head means we rebuild the entire graph
			DX12::RenderPassBase* current = nullptr;

			root = DX12::GPassSpace::GetHeadNode();
			current = DX12::GPassSpace::GetTailNode();

			current->SetAutoBarrierSubmits(true, true);

#ifdef _MANDELBROT_TEST
			DX12::MandelbrotPass* mandelbrotPass = DX12::MandelbrotSpace::GetMandelbrotPass();
			mandelbrotPass->LinkRenderSurface(*DX12::GetMainRenderSurface());
			current = current->SetNext(*mandelbrotPass);

			return root;
			// end the graph here if testing mandelbrot
#endif

			DX12::PostProcessPass* postProcessPass = DX12::PostProcessSpace::GetPostProcessPass();
			postProcessPass->LinkInputGPass(DX12::GPassSpace::GetGPassRTPass());
			postProcessPass->LinkRenderSurface(*DX12::GetMainRenderSurface());
			current = current->SetNext(*postProcessPass);

			return root;
		}

		void WarmUp()
		{
			DX12::GPassSpace::WarmUp();
			DX12::PostProcessSpace::WarmUp();
			DX12::MandelbrotSpace::WarmUp();

			::RenderGraph::GetHead();
		}

		void Release()
		{
			DX12::GPassSpace::Release();
			DX12::PostProcessSpace::Release();
			DX12::MandelbrotSpace::Release();

			::RenderGraph::GetHead() = nullptr;
		}
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

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
		HRESULT_CHECK_CRITICAL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)), "Couldn't gather debufg controller interface.");
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE); // Catches out-of-bounds shader access

		Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
		HRESULT_CHECK_CRITICAL(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)), "Couldn't gather removed device data interface.");
		dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
#endif

	HRESULT_CHECK_CRITICAL(
		D3D12CreateDevice(DX12::GetDxgiAdapter(), ::g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device8), (void**)&device),
		"Could not create Dx12 device with selected adapter.")

	// Technically adapter interface is no longer needed
	//::COMDxgiAdapter.Reset();

	HRESULT_CHECK_FALLBACK(device->SetName(L"MainDevice"),,, "Could not set Dx12 device's name.");

#ifdef _DEBUG
	{
		Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueueInterface = nullptr;
		HRESULT_CHECK_CRITICAL(device->QueryInterface(IID_PPV_ARGS(&infoQueueInterface)), "Couldn't gather info queue interface.");
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
		Microsoft::WRL::ComPtr<ID3D12Debug6> debugInterface6 = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Debug3> debugInterface3 = nullptr;

		HRESULT_CHECK_FALLBACK(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface6)),,,
			"Failed to get debug interface v6. Will try v3...");

		if (debugInterface6)
		{
			debugInterface6->EnableDebugLayer();
		}
		else
		{
			HRESULT_CHECK_CRITICAL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface3)), "Failed to get debug interface 3.");
			debugInterface3->EnableDebugLayer();
		}

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
		if (SUCCEEDED(D3D12CreateDevice(adapter, ::g_expectedMinimumFeatureLevel, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
		else
		{
			Utils::COM::SafeRelease(adapter);
		}
	}

	FALLBACK_ASSERT(adapter,, return nullptr, "No good GPU adapter.");

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
		featureLevelsData.MaxSupportedFeatureLevel >= ::g_expectedMinimumFeatureLevel,,
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
	// Things are written lazy-init style mostly (encapsulation purposes),
	// warmup efectively makes the initialization controlled.

	HRESULT_CHECK_FALLBACK(DX12::GetRTVHeap()->GetHeap()->SetName(L"RTV Heap"), , , "Could not name RTV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetDSVHeap()->GetHeap()->SetName(L"DSV Heap"), , , "Could not name DSV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetSRVHeap()->GetHeap()->SetName(L"SRV Heap"), , , "Could not name SRV Heap.");
	HRESULT_CHECK_FALLBACK(DX12::GetUAVHeap()->GetHeap()->SetName(L"UAV Heap"), , , "Could not name UAV Heap.");

	DX12::GetMainRenderSurface(
		hwnd,
		DX12::CC::k_defaultRenderTargetFormat,
		DX12::GetMainRendererCommand());

	::RenderGraph::WarmUp();

	return true;
}

void DX12::Render(DX12::RendererCommand& rendererCommand)
{
	rendererCommand.PreRender();
	rendererCommand.UpdateSurfaceViewport();

	ID3D12GraphicsCommandList6* commandList = rendererCommand.GetCommandList();

#ifdef _DEBUG
	// HUGE ASSUMPTION!!! THE CURRENT DEFERRED CLEANUP ASSUMES ONLY ONE RENDERER COMMAND
	// USING ANOTHER ONE WILL WREAK HAVOC ONTO HELD RESOURCES
	// MOVING COM POINTER RELEASES IS SIMPLE ENOUGH: MOVE THEIR PENDING RELASE MANAGEMENT INTO THE RENDERER COMMAND OBJECT.
	// MOVING HEAP RELEASES REQUIRES A BIT MORE WORK IN HEAPS TO SOMEHOW LET THEM KNOW HANDLES BEING USED BY WHICH COMMAND.
	static DX12::RendererCommand* mainCommand = DX12::GetMainRendererCommand();
	CRITICAL_ASSERT(&rendererCommand == mainCommand, "You're about to start freeing resources which may be protected. Currently only ONE (Main) render command is supported.");
#endif

	const unsigned int currentFrame = rendererCommand.GetCurrentFrame();
	if (DX12::TestPendingResourceFreeingFlag(currentFrame))
		::DefferedFree::FreePendingResources(currentFrame);

	DX12::RenderSurface* surface = rendererCommand.GetLinkedSurface();
	CRITICAL_ASSERT(surface, "To render a renderer command, a valid surface that is linked to it is needed.")
	DX12::Math::Vector2u32 size{ surface->GetWidth(), surface->GetHeight() };
	DX12::GPassSpace::SetDimensions(size);

	DX12::PendingTransitionResourceBarrier(
		surface->GetBackbuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	::RenderGraph::GetHead()->AccummulateCommands(commandList);

	DX12::PendingTransitionResourceBarrier(
		surface->GetBackbuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	DX12::SubmitPendingBarriers(commandList);

	rendererCommand.PostRender();
}

void DX12::Render()
{
	DX12::Render(*DX12::GetMainRendererCommand());
}

void DX12::ReleaseAll()
{
	// intended to be called only once at shutdown, reinit is not possible due to static nature of things?
	static auto cleaner = []()
		{
			DX12::GetMainRendererCommand()->Flush();

			::RenderGraph::Release();
			DX12::Shaders::Release();
			
			// Render Graph passes hold textures and stuff
			::DefferedFree::FreePendingResources(DX12::CC::k_forAllFrames);

			::g_mainRenderSurface.reset();

			DX12::GetMainRendererCommand()->Release();

			// Render surface containes RTVs that hav ebeen deferred
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

void DX12::Resize()
{
	DX12::GetMainRendererCommand()->ResizeSurface();
}

#ifdef _DEBUG
void DX12::DEBUG::PrintDeviceRemovedReason(HRESULT hr)
{
	if (hr != DXGI_ERROR_DEVICE_REMOVED && hr != DXGI_ERROR_DEVICE_HUNG)
		return;
	
	ID3D12Device8* device = DX12::GetRendererDevice();
	HRESULT removeReason = device->GetDeviceRemovedReason();
	std::string output;
	output.reserve(2500);
	output.append("[REMOVAL][%x] Device Removed.\n");

	Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
	HRESULT_CHECK_CRITICAL(device->QueryInterface(IID_PPV_ARGS(&dred)), "Couldn't gather DRED.");
	
	D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbsOutput;
	if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbsOutput)))
	{
		output.append("== = DRED AUTO - BREADCRUMBS DUMP == =\n");
		const D3D12_AUTO_BREADCRUMB_NODE1* pNode = breadcrumbsOutput.pHeadAutoBreadcrumbNode;
		while (pNode != nullptr)
		{
			UINT completedOps = *pNode->pLastBreadcrumbValue;
			UINT totalOps = pNode->BreadcrumbCount;
			unsigned long long int address = (unsigned long long int)pNode->pCommandList;

			output.append(std::format("Command List: 0x{:x}\n", address));
			output.append(std::format("Completed Ops: {} / {} \n", completedOps, totalOps));
	
			if (completedOps < totalOps)
			{
				// The operation immediately following completedOps is the one that HUNG
				D3D12_AUTO_BREADCRUMB_OP failedOp = pNode->pCommandHistory[completedOps];
				output.append(std::format("LAST FAILED/HANGING OP INDEX: {} (Op Code: {})\n", completedOps, static_cast<int>(failedOp)));
			}

			pNode = pNode->pNext;
		}
	}

	D3D12_DRED_PAGE_FAULT_OUTPUT pageFaultOutput;
	if (SUCCEEDED(dred->GetPageFaultAllocationOutput(&pageFaultOutput))
		&& pageFaultOutput.PageFaultVA != 0)
	{
		output.append("== = DRED PAGE FAULT DUMP == =\n");
		output.append(std::format("Faulting GPU Virtual Address: 0x{:x}\n", pageFaultOutput.PageFaultVA));
	}
	
	CRITICAL_ASSERT(false, "%s", output.c_str());
}
#endif

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

	for (int i = 0; i < DX12::CC::k_commandFrameBuffers; i++)
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
	FALLBACK_ASSERT(_fenceEvent,,,"Could not create fence event.");

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

	CRITICAL_ASSERT(_linkedSurface, "To present the backbuffers a valid surface is needed.");
	_linkedSurface->Present();

	_issuableFenceValue++;
	_frame[_currentFrame].IssueFenceValue(_issuableFenceValue);
	_commandQueue->Signal(_fence.Get(), _issuableFenceValue);

#ifdef _DEBUG_CPU_GPU_SYNCHRONISED
	// forced synchronisation
	_frame[_currentFrame].WaitForGPU(_fenceEvent, _fence.Get());
	HRESULT deviceFailHR = DX12::GetRendererDevice()->GetDeviceRemovedReason();
	if (FAILED(deviceFailHR))
		DX12::DEBUG::PrintDeviceRemovedReason(deviceFailHR);
#endif

	_currentFrame = ++_currentFrame % DX12::CC::k_commandFrameBuffers;
}

void DX12::RendererCommand::Flush()
{
	for (int i = 0; i < DX12::CC::k_commandFrameBuffers; i++)
	{
		_frame[i].WaitForGPU(_fenceEvent, _fence.Get());
		_frame[i].Reinstate();
	}

	_issuableFenceValue = 0;
}

void DX12::RendererCommand::UpdateSurfaceViewport()
{
	CRITICAL_ASSERT(_linkedSurface, "To resize viewports, a valid surface is required to be linked.");
	_commandList->RSSetViewports(1, &_linkedSurface->GetViewport());
	_commandList->RSSetScissorRects(1, &_linkedSurface->GetScissor());
}

void DX12::RendererCommand::ResizeSurface()
{
	if (!_linkedSurface)
		return;

	Flush();

	_linkedSurface->Resize();
}

void DX12::RendererCommand::Release()
{
	if (_fence)
	{
		Flush();

		if (_fenceEvent)
		{
			CloseHandle(_fenceEvent);
			_fenceEvent = 0;
		}
	}

	_linkedSurface = nullptr;

	_fence.Reset();

	for (int i = 0; i < DX12::CC::k_commandFrameBuffers; i++)
		_frame[i].Release();

	_commandList.Reset();
	_commandQueue.Reset();

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

void DX12::RendererCommand::GfxCommandFrame::Release()
{
	_allocator.Reset();
	_fenceValue = 0;
}

void DX12::RendererCommand::GfxCommandFrame::Reinstate()
{
	_allocator->Reset();
	_fenceValue = 0;
}

void DX12::RendererCommand::GfxCommandFrame::WaitForGPU(HANDLE fenceEvent, ID3D12Fence1* fence)
{
	assert(fenceEvent && fence);

	if (fence->GetCompletedValue() >= _fenceValue) [[likely]]
		return;
	
	HRESULT_CHECK_CRITICAL(fence->SetEventOnCompletion(_fenceValue, fenceEvent), "Could not assign fence signal to event handle.");

	WaitForSingleObject(fenceEvent, INFINITE);
}
