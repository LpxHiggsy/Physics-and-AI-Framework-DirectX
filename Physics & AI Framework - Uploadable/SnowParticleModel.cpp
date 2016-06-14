#include "SnowParticleModel.h"


SnowParticleModel::SnowParticleModel(Transform* transform, float mass) : ParticleModel(transform, mass)
{
	_carVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
}

SnowParticleModel::~SnowParticleModel()
{
}

void SnowParticleModel::CalculateVelocity(float t)
{
	_carVelocity.x = _carDirection.x * t;
	_carVelocity.y = _carDirection.y * t;
	_carVelocity.z = _carDirection.z * t;
}

void SnowParticleModel::Update(float t)
{
	CalculateVelocity(t);

	// Update Particle Model
	ParticleModel::Update(t);
}