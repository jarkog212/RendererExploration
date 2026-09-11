#pragma once

#include "Utils.h"
#include "Attributes.h"
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
		int _id;
		DX12::Projection::Type _type;
		DirectX::XMVECTOR _up;
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

	class Camera : public AttributeHolder<Camera>, 
		public ATransform,
		public AMovable,
		public AIUpdatable
	{
	public:

		Camera(const CameraSpecs& specs) : AMovable((ATransform*)this), _specs(specs) { Update(0.f); };
		DEBUG_DESTRUCTOR(Camera);

		constexpr DirectX::XMMATRIX GetView() const { return _view; }
		constexpr DirectX::XMMATRIX GetProjection() const { return _projection; }
		constexpr DirectX::XMMATRIX GetInvProjection() const { return _invProjection; }
		constexpr DirectX::XMMATRIX GetViewProjection() const { return _viewProjection; }
		constexpr DirectX::XMMATRIX GetInvViewProjection() const { return _invViewProjection; }
		constexpr DirectX::XMVECTOR GetUp() const { return _specs._up; }
		constexpr float GetNearZ() const { return _specs._nearZ; }
		constexpr float GetFarZ() const { return _specs._farZ; }
		constexpr float GetFov() const { return _specs._fov; }
		constexpr float GetAspectRatio() const { return _specs._aspectRatio; }
		constexpr float GetViewWidth() const { return _specs._viewWidth; }
		constexpr float GetViewHeight() const { return _specs._viewHeight; }
		DX12::Projection::Type GetType() const { _specs._type; }

		void SetUp(DirectX::XMVECTOR& up);
		void SetNearZ(float nearZ);
		void SetFarZ(float farZ);
		void SetFov(float fov);
		void SetAspectRatio(float aspectRatio);
		void SetViewWidth(float viewWidth);
		void SetViewHeight(float viewHeight);

		// AIUpdatable
		void Update(float deltaTime) override;

	private:

		CameraSpecs _specs = {};
		DirectX::XMMATRIX _view = {};
		DirectX::XMMATRIX _projection = {};
		DirectX::XMMATRIX _invProjection = {};
		DirectX::XMMATRIX _viewProjection = {};
		DirectX::XMMATRIX _invViewProjection = {};
		bool _dirty = false;
	};
}