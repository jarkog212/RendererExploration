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

		static const int _k_versions = 3;

		virtual DEBUG_DESTRUCTOR(Transform);

		struct AttributeType
		{
			DirectX::XMVECTOR _location{};
			DirectX::XMVECTOR _forward{};
			DirectX::XMVECTOR _up{};
			DirectX::XMVECTOR _scale{};
			int _version = 0;
		};

		AttributeType* GetValue() { return &_transform; };
		const AttributeType* GetValue() const { return &_transform; };

		virtual void PrecalculateWorld();

		const DirectX::XMMATRIX& GetWorldMatrix() const { return _worldTransform; };
		const int GetWorldMatrixVersion() const { return _thisWorldVersion; }

		void AssignParent(const Transform& parent) { _parent = &parent; };

	protected:

		const Transform* _parent = nullptr;
		AttributeType _transform{};
		DirectX::XMMATRIX _localTransform{};
		DirectX::XMMATRIX _worldTransform{};
		int _thisWorldVersion = 0;

		int _lastParentVersion = -1;
		int _lastlocalVersion = -1;
	};

	class GPUMappedTransform : public Transform 
	{
	public:

		virtual DEBUG_DESTRUCTOR(GPUMappedTransform);

		// Transform
		void PrecalculateWorld() override;
		// end Transform

	protected:

		virtual DirectX::XMMATRIX* GetTargetMatrix() = 0;

		int _lastGPUVersion = -1;
	};

	class Movable
	{
	public:

		Movable(Transform* self) : _transformToModify(self->GetValue()) {};
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
