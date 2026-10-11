#include "DX12MandelbrotPass.h"
#include "DX12CommonDescriptors.h"
#include "DX12Utils.h"
#include "DX12Shaders.h"
#include "DX12RenderSurface.h"

namespace
{
	namespace RootSignature
	{
		static Microsoft::WRL::ComPtr<ID3D12RootSignature> g_instance = nullptr;
		enum Params : unsigned int
		{
			Constants,

			P_Count
		};

		struct ShaderConstants
		{
			static constexpr unsigned int _k_count = 3;

			float _width = 0;
			float _height = 0;
			unsigned int _frame = 0;
		};
	}

	static Microsoft::WRL::ComPtr<ID3D12PipelineState> g_pipelineState = nullptr;

	static std::shared_ptr<DX12::Shaders::Shader> g_mandelBrotPixelShader = nullptr;
	static std::unique_ptr<DX12::MandelbrotPass> g_mandelbrotPass = nullptr;

	constexpr DX12::Shaders::Specs k_mandelbrotShaderSpecs{
		L"Mandelbrot",
		L"Mandelbrot",
		DX12::Shaders::Stage::Pixel,
		DX12::Shaders::GenericShader::Type::NotGeneric
	};
}

void DX12::MandelbrotSpace::WarmUp()
{
	DX12::MandelbrotSpace::GetPipelineState();
	DX12::MandelbrotSpace::GetMandelbrotPass();
	DX12::MandelbrotSpace::GetMandelbrotPixelShader();
}

void DX12::MandelbrotSpace::Release()
{
	::RootSignature::g_instance.Reset();
	::g_pipelineState.Reset();
	::g_mandelbrotPass.reset();
	::g_mandelBrotPixelShader.reset();
}

ID3D12RootSignature* DX12::MandelbrotSpace::GetRootSignature()
{
	if (::RootSignature::g_instance) [[likely]]
		return ::RootSignature::g_instance.Get();

	DX12::RootParameter params[::RootSignature::Params::P_Count]{};
	params[::RootSignature::Params::Constants].AsConstants(
		::RootSignature::ShaderConstants::_k_count,
		D3D12_SHADER_VISIBILITY_PIXEL,
		0);

	D3D12_ROOT_SIGNATURE_DESC1 desc = DX12::CC::Desc::RootSignature::k_vertexPixelTemplate;
	desc.NumParameters = ::RootSignature::Params::P_Count;
	desc.pParameters = params;

	::RootSignature::g_instance = DX12::Helpers::CreateRootSignature(desc);

	HRESULT_CHECK_CRITICAL(
		::RootSignature::g_instance->SetName(L"Mandelbrot Root Signature."),
		"Could not name Mandelbrot root signature.");

	return ::RootSignature::g_instance.Get();
}

ID3D12PipelineState* DX12::MandelbrotSpace::GetPipelineState()
{
	if (::g_pipelineState) [[likely]]
		return ::g_pipelineState.Get();

	using namespace DX12::PipelineStream;
	struct {
		SubRootSignature _rs{ DX12::MandelbrotSpace::GetRootSignature() };
		SubVertexShader _vs{ DX12::ShaderSpace::GetGenericShader(DX12::Shaders::GenericShader::Type::FullscreenTri_VS) };
		SubPixelShader _ps {DX12::MandelbrotSpace::GetMandelbrotPixelShader()->AsDX12Bytecode()};
		SubPrimitiveTopology _primitiveTopoliogy{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
		SubRenderTargetFormats _renderTargetFormats{};
		SubRasterizer _rasterizer{ DX12::CC::Desc::Rasterizer::k_noCullTemplate };
	}stream;
	stream._renderTargetFormats._sub.NumRenderTargets = 1;
	stream._renderTargetFormats._sub.RTFormats[0] = DX12::CC::k_defaultRenderTargetFormat;

	::g_pipelineState = DX12::Helpers::CreatePipelineState(&stream, sizeof(stream));

	HRESULT_CHECK_CRITICAL(
		::g_pipelineState->SetName(L"Mandelbrot PSO."),
		"Could not name Mandelbrot PSO.");

	return ::g_pipelineState.Get();
}

DX12::MandelbrotPass* DX12::MandelbrotSpace::GetMandelbrotPass()
{
	if (::g_mandelbrotPass) [[likely]]
		return ::g_mandelbrotPass.get();

	::g_mandelbrotPass = std::make_unique<DX12::MandelbrotPass>();
	return ::g_mandelbrotPass.get();
}

DX12::Shaders::Shader* DX12::MandelbrotSpace::GetMandelbrotPixelShader()
{
	if (::g_mandelBrotPixelShader) [[likely]]
		return ::g_mandelBrotPixelShader.get();

	::g_mandelBrotPixelShader = DX12::ShaderSpace::GetOrLoad(::k_mandelbrotShaderSpecs);
	return ::g_mandelBrotPixelShader.get();
}

DX12::MandelbrotPass::MandelbrotPass(MandelbrotPass&& other) noexcept :
	_renderSurface(std::exchange(other._renderSurface, nullptr))
{
}

DX12::MandelbrotPass& DX12::MandelbrotPass::operator=(MandelbrotPass&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving mandelbrot pass to itself.");

	std::swap(_renderSurface, other._renderSurface);

	return *this;
}

void DX12::MandelbrotPass::PrePassResourceBarriers()
{
	CRITICAL_ASSERT(_renderSurface, "Cannot declare post process pass's barriers without valid output surface.");

	DX12::PendingTransitionResourceBarrier(
		_renderSurface->GetBackbuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
}

void DX12::MandelbrotPass::AccummulatePassCommands(int currentFrame, ID3D12GraphicsCommandList6* commandList)
{
	CRITICAL_ASSERT(_renderSurface, "Cannot do post process pass without valid output surface.");

	commandList->SetGraphicsRootSignature(DX12::MandelbrotSpace::GetRootSignature());
	commandList->SetPipelineState(DX12::MandelbrotSpace::GetPipelineState());

	::RootSignature::ShaderConstants constants {
		(float)_renderSurface->GetWidth(),
		(float)_renderSurface->GetHeight(),
		0
	};

	commandList->SetGraphicsRoot32BitConstants(
		::RootSignature::Params::Constants,
		constants._k_count,
		&constants,
		0);

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const DX12::DescriptorHandleView surfaceRTHandle = _renderSurface->GetBackbufferHandle();
	commandList->OMSetRenderTargets(1, &surfaceRTHandle._cpu, true, nullptr);
	commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12::MandelbrotPass::PostPassResourceBarriers()
{
}
