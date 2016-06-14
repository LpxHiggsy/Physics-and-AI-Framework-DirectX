#pragma once

#include "SnowParticleModel.h"
#include "GameObject.h"

class SnowParticle
{
public:
	SnowParticle( GameObject* Snow);
	~SnowParticle();

	virtual void Update(float t);
	void Draw(ID3D11DeviceContext* _pImmediateContext);

	GameObject* GetSnowParticles() const { return snow; };

	// Get/Set Snow Position
	XMFLOAT3 GetSnowPosition() const { return snowPos; };
	void SetSnowPosition(XMFLOAT3 _planePos) { snowPos = _planePos; };
	void SetSnowPosition(float x, float y, float z) { snowPos.x = x, snowPos.y = y, snowPos.z = z; };

	// Get/Calculate Snow Forward Vector
	XMFLOAT3 GetForwardVector() const { return planeForwardVector; };
	void CalculateForwardVector();

private:
	// snow Object
	GameObject* snow;

	// snow Properties
	XMFLOAT3 snowPos;
	XMFLOAT3 planeForwardVector;
	XMFLOAT3 angularVelocity;


};