//--------------------------------------------------------------------------------------
// Ship.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameplayObject.h"
#include "ShipInput.h"
#include "Projectile.h"
#include "Weapon.h"
#include "BatchRemovalCollection.h"

namespace PlayFabMultiplayerRumble
{

class PlayerState;

class Ship : public GameplayObject
{
public:
    Ship();

    //virtual void Initialize() override;
    void Initialize(bool isLocal, bool useSpawnEffect = true);
    virtual void Update(float elapsedTime) override;
    virtual void Draw(float elapsedTime, RenderContext* renderContext, bool onlyDrawBody, float scale = 1.0f);
    virtual bool TakeDamage(GameplayObject* source, float damageAmount) override;
    virtual void Die(GameplayObject* source, bool cleanupOnly) override;
    void SetSafe(bool isSafe);

    virtual GameplayObjectType GetType() const override { return GameplayObjectType::Ship; }

    // Prepare the ship input data for the ShipInput packet
    std::vector<unsigned char> Serialize();

    // Get the latest ship input from the ShipInput packet
    void Deserialize(const std::vector<unsigned char> &data);

    void SetShipTexture(uint32_t index);

    // Rendering
    uint32_t Variation = 0;
    DirectX::XMVECTORF32 Color = { { { 0.000000000f, 1.000000000f, 0.000000000f, 1.000000000f } } };

    // Input
    ShipInput Input;

    // Gameplay members
    int Score = 0;
    std::shared_ptr<Weapon> PrimaryWeapon = nullptr;
    std::shared_ptr<Weapon> DroppedWeapon = nullptr;
    float Shield = 0;
    float RespawnTimer = 0;
    GameplayObject* LastDamagedBy = nullptr;

    bool IsLocal = false;

    

    BatchRemovalCollection<std::shared_ptr<Projectile>> Projectiles;
private:
    TextureHandle m_primaryTexture;
    TextureHandle m_overlayTexture;
    TextureHandle m_shieldTexture;
    
    float m_safeTimer = 0;
    float m_shieldPulseTime = 0;
    float m_shieldRechargeTimer = 0;

public:
    /// The colors used for each ship.
    static const std::array<DirectX::XMVECTORF32, 18> Colors;

    static constexpr uint32_t MaxVariations = 4;
};

}
