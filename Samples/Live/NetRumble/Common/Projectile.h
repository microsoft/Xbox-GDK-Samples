//--------------------------------------------------------------------------------------
// Projectile.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameplayObject.h"

namespace NetRumble
{
	class Ship;

	class Projectile : public GameplayObject
	{
	public:
		Projectile(Ship* owner, DirectX::SimpleMath::Vector2 direction);
		virtual ~Projectile() = default;

		virtual void Update(float elapsedTime) override;
		virtual bool OnTouch(GameplayObject* target) override;
		virtual void Die(GameplayObject* source, bool cleanupOnly) override;

		virtual void Draw(float elapsedTime, RenderContext* renderContext) = 0;

		virtual GameplayObjectType GetType() const override { return GameplayObjectType::Projectile; }
		inline Ship* GetOwner() const { return m_owner; }

	protected:
		Ship* m_owner;
		float m_damageAmount = 0.0f;
		float m_damageRadius = 0.0f;
		bool m_damageOwner = true;
		float m_duration = 0.0f;
	};
}
