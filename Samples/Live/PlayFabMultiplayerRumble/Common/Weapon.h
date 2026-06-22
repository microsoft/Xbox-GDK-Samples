//--------------------------------------------------------------------------------------
// Weapon.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace PlayFabMultiplayerRumble
{
	class Ship;

	enum class WeaponType
	{
		Unknown,
		Laser,
		DoubleLaser,
		TripleLaser,
		Rocket,
		Mine
	};

	class Weapon
	{
	public:
		Weapon(Ship* owner);
		virtual ~Weapon() = default;

		virtual void Update(float elapsedTime);
		virtual void Fire(const DirectX::SimpleMath::Vector2 &direction);
		virtual void CreateProjectiles(const DirectX::SimpleMath::Vector2 &direction) = 0;
		virtual WeaponType GetWeaponType() const { return WeaponType::Unknown; }

	protected:
		Ship* m_owner;
		float m_timeToNextFire = 0.0f;
		float m_fireDelay = 0.0f;

		std::wstring m_fireSoundEffect;
	};
}
