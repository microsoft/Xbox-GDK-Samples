//--------------------------------------------------------------------------------------
// ShipBehavior.cs
//
// The game play behavior for a ship that handles effects and bullet spawning.
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using UnityEngine;
using UnityEngine.Assertions;

public class ShipBehavior : MonoBehaviour
{
    public SpriteRenderer[] EngineFlares;
    public float MaxVelocity = 20f;

    public SpriteRenderer ShieldSprite;
    public AnimationCurve ShieldStrengthCurve;
    public GameObject ShieldImpactPrefab;

    public SpriteRenderer SpriteRenderer;
    public Rigidbody2D RigidBody;
    public AnimationCurve EngineOnOffCurve;
    public float EngineOnOffTime = 1.0f;

    public GameObject ExplosionPrefab;

    [SerializeField]
    private float _originalEngineFlareScale = 5.0f;

    [SerializeField]
    private float _engineFlareScale = 0.0f;

    [SerializeField]
    private float _engineFlareInterpolant = 0.0f;

    [SerializeField]
    private float _engineFlareDesiredScale = 0.0f;

    [SerializeField]
    private float _lastDeltaTime = 0.0f;

    private const float EPSILON = 0.001f;

    public void SetShipColor(Color shipColor)
    {
        // Set the color of the overlay
        SpriteRenderer.color = shipColor;
    }

    public void FlareEngine(bool on)
    {
        if (on)
        {
            TurnOnEngine();
        }

        else
        {
            TurnOffEngine();
        }
    }

    public void ShieldImpact(Vector2 impactpoint, Vector2 impactDirection)
    {
        // Spawn a shield impact effect where the impact happened
        var impactInstance = Instantiate(
            ShieldImpactPrefab, 
            transform.position, 
            Quaternion.FromToRotation(Vector3.right, - impactDirection),
            transform);
        var impact = impactInstance.GetComponent<ParticleSystem>();

        impact.Play();
        Destroy(impactInstance, impact.main.duration);
    }

    public void SetShieldStrength(float normalizedPercentage)
    {
        var strengthValue = ShieldStrengthCurve.Evaluate(normalizedPercentage);
        // go from white to red as the strength fades
        ShieldSprite.color = new Color(1.0f, strengthValue, strengthValue, strengthValue);
    }

    public void Explode()
    {
        // Spawn a small explosion where the laser hits an object
        var explosionInstance = Instantiate(ExplosionPrefab, gameObject.transform.position, Quaternion.identity);
        var explosion = explosionInstance.GetComponent<ParticleSystem>();

        explosion.Play();
        Destroy(explosionInstance, explosion.main.duration);
    }

    void Start()
    {
        // Don't let physics 'spin' the ship
        RigidBody.freezeRotation = true;
    }

    void Update()
    {
        UpdateEngineFlare();
    }

    private void FixedUpdate()
    {
        // Clamp the velocity of the ship
#if UNITY_6000_0_OR_NEWER
        if (RigidBody.linearVelocity.sqrMagnitude > MaxVelocity)
        {
            RigidBody.linearVelocity *= MaxVelocity / RigidBody.linearVelocity.sqrMagnitude;
        }
#else
        if (RigidBody.velocity.sqrMagnitude > MaxVelocity)
        {
            RigidBody.velocity *= MaxVelocity / RigidBody.velocity.sqrMagnitude;
        }
#endif

    }

    private void OnValidate()
    {
        Assert.IsNotNull(SpriteRenderer);
        Assert.IsNotNull(RigidBody);
        Assert.IsNotNull(ShieldSprite);
        Assert.IsNotNull(ExplosionPrefab);
    }

    private void UpdateEngineFlare()
    {
        _lastDeltaTime = Time.deltaTime;

        var invertedScaleEpsilon = 1.0f - EPSILON;
        var invertedTimeEpsilon = EngineOnOffTime - EPSILON;

        if (_engineFlareDesiredScale <= EPSILON && _engineFlareInterpolant > EPSILON)
        {
            _engineFlareInterpolant -= _lastDeltaTime;
            if (_engineFlareInterpolant <= EPSILON)
            {
                _engineFlareInterpolant = 0.0f;
            }
        }
        else if (_engineFlareDesiredScale >= invertedScaleEpsilon && _engineFlareInterpolant < invertedTimeEpsilon)
        {
            _engineFlareInterpolant += _lastDeltaTime;
            if (_engineFlareInterpolant >= invertedTimeEpsilon)
            {
                _engineFlareInterpolant = EngineOnOffTime;
            }
        }

        _engineFlareScale = EngineOnOffCurve.Evaluate(_engineFlareInterpolant);

        foreach (var EngineFlare in EngineFlares)
        {
            EngineFlare.transform.localScale = new Vector3(
                _originalEngineFlareScale * _engineFlareScale,
                _originalEngineFlareScale * _engineFlareScale,
                1.0f);
            EngineFlare.color = new Color(
                EngineFlare.color.r, EngineFlare.color.g, EngineFlare.color.b,
                _engineFlareScale);
        }
    }

    private void TurnOffEngine()
    {
        _engineFlareDesiredScale = 0.0f;
    }

    private void TurnOnEngine()
    {
        _engineFlareDesiredScale = 1.0f;
    }
}
