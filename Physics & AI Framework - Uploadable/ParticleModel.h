#pragma once

#include <directxmath.h>
#include <d3d11_1.h>

#include "Transform.h"

using namespace DirectX;
using namespace std;

class ParticleModel
{
public:
	ParticleModel(Transform* transform, float mass);
	~ParticleModel();

	// --------------- Movement ----------------

	void Move(float amountX, float amountY, float amountZ);
	void MoveConstVel(float t);
	void MoveConstAcc(float t);
	
	// -------------- Drag Force ---------------- //

	void DragForce();
	void DragLamFlow();
	void MotionInFluid(float t);
	void TurbulantFlow();

	// --------------- Sliding ------------------ //

	void SlidingMotion();
	void SlidingForce(float theta, float frCoef);

	// ------------ Collision Check ------------- //

	void BaseCollisionCheck(XMFLOAT3 floorPos);
	bool CollisionCheck(XMFLOAT3 object2Pos, float object2Radius);
	void ResolveCollision(ParticleModel* object2);

	// --------------- Updates ------------------ //

	void UpdateNetForce();
	void UpdateAccel();
	void UpdateState();
	virtual void Update(float t);

	// --------------- Get/Set ------------------ //

	// Get/Set Collision Radius
	float GetCollisionRadius() const { return _collisionRadius; }
	void SetCollisionRadius(float collisionRadius) { _collisionRadius = collisionRadius; }

	// Get/Set Mass
	float GetMass() const { return _mass; };
	void SetMass(float mass) { _mass = mass; }

	// Get/Set Velocity
	XMFLOAT3 GetVelocity() const { return _velocity; }
	void SetVelocity(XMFLOAT3 velocity) { _velocity = velocity; }

	void SetTheta(float theta) { _theta = theta; }

protected:
	Transform* _transform;

private:
	// Movement
	XMFLOAT3 _acceleration;
	XMFLOAT3 _velocity;

	XMFLOAT3 _constAccel;

	bool _useConstAcc;

	// Forces
	XMFLOAT3 _netForce;
	float _forceMag;

	// Drag Force
	XMFLOAT3 _dragForce;
	float _dragFactor;
	bool _laminar;

	// Sliding
	XMFLOAT3 _slidingForce;
	bool _slidingOn;

	// Collision Check
	float _collisionRadius;

	// Other
	static const float _gravity;
	float _mass;

	float _theta;
};