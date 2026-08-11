#include "DX12Shaders.h"

#include <dxcapi.h>
#include <d3d12shader.h>
#include <wrl.h>

#include <memory>
#include <filesystem>

namespace 
{
	namespace Compiler
	{
		static Microsoft::WRL::ComPtr<IDxcCompiler3> g_shaderCompiler = nullptr;
		static Microsoft::WRL::ComPtr<IDxcUtils> g_DXCUtils = nullptr;
		static Microsoft::WRL::ComPtr<IDxcIncludeHandler> g_DXCInlcudeHandler = nullptr;

		static const wchar_t* k_shaderStageNames[] = {
			L"vs_6_5",
			L"hs_6_5",
			L"ds_6_5",
			L"gs_6_5",
			L"ps_6_5",
			L"cs_6_5",
			L"as_6_5",
			L"ms_6_5"
		};
		static_assert(_countof(k_shaderStageNames) == DX12::Shaders::Stage::Count, "Number of names does not match count of shader stages.");

		IDxcBlob* CompileShader(DX12::Shaders::ShaderSpecs& spec)
		{
			if (!::Compiler::g_shaderCompiler) [[unlikely]]
				HRESULT_CHECK_CRITICAL(
					DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&::Compiler::g_shaderCompiler)),
					"Could not instantiate compiler.");

			if (!::Compiler::g_DXCUtils) [[unlikely]]
				HRESULT_CHECK_CRITICAL(
					DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&::Compiler::g_DXCUtils)),
					"Could not instantiate Dxc Utils.");

			if (!::Compiler::g_DXCInlcudeHandler) [[unlikely]]
				HRESULT_CHECK_CRITICAL(
					::Compiler::g_DXCUtils->CreateDefaultIncludeHandler(&::Compiler::g_DXCInlcudeHandler),
					"Could not instantiate Dxc Include handler.");

			std::filesystem::path path = DX12::CC::k_shaderFilePath;
			path.append("\\")
				.append(spec._filename);

			Microsoft::WRL::ComPtr<IDxcBlobEncoding> blobEncoding = nullptr;
			HRESULT_CHECK_CRITICAL(
				::Compiler::g_DXCUtils->LoadFile(path.c_str(), nullptr, &blobEncoding),
				"Could not load the shader file: %ls",
				path.c_str());

			LPCWSTR args[]
			{
				spec._filename,
				L"-E", spec._function,
				L"-T", ::Compiler::k_shaderStageNames[(int)spec._stage],
				DXC_ARG_ALL_RESOURCES_BOUND,
#ifdef _DEBUG
				DXC_ARG_DEBUG,
				DXC_ARG_SKIP_OPTIMIZATIONS,
#else
				DXC_ARGS_OPTIMIZATION_LEVEL3
#endif
				DXC_ARG_WARNINGS_ARE_ERRORS,
				L"-Qstrip_reflect",
				L"-Qstrip_debug",
			};

			DISPLAY_RAW_OUTPUT("Compiling shader: %ls", spec._filename);

			DxcBuffer buffer{};
			buffer.Encoding = DXC_CP_ACP;
			buffer.Ptr = blobEncoding->GetBufferPointer();
			buffer.Size = blobEncoding->GetBufferSize();

			Microsoft::WRL::ComPtr<IDxcResult> results = nullptr;
			HRESULT_CHECK_FALLBACK(
				::Compiler::g_shaderCompiler->Compile(&buffer, args, _countof(args), ::Compiler::g_DXCInlcudeHandler.Get(), IID_PPV_ARGS(&results)),,,
				"Could not compile the shader: %ls",
				spec._filename);

			Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors = nullptr;
			HRESULT_CHECK_FALLBACK(
				results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr),,,
				"Could not access shader compilation output logs (errors): %ls",
				spec._filename);

			CRITICAL_ASSERT(
				errors && errors->GetStringLength() == 0,
				"[ERROR] Shader compilation error: \n -- %s", errors->GetStringPointer());

			DISPLAY_RAW_OUTPUT("[COMPILED]");

			IDxcBlob* out = nullptr;
			HRESULT_CHECK_CRITICAL(
				results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&out), nullptr),
				"Could not get the compiled shader buffer for %ls",
				spec._filename);

			return out;
		}
	} // Compiler

	static std::unique_ptr<char[]> g_genericShadersRaw = nullptr;
	static std::unique_ptr<DX12::Shaders::GenericShader[]> g_genericShaderViews = nullptr;
	constexpr DX12::Shaders::ShaderSpecs k_genericShaderFiles[] = 
	{
		{L"FullscreenTri.hlsl", L"FullscreenTri", DX12::Shaders::GenericShader::Type::FullscreenTri_VS, DX12::Shaders::Stage::Vertex},
	};
	static_assert(_countof(k_genericShaderFiles) == DX12::Shaders::GenericShader::Type::Count, "Missing file specification of at least one generic shader.");

	unsigned long long int CompileAndSaveGenericShaders() 
	{
		unsigned long long int shaderHeapSize = 0;
		Microsoft::WRL::ComPtr<IDxcBlob> blobs[DX12::Shaders::GenericShader::Type::Count];

		for (int i = 0; i < DX12::Shaders::GenericShader::Count; i++)
		{
			DX12::Shaders::ShaderSpecs spec = k_genericShaderFiles[i];
			std::filesystem::path path = DX12::CC::k_shaderFilePath;
			path.append("\\")
				.append(spec._filename);

			CRITICAL_ASSERT(Utils::System::FileExists(path), "Missing generic shader file: %s", path.string().c_str());
			
			IDxcBlob* blob = ::Compiler::CompileShader(spec);
			shaderHeapSize += blob->GetBufferSize() + sizeof(DX12::Shaders::ShaderView::_k_headerSize);
			blobs[i].Attach(blob);
		}

		g_genericShadersRaw = std::make_unique<char[]>(shaderHeapSize);
		unsigned long long int offset = 0;
		for (int i = 0; i < DX12::Shaders::GenericShader::Count; i++)
		{
			unsigned long long int size = blobs[i]->GetBufferSize();
			memcpy((void*)g_genericShadersRaw[offset], &size, sizeof(DX12::Shaders::ShaderView::_k_headerSize));
			offset += DX12::Shaders::ShaderView::_k_headerSize;
			memcpy((void*)g_genericShadersRaw[offset], blobs[i]->GetBufferPointer(), size);
			offset += size;
		}

		CRITICAL_ASSERT(offset == shaderHeapSize, "Parsing shaders to raw heap was not successful.")

		Utils::System::SaveFile(DX12::CC::k_precompiledShaderFilePath, g_genericShadersRaw.get(), shaderHeapSize);

		return shaderHeapSize;
	}

	void PreloadGenericShaders()
	{
		unsigned long long int shaderHeapSize = 0;
		CRITICAL_ASSERT(!g_genericShadersRaw, "Loading new generic shaders while their heap is not empty.");
		CRITICAL_ASSERT(!g_genericShaderViews, "There are already generic shader views despite the call to make them again.");

		if (!Utils::System::FileExists(DX12::CC::k_precompiledShaderFilePath))
		{
			shaderHeapSize = ::CompileAndSaveGenericShaders();
		}
		else
		{
			unsigned long long int shaderHeapSize = Utils::System::FileSizeOf(DX12::CC::k_precompiledShaderFilePath);
			g_genericShadersRaw = std::make_unique<char[]>(shaderHeapSize);
			char* bufferProxy = g_genericShadersRaw.get();
			Utils::System::LoadFile(DX12::CC::k_precompiledShaderFilePath, bufferProxy, shaderHeapSize);
		}

		CRITICAL_ASSERT(g_genericShadersRaw && shaderHeapSize > 0, "Failed to load or compile generic shaders or allocate shader heap.");

		g_genericShaderViews = std::make_unique<DX12::Shaders::GenericShader[]>(DX12::Shaders::GenericShader::Count);

		unsigned long long int offset = 0;
		unsigned int index = 0;

		while (offset < shaderHeapSize)
		{
			g_genericShaderViews[index]._type = static_cast<DX12::Shaders::GenericShader::Type>(index);
			DX12::Shaders::ShaderView const*& shader = g_genericShaderViews[index]._shader;
			shader = reinterpret_cast<DX12::Shaders::ShaderView const*>(&g_genericShadersRaw[offset]);

			offset += shader->_size + DX12::Shaders::ShaderView::_k_headerSize;
		}

		CRITICAL_ASSERT(offset == shaderHeapSize, "Converting heap to objects led to incorrect final offset.")
	}
}

DX12::Shaders::Shader::~Shader()
{
	delete[] _byteCode;
}

void DX12::Shaders::Shader::MoveImpl(Shader&& other)
{
	CRITICAL_ASSERT(this != &other, "Trying to move shader to itself.");

	_size = other._size;
	_byteCode = other._byteCode;

	other = {};
}

void DX12::Shaders::Initialize()
{
	DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::Type::FullscreenTri_VS);
}

void DX12::Shaders::Release()
{
	g_genericShaderViews.reset();
	g_genericShadersRaw.reset();
	::Compiler::g_shaderCompiler.Reset();
	::Compiler::g_DXCUtils.Reset();
	::Compiler::g_DXCInlcudeHandler.Reset();
}

D3D12_SHADER_BYTECODE DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::Type type)
{
	if (g_genericShaderViews)
	{
		const DX12::Shaders::ShaderView const* shader = g_genericShaderViews[type]._shader;
		D3D12_SHADER_BYTECODE out;
		out.BytecodeLength = shader->_size;
		out.pShaderBytecode = &shader->_byteCode;
		return out;
	}

	::PreloadGenericShaders();

	return DX12::Shaders::GetGenericShader(type);
}

DX12::Shaders::Shader DX12::Shaders::LoadAndCompileShader(DX12::Shaders::ShaderSpecs& spec)
{
	IDxcBlob* shaderBlob = ::Compiler::CompileShader(spec);
	DX12::Shaders::Shader shader{};
	shader._size = shaderBlob->GetBufferSize();
	shader._byteCode = new char[shader._size];
	
	memcpy(shader._byteCode, shaderBlob->GetBufferPointer(), shader._size);

	Utils::COM::SafeRelease(shaderBlob);

	return shader;
}

