#pragma once

#include "DX12CommonConstants.h"
#include "Utils.h"

#pragma comment(lib, "dxcompiler.lib")

namespace DX12::Shaders
{
	enum class Stage
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

		D3D12_SHADER_BYTECODE AsDX12Bytecode() const;

		unsigned long long int _size = 0;
		char* _byteCode = nullptr;
	};

	struct GenericShader
	{
		enum class Type : unsigned int
		{
			FullscreenTri_VS = 0,
			FillColor_PS,
			PostProcess_PS,

			Count,
			NotGeneric
		}_type;

		DX12::Shaders::ShaderView _shader = {};
	};

	struct Specs
	{
		const wchar_t* _filename;
		const wchar_t* _function;
		DX12::Shaders::Stage _stage;
		DX12::Shaders::GenericShader::Type _type;
	};

	struct Shader : public ShaderView
	{
		Shader(const DX12::Shaders::Specs& specs);
		~Shader();
		Shader(Shader&& other) noexcept;
		Shader& operator=(Shader&& other);

		NO_COPY(Shader);

		const DX12::Shaders::ShaderView CreateView() const { return { _size, _byteCode }; };

		std::wstring _name;
	};
}

namespace DX12::ShaderSpace
{
	void WarmUp();
	void Release();

	D3D12_SHADER_BYTECODE GetGenericShader(DX12::Shaders::GenericShader::Type type);
	std::shared_ptr<DX12::Shaders::Shader> GetOrLoad(const DX12::Shaders::Specs& specs);
}