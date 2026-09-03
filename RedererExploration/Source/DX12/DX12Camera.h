#pragma once

#include "DirectXMath.h"

namespace DX12::Projection
{
	enum class Type
	{
		Perspective = 0,
		Orthogonal
	};

	struct CameraSpecs
	{
		unsigned int _id;
		DX12::Projection::Type _type;
		DirectX::XMFLOAT3 _up;
		union 
		{
			float _fov;
			float _viewWidth;
		};
		union 
		{
			float _aspectRatio;
			float _viewHeight;
		};
		float _nearZ;
		float _farZ;
	};

	static const CameraSpecs k_PerspectiveCameraSpecsTemplate = {
		-1, // _id
		DX12::Projection::Type::Perspective,
		{0.f, 1.f, 0.f}, //_up
		{._fov{0.25f}},
		{._aspectRatio{16.f / 9.f}},
		0.001f, // _nearZ
		10000.f // _farZ
	};

	static const CameraSpecs k_OrthogonalCameraSpecsTemplate = {
		-1, // _id
		DX12::Projection::Type::Orthogonal,
		{0.f, 1.f, 0.f}, //_up
		{._viewWidth{1920.f}},
		{._viewHeight{1080.f}},
		0.001f, // _nearZ
		10000.f // _farZ
	};

	class Camera
	{
	public:

		Camera(const CameraSpecs& specs) : _specs(specs) {};

		DirectX::XMMATRIX View() const;
		DirectX::XMMATRIX Projection() const;
		DirectX::XMMATRIX InvProjection() const;
		DirectX::XMMATRIX ViewProjection() const;
		DirectX::XMMATRIX InvViewProjection() const;
		DirectX::XMFLOAT3 Up() const;
		float NearZ() const { return _specs._nearZ; };
		float& NearZ() { return _specs._nearZ; }
		float FarZ() const;
		float& FarZ();
		float Fov() const;
		float& Fov();
		float AspectRatio() const;
		float& AspectRatio();
		float ViewWidth() const;
		float& ViewWidth();
		float ViewHeight() const;
		float& ViewHeight();
		DX12::Projection::Type Type() const;
		DX12::Projection::Type& Type();

	private:

		CameraSpecs _specs;
	};
}