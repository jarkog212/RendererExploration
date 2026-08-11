#include "DX12Shaders.h"

#include <dxcapi.h>
#include <d3d12shader.h>
#include <wrl.h>

#include <memory>
#include <filesystem>
#include <unordered_map>

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

		IDxcBlob* CompileShader(const DX12::Shaders::ShaderSpecs& spec)
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

			std::filesystem::path filename = spec._filename;
			filename += DX12::CC::k_shaderSourceExtension;

			std::filesystem::path path = DX12::CC::k_shaderFilePrefix;
			path.append(filename.c_str());

			Microsoft::WRL::ComPtr<IDxcBlobEncoding> blobEncoding = nullptr;
			HRESULT_CHECK_CRITICAL(
				::Compiler::g_DXCUtils->LoadFile(path.c_str(), nullptr, &blobEncoding),
				"Could not load the shader file: %ls",
				path.c_str());

			LPCWSTR args[]
			{
				filename.c_str(),
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

	static std::unique_ptr<char[]> g_genericShadersArena = nullptr;
	static std::unique_ptr<DX12::Shaders::GenericShader[]> g_genericShaderViews = nullptr;
	constexpr DX12::Shaders::ShaderSpecs k_genericShaderSpecs[] = 
	{
		{L"FullscreenTri", L"FullscreenTri", DX12::Shaders::GenericShader::Type::FullscreenTri_VS, DX12::Shaders::Stage::Vertex},
		{L"FillColor", L"FillColor", DX12::Shaders::GenericShader::Type::FillColor_PS, DX12::Shaders::Stage::Pixel},
	};
	static_assert(_countof(k_genericShaderSpecs) == DX12::Shaders::GenericShader::Type::Count, "Missing file specification of at least one generic shader.");

	static std::unordered_map<const wchar_t* ,DX12::Shaders::Shader> g_shaders;

	unsigned long long int CompileAndSaveGenericShaders() 
	{
		unsigned long long int shaderHeapSize = 0;
		Microsoft::WRL::ComPtr<IDxcBlob> blobs[DX12::Shaders::GenericShader::Type::Count];

		for (int i = 0; i < DX12::Shaders::GenericShader::Count; i++)
		{	
			IDxcBlob* blob = ::Compiler::CompileShader(k_genericShaderSpecs[i]);
			shaderHeapSize += blob->GetBufferSize() + DX12::Shaders::ShaderView::_k_headerSize;
			blobs[i].Attach(blob);
		}

		g_genericShadersArena = std::make_unique<char[]>(shaderHeapSize);
		unsigned long long int offset = 0;
		for (int i = 0; i < DX12::Shaders::GenericShader::Count; i++)
		{
			unsigned long long int size = blobs[i]->GetBufferSize();
			memcpy((void*)(&g_genericShadersArena[offset]), &size, DX12::Shaders::ShaderView::_k_headerSize);
			offset += DX12::Shaders::ShaderView::_k_headerSize;
			memcpy((void*)(&g_genericShadersArena[offset]), blobs[i]->GetBufferPointer(), size);
			offset += size;
		}

		CRITICAL_ASSERT(offset == shaderHeapSize, "Parsing shaders to raw heap was not successful.")

		Utils::System::SaveFile(DX12::CC::k_precompiledGenericShaderFilePath, g_genericShadersArena.get(), shaderHeapSize);

		return shaderHeapSize;
	}

	void PreloadGenericShaders()
	{
		unsigned long long int shaderHeapSize = 0;
		CRITICAL_ASSERT(!g_genericShadersArena, "Loading new generic shaders while their heap is not empty.");
		CRITICAL_ASSERT(!g_genericShaderViews, "There are already generic shader views despite the call to make them again.");

		if (!Utils::System::FileExists(DX12::CC::k_precompiledGenericShaderFilePath))
		{
			shaderHeapSize = ::CompileAndSaveGenericShaders();
		}
		else
		{
			shaderHeapSize = Utils::System::FileSizeOf(DX12::CC::k_precompiledGenericShaderFilePath);
			g_genericShadersArena = std::make_unique<char[]>(shaderHeapSize);
			char* bufferProxy = g_genericShadersArena.get();
			Utils::System::LoadFile(DX12::CC::k_precompiledGenericShaderFilePath, bufferProxy, shaderHeapSize);
		}

		CRITICAL_ASSERT(g_genericShadersArena && shaderHeapSize > 0, "Failed to load or compile generic shaders or allocate shader heap.");

		g_genericShaderViews = std::make_unique<DX12::Shaders::GenericShader[]>(DX12::Shaders::GenericShader::Count);

		unsigned long long int offset = 0;
		unsigned int index = 0;

		while (offset < shaderHeapSize)
		{
			g_genericShaderViews[index]._type = static_cast<DX12::Shaders::GenericShader::Type>(index);
			DX12::Shaders::ShaderView const*& shader = g_genericShaderViews[index]._shader;
			shader = reinterpret_cast<DX12::Shaders::ShaderView const*>(&g_genericShadersArena[offset]);

			offset += shader->_size + DX12::Shaders::ShaderView::_k_headerSize;
		}

		CRITICAL_ASSERT(offset == shaderHeapSize, "Converting heap to objects led to incorrect final offset.")
	}
}

DX12::Shaders::Shader::~Shader()
{
	delete[] _byteCode;
}

D3D12_SHADER_BYTECODE DX12::Shaders::ShaderView::AsDX12Bytecode() const
{
	D3D12_SHADER_BYTECODE out;
	out.BytecodeLength = _size;
	out.pShaderBytecode = &_byteCode;
	return out;
}

const DX12::Shaders::ShaderView DX12::Shaders::Shader::CreateView() const
{
	DX12::Shaders::ShaderView out;
	memcpy(&out, this, sizeof(this));
	return out;
}

DX12::Shaders::Shader& DX12::Shaders::Shader::MoveImpl(Shader&& other)
{
	CRITICAL_ASSERT(this != &other, "Trying to move shader to itself.");

	_size = other._size;
	_byteCode = other._byteCode;

	other = {};

	return *this;
}

void DX12::Shaders::Initialize()
{
	DX12::Shaders::GetGenericShader(DX12::Shaders::GenericShader::Type::FullscreenTri_VS);
}

void DX12::Shaders::Release()
{
	::g_genericShaderViews.reset();
	::g_genericShadersArena.reset();
	::g_shaders.clear();
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

const DX12::Shaders::ShaderView DX12::Shaders::LoadOrCompile(const DX12::Shaders::ShaderSpecs& specs)
{
	DX12::Shaders::TryReleaseShader(specs);
	DX12::Shaders::Shader& out = ::g_shaders[specs._filename];
	std::filesystem::path precompiledPath = DX12::CC::k_precompiledShaderPrefix;
	precompiledPath.append(specs._filename);
	precompiledPath += DX12::CC::k_shaderBinaryExtension;

	if (Utils::System::FileExists(precompiledPath))
	{
		Utils::System::LoadFile(precompiledPath, out._byteCode, out._size);
		return out.CreateView();
	}

	{
		Microsoft::WRL::ComPtr<IDxcBlob> blob = ::Compiler::CompileShader(specs);
		out._size = blob->GetBufferSize();
		out._byteCode = new char[out._size];
		memcpy(out._byteCode, blob->GetBufferPointer(), out._size);
	}

	Utils::System::SaveFile(precompiledPath, out._byteCode, out._size);

	return out.CreateView();
}

bool DX12::Shaders::TryReleaseShader(const DX12::Shaders::ShaderSpecs& specs)
{
	if (auto it = ::g_shaders.find(specs._filename); it != ::g_shaders.end())
	{
		::g_shaders.erase(it);
		return true;
	}

	return false;
}

D3D12_SHADER_BYTECODE DX12::Shaders::GetShader(const DX12::Shaders::ShaderSpecs& specs)
{
	if (auto it = ::g_shaders.find(specs._filename); it != ::g_shaders.end())
		return it->second.AsDX12Bytecode();

	DX12::Shaders::ShaderView view = DX12::Shaders::LoadOrCompile(specs);
	return view.AsDX12Bytecode();
}

const DX12::Shaders::ShaderView DX12::Shaders::GetShaderView(const DX12::Shaders::ShaderSpecs& specs)
{
	if (auto it = ::g_shaders.find(specs._filename); it != ::g_shaders.end())
		return it->second.CreateView();

	return DX12::Shaders::LoadOrCompile(specs);
}
