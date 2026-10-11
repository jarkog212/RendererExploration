#pragma once
#include "DirectXMath.h"
#include "Utils.h"

namespace DX12::Attributes
{
	template <typename D>
	class Holder
	{
	public:

		virtual DEBUG_DESTRUCTOR(Holder);

		template<typename T>
		typename T::AttributeType* GetAttribute()
		{
			T* asType = dynamic_cast<T*>(static_cast<D*>(this));
			return asType ? asType->GetValue() : nullptr;
		}

		template<typename T>
		const typename T::AttributeType* GetAttribute() const
		{
			const T* asType = dynamic_cast<const T*>(static_cast<const D*>(this));
			return asType ? asType->GetValue() : nullptr;
		}
	};

	class Transform
	{
	public:

		virtual DEBUG_DESTRUCTOR(Transform);

		struct AttributeType
		{
			DirectX::XMVECTOR _location{};
			DirectX::XMVECTOR _forward{};
			DirectX::XMVECTOR _up{};
			DirectX::XMVECTOR _scale{};
		};

		AttributeType* GetValue() { return &_transform; };
		const AttributeType* GetValue() const { return &_transform; };

		virtual void PrecalculateWorld();

	protected:

		AttributeType _transform;
		DirectX::XMMATRIX _worldTransform;
	};

	class GPUMappedTransform : public Transform 
	{
	public:

		virtual DEBUG_DESTRUCTOR(GPUMappedTransform);

		void LinkToGPUArena(void* Key);

		// Transform
		void PrecalculateWorld() override;
		// end Transform

	protected:

		DirectX::XMMATRIX* _GPUArenaTransformOffset = nullptr;
		void* _GPUArenaKey = nullptr;
	};

	class Movable
	{
	public:

		Movable(Transform* self) { _transformToModify = self->GetValue(); }
		virtual DEBUG_DESTRUCTOR(Movable);

		void Reset();

		void Apply();
		
		inline void Forward(float value) { _relativeTranslate.m128_f32[2] += value; }
		inline void Right(float value) { _relativeTranslate.m128_f32[0] += value; }
		inline void Up(float value) { _relativeTranslate.m128_f32[1] += value; }
		inline void AddX(float value) { _worldTranslate.m128_u8[0] += value; }
		inline void AddY(float value) { _worldTranslate.m128_u8[1] += value; }
		inline void AddZ(float value) { _worldTranslate.m128_u8[2] += value; }
		inline void Turn(DirectX::XMFLOAT3 value) { _rotate.x += value.x; _rotate.y += value.y; _rotate.z = value.z; }

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
