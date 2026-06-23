//--------------------------------------------------------------------------------------
// TurretBehavior.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// For licensing details, please refer to the LICENSE.md file associated with this
// source repository (found within the root folder).
//--------------------------------------------------------------------------------------

using UnityEngine;
using UnityEngine.Assertions;

public class TurretBehavior : MonoBehaviour
{
    private const int PROJECTILE_COLLISION_LAYER = 9;

    public Color ShipColor;
    public GameObject Projectile;
    public GameObject Target;
    public GameObject ExplosionPrefab;
    public GameObject ExplosionSmokePrefab;
    public AudioSource ExplosionSoundPrefab;
    public SpriteRenderer SpriteRenderer;

    public float FireRate = 0.8f;
    public float ProjectileSpeed = 500f;
    public float ProjectileScale = 2.0f;
    public float FireRadius = 10.0f;
    public int HitPoints = 100;
    public int DamageAmountFromProjectiles = 10;

    private float _timeToNextFire = 0.0f;

    void Start()
    {
        // Set the color of the overlay
        SpriteRenderer.color = ShipColor;
    }

    private void OnValidate()
    {
        Assert.IsNotNull(SpriteRenderer);
    }

    private void OnCollisionEnter2D(Collision2D collision)
    {
        // ignore friendly fire
        if (collision.transform.tag == transform.tag)
        {
            return;
        }

        // take damage if the collision was with a projectile
        if (PROJECTILE_COLLISION_LAYER == collision.gameObject.layer)
        {
            TakeDamage(DamageAmountFromProjectiles);
        }
    }

    public void TakeDamage(int amount)
    {
        HitPoints -= amount;

        if (HitPoints <= 0)
        {
            // Create the explosion
            var explosionInstance = Instantiate(ExplosionPrefab, gameObject.transform.position, Quaternion.identity);
            var explosion = explosionInstance.GetComponent<ParticleSystem>();

            explosion.Play();

            var smokeInstance = Instantiate(ExplosionSmokePrefab, gameObject.transform.position, Quaternion.identity);
            var smoke = smokeInstance.GetComponent<ParticleSystem>();

            smoke.Play();

            // Play explosion sound
            var soundeffect = Instantiate(ExplosionSoundPrefab, gameObject.transform.position, Quaternion.identity);

            // Clean up
            Destroy(soundeffect, soundeffect.clip.length);
            Destroy(explosionInstance, explosion.main.duration);
            Destroy(smokeInstance, smoke.main.duration);
            Destroy(gameObject);
        }
    }

    void Update()
    {
        if (Target == null)
        {
            return;
        }

        // Rotate the turret to face the player
        Vector3 diff = Target.transform.position - transform.position;

        transform.rotation = Quaternion.Euler(
            0f,
            0f,
            (Mathf.Atan2(diff.y, diff.x) - 1.5f) * Mathf.Rad2Deg
            );

        // Update firing timer
        _timeToNextFire = Mathf.Max(_timeToNextFire - Time.deltaTime, 0f);

        // Check how far away the player is
        float distance = Vector3.Distance(
            transform.position,
            Target.transform.position
            );

        // If they're close enough, start firing!
        if (distance < FireRadius)
        {
            Fire(Target.transform.position);
        }
    }

    public void Fire(Vector3 target)
    {
        // Enforce the max fire rate
        if (_timeToNextFire > 0.0f)
        {
            return;
        }

        _timeToNextFire = FireRate;

        // Create a new laser bolt
        GameObject bullet = Instantiate(
            Projectile,
            transform.position + transform.up * 2f,
            transform.rotation
        );

        // Set the scale, match velocity and set the force
        bullet.transform.localScale *= ProjectileScale;

#if UNITY_6000_0_OR_NEWER
        bullet.GetComponent<Rigidbody2D>().linearVelocity = GetComponent<Rigidbody2D>().linearVelocity;
#else
        bullet.GetComponent<Rigidbody2D>().velocity = GetComponent<Rigidbody2D>().velocity;
#endif
        bullet.GetComponent<Rigidbody2D>().AddForce(transform.up * ProjectileSpeed);

        // Don't let the laser collide with us
        Physics2D.IgnoreCollision(
            GetComponent<Collider2D>(),
            bullet.GetComponent<Collider2D>()
        );

        // Pew pew
        GetComponent<AudioSource>().Play();
    }
}
