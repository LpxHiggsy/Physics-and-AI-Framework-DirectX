#pragma once

#include "PlaneParticleModel.h"
#include "GameObject.h"

class Plane
{
public:
	Plane(GameObject* _planeBody, vector < GameObject* > _planeWheels, vector <GameObject*> Waypoints);
	~Plane();

	virtual void Update(float t);
	void Draw(ID3D11DeviceContext* _pImmediateContext);

	// Plane Input
	void Input(float t);

	// Get Plane Body and Wheels
	GameObject* GetPlaneBody() const { return planeBody; };
	vector< GameObject* > GetPlaneWheels() const { planeWheels; };

	// Get/Set Plane Position
	XMFLOAT3 GetPlanePosition() const { return planePos; };
	void SetPlanePosition(XMFLOAT3 _planePos) { planePos = _planePos; };
	void SetPlanePosition(float x, float y, float z) { planePos.x = x, planePos.y = y, planePos.z = z; };

	// Get/Calculate Plane Forward Vector
	XMFLOAT3 GetForwardVector() const { return planeForwardVector; };
	void CalculateForwardVector();

	// Get/Set Plane Rotation
	float GetPlaneRotation() const { return planeRotation; };
	void SetPlaneRotation(float _planeRotation) { planeRotation = _planeRotation; planeRotation = planeRotation * (XM_PI / 180);
											  planeBody->GetTransform()->SetRotation(0.0f, planeRotation, 0.0f); };
	void AddPlaneRotation(float _planeRotation) { planeRotation = _planeRotation; planeRotation = planeRotation * (XM_PI / 180);
											  planeBody->GetTransform()->SetRotation(0.0f, planeRotation, 0.0f); };

	// Get/Set Plane Rotation Speed
//	float GetPlaneRotationSpeed() const { return planeRotationSpeed; };
//	void SetPlaneRotationSpeed(float _planeRotationSpeed) { planeRotationSpeed = planeRotationSpeed; };

	// Get/Set Plane Wheel Rotation
	float GetPlaneWheelRotation() const { return planeWheelRotation; };
	void SetPlaneWheelRotation(float _planeWheelRotation) { planeWheelRotation = _planeWheelRotation; };
	void AddPlaneWheelRotation(float _planeWheelRotation) { planeWheelRotation += _planeWheelRotation; };

	void Pathfinding();
	void GoToTarget();

private:
	// Plane Object
	GameObject* planeBody;
	vector < GameObject* > planeWheels;

	// Plane Properties
	XMFLOAT3 planePos;
	XMFLOAT3 planeForwardVector;
	XMFLOAT3 angularVelocity;

	float planeRotation;
	float planeRotationSpeed;
	float planeWheelRotation;

	float engineSpeedAdd;

	vector < GameObject* > waypoints;

	bool waypointReached;

	XMFLOAT3 targetPosition;
	int targetIndex;
};