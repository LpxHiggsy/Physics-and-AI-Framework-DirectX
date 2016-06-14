#pragma once

#include "ParticleModel.h"

class SnowParticleModel : public ParticleModel
{
public:
	SnowParticleModel(Transform* transform, float mass);
	~SnowParticleModel();

	void CalculateVelocity(float t);

	void Update(float t);

	// --------------- Get/Set ------------------ //

	// Get/Set Car Direction
	XMFLOAT3 GetSnowDirection() const { return _carDirection; }
	void SetSnowDirection(XMFLOAT3 carDirection) { _carDirection = carDirection; }

	// Get/Set Car Velocity
	XMFLOAT3 GetSnowVelocity() const { return _carVelocity; }

private:

	float _thrust;

	XMFLOAT3 _carVelocity;
	XMFLOAT3 _carDirection;
};

