#include "SnowParticle.h"
#include <iostream>

SnowParticle::SnowParticle(GameObject* Snow)
{
	snow = Snow;
	snowPos = snow->GetTransform()->GetPosition();

	angularVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

}

SnowParticle::~SnowParticle()
{

}

void SnowParticle::CalculateForwardVector()
{

}

void SnowParticle::Update(float t)
{

	SnowParticleModel* SnowBodyModel = (SnowParticleModel*)snow->GetParticleModel();

	if (snow->GetTransform()->GetPosition().y <= 5.0f)
	{
		snow->GetTransform()->SetPosition(snowPos.x, 300.0f, snowPos.z);
	}

	// Update Particle Model
		snow->GetParticleModel()->Update(t);

	// Update Transform
		snow->Update(t);

	
}

void SnowParticle::Draw(ID3D11DeviceContext* _pImmediateContext)
{
	snow->Draw(_pImmediateContext);
}