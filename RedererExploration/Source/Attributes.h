#pragma once
#include "DirectXMath.h"

namespace Attrib
{
	template <typename D>
	class Holder
	{
	public:

		virtual ~Holder() { DEBUG_DESTRUCTOR_BODY(Holder); }

		template<typename T>
		typename T::AttributeType* GetAttribute()
		{
			T* asType = dynamic_cast<T*>(static_cast<D*>(this));
			return asType ? asType->GetValue() : nullptr;
		}

		template<typename T>
		const typename T::AttributeType* GetAttribute() const
		{
			T* asType = dynamic_cast<T*>(static_cast<D*>(this));
			return asType ? asType->GetValue() : nullptr;
		}
	};

	class Transform
	{
	public:

		virtual ~Transform() { DEBUG_DESTRUCTOR_BODY(Transform); }

		struct AttributeType
		{
			DirectX::XMVECTOR _location{};
			DirectX::XMVECTOR _forward{};
			DirectX::XMVECTOR _up{};
			DirectX::XMVECTOR _scale{};
		};

		AttributeType* GetValue() { return &_transform; };
		const AttributeType* GetValue() const { return &_transform; };

	protected:

		AttributeType _transform;
	};

	class Movable
	{
	public:

		Movable(Transform* self) { _transformToModify = self->GetValue(); }
		virtual ~Movable() { DEBUG_DESTRUCTOR_BODY(Movable); }

		inline void Reset()
		{
			_relativeTranslate = {};
			_worldTranslate = {};
			_rotate = {};
		}

		inline void Apply()
		{
			Attrib::Transform::AttributeType transform = *_transformToModify;

			// world translate
			if (_worldTranslate.m128_f32[0] || _worldTranslate.m128_f32[1] || _worldTranslate.m128_f32[2])
				transform._location = DirectX::XMVectorAdd(transform._location, _worldTranslate);

			// rotation
			if (_rotate.x != 0.f || _rotate.y != 0.f || _rotate.z != 0.f)
			{
				// Roll = about Forward = Z Axis, Pitch = about Right = X Axis, Yaw = about Up = Y Axis
				DirectX::XMVECTOR quat = DirectX::XMQuaternionRotationRollPitchYaw(_rotate.z, _rotate.x, _rotate.y);
				transform._forward = DirectX::XMVector3Rotate(transform._forward, quat);
				transform._up = DirectX::XMVector3Rotate(transform._up, quat);
			}

			// relative translate
			if (const float forward = _relativeTranslate.m128_f32[2]; forward != 0.f)
			{
				transform._location = DirectX::XMVectorMultiplyAdd(
					transform._forward,
					DirectX::XMVECTOR(forward, forward, forward),
					transform._location);
			}

			if (const float right = _relativeTranslate.m128_f32[0]; right != 0.f)
			{
				const DirectX::XMVECTOR rightAxis = DirectX::XMVector3Cross(transform._up, transform._forward);
				transform._location = DirectX::XMVectorMultiplyAdd(
					rightAxis,
					DirectX::XMVECTOR(right, right, right),
					transform._location);
			}

			if (const float up = _relativeTranslate.m128_f32[1]; up != 0.f)
			{
				transform._location = DirectX::XMVectorMultiplyAdd(
					transform._up,
					DirectX::XMVECTOR(up, up, up),
					transform._location);
			}

			*_transformToModify = transform;
			Reset();
		};

		inline void Forward(float value) { _relativeTranslate.m128_f32[2] = value; };
		inline void Right(float value) { _relativeTranslate.m128_f32[0] = value; };
		inline void Up(float value) { _relativeTranslate.m128_f32[1] = value; };
		inline void Turn(DirectX::XMFLOAT3 value) { _rotate = value; };

		using AttributeType = Movable;

		AttributeType* GetValue() { return this; };
		const AttributeType* GetValue() const { return this; };

	protected:

		DirectX::XMVECTOR _relativeTranslate{};
		DirectX::XMVECTOR _worldTranslate{};
		DirectX::XMFLOAT3 _rotate{};
		Transform::AttributeType* _transformToModify = nullptr;
	};

	class IUpdatable
	{
	public:

		virtual ~IUpdatable() { DEBUG_DESTRUCTOR_BODY(AUpdatable); }

		using AttributeType = IUpdatable;

		AttributeType* GetValue() { return this; };
		const AttributeType* GetValue() const { return this; };

		virtual void Update(float deltaTime) = 0;
	};
}
