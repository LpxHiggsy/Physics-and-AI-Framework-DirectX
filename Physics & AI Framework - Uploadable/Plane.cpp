#include "Plane.h"
#include <iostream>

Plane::Plane(GameObject* _planeBody, vector < GameObject* > _planeWheels, vector <GameObject*> Waypoints)
{
	planeBody = _planeBody;
	planeWheels = _planeWheels;

	planePos = planeBody->GetTransform()->GetPosition();

	planeRotation = -50.0f;
	planeRotationSpeed = 0.001f;
	planeWheelRotation = 0.0f;

	engineSpeedAdd = 0.0008f;
	
	waypoints = Waypoints;
	targetPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	targetIndex = 0;

	angularVelocity = XMFLOAT3(0.0f, 0.0f, 0.0f);

}

Plane::~Plane()
{

}

void Plane::Input(float t)
{
	PlaneParticleModel* planeBodyModel = (PlaneParticleModel*)planeBody->GetParticleModel();
	float engineSpeed = planeBodyModel->GetEngineSpeed();

	planePos = planeBody->GetTransform()->GetPosition();

	if (GetAsyncKeyState('B'))
	{
		planePos = XMFLOAT3(planePos.x, 20.0f, planePos.z);
		planeBody->GetTransform()->SetPosition(planePos);
	}

	if (GetAsyncKeyState('W'))
	{
		XMFLOAT3 currentPos = planeBody->GetTransform()->GetPosition();
		// Set Car Engine Speed
		
		planeBodyModel->AddEngineSpeed((engineSpeedAdd));

		planeBodyModel->CalculateVelocity();
		planeBodyModel->CalculateThrust(engineSpeed);		
		planeBodyModel->MotionInFluid(t);

		planeBody->GetTransform()->SetPosition(currentPos.x, currentPos.y + engineSpeed,currentPos.z);


	}

	else if (GetAsyncKeyState('S'))
	{

		planeBodyModel->AddEngineSpeed((engineSpeedAdd));
		planeBodyModel->MotionInFluid(t);
	}
	else
	{
		float engineSpeed = planeBodyModel->GetEngineSpeed();

		if (engineSpeed > 0)
		{
			planeBodyModel->AddEngineSpeed(-engineSpeedAdd);

			if (engineSpeed < 0.0008f && engineSpeed != 0.0f)
			{
				planeBodyModel->SetEngineSpeed(0.0f);
			}
		}
		else if (engineSpeed < 0)
		{
			planeBodyModel->AddEngineSpeed(engineSpeedAdd);
		}
	}

	// Car Rotation Check
	if (GetAsyncKeyState('A'))
	{
		planeWheelRotation -= 0.01f;

	}
	else if (GetAsyncKeyState('D'))
	{
		planeWheelRotation += 0.01f;
	}
	else
	{
		if (planeWheelRotation < 0)
		{
			planeWheelRotation += 0.02f;
		}
		else if (planeWheelRotation > 0)
		{
			planeWheelRotation -= 0.02f;
		}

		if (planeWheelRotation < 0.01f && planeWheelRotation > -0.01f)
		{
			planeWheelRotation = 0;
		}
	}
}

void Plane::CalculateForwardVector()
{
	planeBody->GetTransform()->GetRotation();

	planeForwardVector.x = sin((planeRotation / 17.425f) * (XM_PI / 180.0f));
	planeForwardVector.y = 0.0f;
	planeForwardVector.z = cos((planeRotation / 17.425f) * (XM_PI / 180.0f));

	float planeDirectionMag = sqrt((planeForwardVector.x * planeForwardVector.x) + (planeForwardVector.y * planeForwardVector.y) + (planeForwardVector.z * planeForwardVector.z));
	planeForwardVector = XMFLOAT3((planeForwardVector.x / planeDirectionMag), (planeForwardVector.y / planeDirectionMag), (planeForwardVector.z / planeDirectionMag));

	PlaneParticleModel* planeBodyModel = (PlaneParticleModel*)planeBody->GetParticleModel();
	planeBodyModel->SetPlaneDirection(planeForwardVector);

	// Reset Car Rotation if over 360 or 0 degrees
	if (planeRotation <= -6247.0f || planeRotation >= 6247.0f)
	{
		planeRotation = 0.0f;
	}
}

void Plane::Update(float t)
{
	CalculateForwardVector();

	PlaneParticleModel* planeBodyModel = (PlaneParticleModel*)planeBody->GetParticleModel();
	float engineSpeed = planeBodyModel->GetEngineSpeed();

	// Update Car Wheels Rotation
	planeBody->GetChildren().at(0)->GetTransform()->SetRotation(engineSpeed * 50, planeWheelRotation, 0.0f);
	planeBody->GetChildren().at(1)->GetTransform()->SetRotation(engineSpeed * 50, 0.0f, 0.0f);
	planeBody->GetChildren().at(2)->GetTransform()->SetRotation(engineSpeed * 50, 0.0f, 0.0f);

	// Check Plane type --- you will want to add an AI controlled plane
	string type = planeBody->GetType();

	if (type == "Plane")
	{
		
		XMFLOAT3 velTemp = planeBody->GetParticleModel()->GetVelocity();
		XMFLOAT3 carVelTemp = planeBodyModel->GetPlaneVelocity();

		velTemp.x += carVelTemp.x;
		velTemp.y += carVelTemp.y;
		velTemp.z += carVelTemp.z;

		planeBodyModel->SetVelocity(velTemp);

		// Limit Wheel Rotation
		float planeWheelLimit = 0.1f;

		if (planeWheelRotation <= -planeWheelLimit)
		{
			planeWheelRotation = -planeWheelLimit;
		}
		else if (planeWheelRotation >= planeWheelLimit)
		{
			planeWheelRotation = planeWheelLimit;
		}
	}
	else
	{
		planePos = planeBody->GetTransform()->GetPosition();
		// AI positioning
		XMFLOAT3 planeVelocity = planeBodyModel->GetPlaneVelocity();

		Pathfinding();
		GoToTarget();

		XMFLOAT3 velTemp = planeBody->GetParticleModel()->GetVelocity();
		XMFLOAT3 carVelTemp = planeBodyModel->GetPlaneVelocity();

		velTemp.x += carVelTemp.x / t;
		velTemp.y += carVelTemp.y / t;
		velTemp.z += carVelTemp.z / t;

		planeBodyModel->SetVelocity(velTemp);

		// Limit Wheel Rotation
		float planeWheelLimit = 0.1f;

		if (planeWheelRotation <= -planeWheelLimit)
		{
			planeWheelRotation = -planeWheelLimit;
		}
		else if (planeWheelRotation >= planeWheelLimit)
		{
			planeWheelRotation = planeWheelLimit;
		}


	}


	// Rotate Car in consideration to the Wheels Rotation
	if (engineSpeed > 0)
	{
		planeRotation += (planeWheelRotation * 100) * (engineSpeed * 20);
	}
	else if (engineSpeed < 0)
	{
		engineSpeed *= -1;
		planeRotation -= (planeWheelRotation * 100) * (engineSpeed * 20);
	}

	planeBody->GetTransform()->SetRotation(-1.5500f, planeRotation * planeRotationSpeed + 3.14139f, 0.0f);

	// Update Transform
	planeBody->Update(t);

	// Update Particle Model
	planeBody->GetParticleModel()->Update(t);
}

void Plane::Draw(ID3D11DeviceContext* _pImmediateContext)
{
	planeBody->Draw(_pImmediateContext);
}
void Plane::Pathfinding()
{
	targetPosition = waypoints.at(targetIndex)->GetTransform()->GetPosition();
	XMFLOAT3 carPosition = GetPlaneBody()->GetTransform()->GetPosition();

	// Distance to Target Position
	XMFLOAT3 targetDistance;
	targetDistance.x = targetPosition.x - carPosition.x;
	targetDistance.y = targetPosition.y - carPosition.y;
	targetDistance.z = targetPosition.z - carPosition.z;

	float targetDistanceMagnitude = sqrt((targetDistance.x * targetDistance.x) + (targetDistance.y * targetDistance.y) + (targetDistance.z * targetDistance.z));

	if (targetDistanceMagnitude <= 30.0f)
	{
		targetIndex += 1;
	}

	if (targetIndex >= waypoints.size())
	{
		targetIndex = 0;
	}
}

// --------------- STEERING BEHAVIOUR ---------------- //

void Plane::GoToTarget()
{
	XMFLOAT3 carForwardVector = GetForwardVector();
	XMFLOAT3 carPos = GetPlaneBody()->GetTransform()->GetPosition();

	XMFLOAT3 upDirection = XMFLOAT3(carPos.x, carPos.y + 10.0f, carPos.z);
	upDirection.x = upDirection.x - carPos.x;
	upDirection.y = upDirection.y - carPos.y;
	upDirection.z = upDirection.z - carPos.z;

	XMFLOAT3 forwardDirection;
	forwardDirection.x = carPos.x - (carPos.x + carForwardVector.x);
	forwardDirection.y = carPos.y - (carPos.y + carForwardVector.y);
	forwardDirection.z = carPos.z - (carPos.z + carForwardVector.z);

	XMFLOAT3 targetDirection;
	targetDirection.x = targetPosition.x - carPos.x;
	targetDirection.y = targetPosition.y - carPos.y;
	targetDirection.z = targetPosition.z - carPos.z;

	XMFLOAT3 crossProduct;
	crossProduct.x = (forwardDirection.y * targetDirection.z) - (forwardDirection.z * targetDirection.y);
	crossProduct.y = (forwardDirection.z * targetDirection.x) - (forwardDirection.x * targetDirection.z);
	crossProduct.z = (forwardDirection.x * targetDirection.y) - (forwardDirection.y * targetDirection.x);

	float dotProduct = (crossProduct.x + upDirection.x) + (crossProduct.y + upDirection.y) + (crossProduct.z + upDirection.z);

	PlaneParticleModel* carBodyTemp = (PlaneParticleModel*)GetPlaneBody()->GetParticleModel();
	float engineSpeed = carBodyTemp->GetEngineSpeed();

	if (dotProduct < 5.0f && dotProduct > -5.0f)
	{
		carBodyTemp->AddEngineSpeed(0.0008f);
		SetPlaneWheelRotation(0.0f);
	}
	else if (dotProduct > 5.0f)
	{
		AddPlaneWheelRotation(-0.005f);

		if (engineSpeed < 0.1)
		{
			carBodyTemp->AddEngineSpeed(0.0002f);
		}
		else if (engineSpeed > 0.1)
		{
			carBodyTemp->AddEngineSpeed(-0.0002f);
		}
	}
	else if (dotProduct < 5.0f)
	{
		AddPlaneWheelRotation(0.005f);

		if (engineSpeed < 0.1)
		{
			carBodyTemp->AddEngineSpeed(0.0002f);
		}
		else if (engineSpeed > 0.1)
		{
			carBodyTemp->AddEngineSpeed(-0.0002f);
		}
	}

	engineSpeed = carBodyTemp->GetEngineSpeed();

	GameObject* cartemp = GetPlaneBody();
	float carRotation = GetPlaneBody()->GetTransform()->GetRotation().y;
	float carWheelRotation = GetPlaneWheelRotation();
	float carRotationSpeed = 0.1f;
}