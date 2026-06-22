//--------------------------------------------------------------------------------------
// PowerUp.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameplayObject.h"

namespace PlayFabMultiplayerRumble
{
	enum class PowerUpType
	{
		Unknown,
		DoubleLaser,
		TripleLaser,
		Rocket
	};

	class PowerUp : public GameplayObject
	{
	public:
		PowerUp();
		virtual ~PowerUp() = default;

		void Initialize();
		virtual bool OnTouch(GameplayObject* target) override;
		void Draw(float elapsedTime, RenderContext* renderContext, const TextureHandle &sprite, DirectX::XMVECTOR color);
		virtual void Draw(float elapsedTime, RenderContext* renderContext) = 0;

		virtual GameplayObjectType GetType() const override { return GameplayObjectType::PowerUp; }
		virtual PowerUpType GetPowerUpType() const { return PowerUpType::Unknown; }

		static PowerUpType ChooseNextPowerUpType();

	protected:
		// speed of the rotation of the power-up, in radians/sec
		float rotationSpeed = 2.0f;

		// amplitude of the power-up pulsation
		float pulseAmplitude = 0.1f;

		// rate of pulsation
		float pulseRate = 0.1f;

	private:
		float pulseTimer = 0.0f;
	};
}
