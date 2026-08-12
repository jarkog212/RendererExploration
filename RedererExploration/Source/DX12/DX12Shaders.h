#pragma once

#include "DX12CommonConstants.h"
#include "Utils.h"

#pragma comment(lib, "dxcompiler.lib")

namespace DX12::Shaders
{
	enum Stage 
	{
		Vertex = 0,
		Hull,
		Domain,
		Geometry,
		Pixel,
		Amplification,
		Mesh,
		Compute,

		Count
	};

	struct ShaderView
	{
		static constexpr unsigned int _k_headerSize = sizeof(unsigned long long int);

		unsigned long long int _size = 0;
		char* _byteCode = nullptr;

		D3D12_SHADER_BYTECODE AsDX12Bytecode() const;
	};

	struct Shader : public ShaderView 
	{
		Shader() = default;
		~Shader() { Release(); };
		Shader(Shader&& other) noexcept;
		Shader& operator=(Shader&& other);

		NO_COPY(Shader);

		void Release();

		const DX12::Shaders::ShaderView CreateView() const;		
	};

	struct GenericShader
	{
		enum Type : unsigned int
		{
			FullscreenTri_VS = 0,
			FillColor_PS,

			Count, 
		}_type;

		DX12::Shaders::ShaderView const* _shader = nullptr;
	};

	struct ShaderSpecs
	{
		const wchar_t* _filename;
		const wchar_t* _function;
		DX12::Shaders::GenericShader::Type _type;
		DX12::Shaders::Stage _stage;
	};

	void Initialize();
	void Release();

	D3D12_SHADER_BYTECODE GetGenericShader(DX12::Shaders::GenericShader::Type type);
	const DX12::Shaders::ShaderView LoadOrCompile(const DX12::Shaders::ShaderSpecs& specs);
	bool TryReleaseShader(const DX12::Shaders::ShaderSpecs& specs);
	D3D12_SHADER_BYTECODE GetShader(const DX12::Shaders::ShaderSpecs& specs);
	const DX12::Shaders::ShaderView GetShaderView(const DX12::Shaders::ShaderSpecs& specs);
}