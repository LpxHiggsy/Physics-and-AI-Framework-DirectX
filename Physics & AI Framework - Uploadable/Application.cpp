#include "Application.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

bool Application::HandleKeyboard(MSG msg)
{
	XMFLOAT3 cameraPosition = _camera->GetPosition();

	switch (msg.wParam)
	{
	case VK_UP:
		_cameraOrbitRadius = max(_cameraOrbitRadiusMin, _cameraOrbitRadius - (_cameraSpeed * 0.2f));
		return true;
		break;

	case VK_DOWN:
		_cameraOrbitRadius = min(_cameraOrbitRadiusMax, _cameraOrbitRadius + (_cameraSpeed * 0.2f));
		return true;
		break;

	case VK_RIGHT:
		_cameraOrbitAngleXZ -= _cameraSpeed;
		return true;
		break;

	case VK_LEFT:
		_cameraOrbitAngleXZ += _cameraSpeed;
		return true;
		break;
	}

	return false;
}

Application::Application()
{
	_hInst = nullptr;
	_hWnd = nullptr;
	_driverType = D3D_DRIVER_TYPE_NULL;
	_featureLevel = D3D_FEATURE_LEVEL_11_0;
	_pd3dDevice = nullptr;
	_pImmediateContext = nullptr;
	_pSwapChain = nullptr;
	_pRenderTargetView = nullptr;
	_pVertexShader = nullptr;
	_pPixelShader = nullptr;
	_pVertexLayout = nullptr;
	_pVertexBuffer = nullptr;
	_pIndexBuffer = nullptr;
	_pConstantBuffer = nullptr;

	DSLessEqual = nullptr;
	RSCullNone = nullptr;
}

Application::~Application()
{
	Cleanup();
}

// ----------------- Initialisation ------------------ //

HRESULT Application::Initialise(HINSTANCE hInstance, int nCmdShow)
{
    if (FAILED(InitWindow(hInstance, nCmdShow)))
	{
        return E_FAIL;
	}

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _WindowWidth = rc.right - rc.left;
    _WindowHeight = rc.bottom - rc.top;

    if (FAILED(InitDevice()))
    {
        Cleanup();

        return E_FAIL;
    }

	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\stone.dds", nullptr, &_pTextureRV);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\CFAPlaneTexture.dds", nullptr, &_pAIPlaneTex);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\F35PlaneTexture.dds", nullptr, &_pPlaneTex);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\carTex.dds", nullptr, &_pCarTex);

	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\track01.dds", nullptr, &_pRaceTrackTex);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\grass.dds", nullptr, &_pGrassTex);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\mountain.dds", nullptr, &_pMountainTex);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\floor.dds", nullptr, &_pStoneTex);
	CreateDDSTextureFromFile(_pd3dDevice, L"Resources\\skyBox.dds", nullptr, &_pSkyTex);

	// Setup Camera
	XMFLOAT3 eye = XMFLOAT3(0.0f, 2.0f, -1.0f);
	XMFLOAT3 at = XMFLOAT3(0.0f, 2.0f, 0.0f);
	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	_camera = new Camera(eye, at, up, (float)_renderWidth, (float)_renderHeight, 0.01f, 100.0f);

	InitObjects();

	return S_OK;
}

void Application::InitObjects()
{
	// Setup the scene's light
	basicLight.AmbientLight = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	basicLight.DiffuseLight = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	basicLight.SpecularLight = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	basicLight.SpecularPower = 20.0f;
	basicLight.LightVecW = XMFLOAT3(0.0f, 1.0f, -1.0f);

	cubeGeometry.indexBuffer = _pIndexBuffer;
	cubeGeometry.vertexBuffer = _pVertexBuffer;
	cubeGeometry.numberOfIndices = 36;
	cubeGeometry.vertexBufferOffset = 0;
	cubeGeometry.vertexBufferStride = sizeof(SimpleVertex);

	// SkyBox Material
	Material skyBoxMat;
	skyBoxMat.ambient = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	skyBoxMat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skyBoxMat.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	skyBoxMat.specularPower = 10.0f;

	// Shiny Material
	shinyMaterial.ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	shinyMaterial.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	shinyMaterial.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	shinyMaterial.specularPower = 10.0f;

	// No Specular Material
	noSpecMaterial.ambient = XMFLOAT4(255.0f, 255.0f, 255.0f, 255.0f);
	noSpecMaterial.diffuse = XMFLOAT4(255.0f, 255.0f, 255.0f, 255.0f);
	noSpecMaterial.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	noSpecMaterial.specularPower = 0.0f;

	// Appearance, Transform, ParticleModel
	Appearance* appearance;
	Transform* transform;
	ParticleModel* particleModel;

	// Ground Plane Initialisation
	Geometry groundGeometry = OBJLoader::Load("Objects/groundPlane.obj", _pd3dDevice);

	appearance = new Appearance(groundGeometry, shinyMaterial);
	appearance->SetTextureRV(_pGrassTex);

	transform = new Transform();
	transform->SetPosition(0.0f, -1.0f, 0.0f);
	transform->SetScale(0.5f, 0.5f, 0.5f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	groundPlane = new GameObject("Ground Plane", appearance, transform, particleModel);

	// Mountains Initialisation
	Geometry mountainGeometry = OBJLoader::Load("Objects/mountain.obj", _pd3dDevice);

	appearance = new Appearance(mountainGeometry, shinyMaterial);
	appearance->SetTextureRV(_pMountainTex);

	transform = new Transform();
	transform->SetPosition(0.0f, 0.0f, 0.0f);
	transform->SetScale(0.2f, 0.2f, 0.2f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	mountain = new GameObject("Mountain", appearance, transform, particleModel);

	// Race Track Initialisaton
	Geometry raceTrackGeometry = OBJLoader::Load("Objects/raceTrack.obj", _pd3dDevice);

	appearance = new Appearance(raceTrackGeometry, shinyMaterial);
	appearance->SetTextureRV(_pRaceTrackTex);

	transform = new Transform();
	transform->SetPosition(0.0f, 0.5f, 0.0f);
	transform->SetScale(1.0f, 1.0f, 1.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	raceTrack = new GameObject("Race Track", appearance, transform, particleModel);


	// Sky Box Initialisation
	Geometry skyBoxGeometry = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(skyBoxGeometry, shinyMaterial);
	appearance->SetTextureRV(_pSkyTex);

	transform = new Transform();
	transform->SetPosition(0.0f, 0.0f, 0.0f);
	transform->SetScale(-10000.0f, -10000.0f, -10000.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	skyBox = new GameObject("Sky Box", appearance, transform, particleModel);

	Geometry way1 = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(way1, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(-50.5f * 2, 1.8f * 2, 20.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	waypoint1 = new GameObject("Waypoint", appearance, transform, particleModel);

	Geometry way2 = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(way2, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(0.5f * 2, 1.8f * 2, 8.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	waypoint2 = new GameObject("Waypoint", appearance, transform, particleModel);

	Geometry way3 = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(way3, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(50.5f*2, 1.8f * 2, 8.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	waypoint3 = new GameObject("Waypoint", appearance, transform, particleModel);

	Geometry way4 = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(way4, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(50.5f * 2, 1.8f * 2, -20.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	waypoint4 = new GameObject("Waypoint", appearance, transform, particleModel);

	Geometry way5 = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(way5, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(-70.5f * 2, 1.8f * 2, -20.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	waypoint5 = new GameObject("Waypoint", appearance, transform, particleModel);

	waypoints.push_back(waypoint1);
	waypoints.push_back(waypoint2);
	waypoints.push_back(waypoint3);
	waypoints.push_back(waypoint4);
	waypoints.push_back(waypoint5);

	// Init Plane collection Objects
	InitPlaneObjects();
	InitAIPlaneObjects();
	bPops();
	sPops();
	
	// Camera
	XMFLOAT3 planePos = myPlane->GetPlanePosition();

	myPlane->CalculateForwardVector();
	XMFLOAT3 planeDirection = myPlane->GetForwardVector();

	camNum = 2;

	eyeX = -10.0f;
	eyeY = 4.0f;
	eyeZ = -8.5f;

	// Main Camera
	XMFLOAT3 Eye = XMFLOAT3(eyeX, eyeY, eyeZ);
	XMFLOAT3 At = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	cameraMain = new Camera(Eye, At, Up, (float)_WindowWidth, (float)_WindowHeight, 0.01f, 10000.0f);
	cameraMain->CalculateViewProjection();

	// Top View Camera
	Eye = XMFLOAT3(planePos.x + 0.01f, planePos.y + 15.0f, planePos.z + 0.01f);
	At = XMFLOAT3(planePos.x, planePos.y, planePos.z);
	Up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	cameraTopView = new Camera(Eye, At, Up, (float)_WindowWidth, (float)_WindowHeight, 0.01f, 10000.0f);
	cameraTopView->CalculateViewProjection();

	// Plane Main Camera
	Eye = XMFLOAT3(-cameraPlanePos.x, -cameraPlanePos.y + 10.0f, -cameraPlanePos.z);
	At = XMFLOAT3(planeDirection.x, planeDirection.y, planeDirection.z);
	Up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	cameraPlaneMain = new Camera(Eye, At, Up, (float)_WindowWidth, (float)_WindowHeight, 0.01f, 10000.0f);
	cameraPlaneMain->CalculateViewProjection();

	// Plane Alternative Camera
	Eye = XMFLOAT3(cameraPlanePos.x, cameraPlanePos.y + 10.0f, cameraPlanePos.z);
	At = XMFLOAT3(planeDirection.x, planeDirection.y, planeDirection.z);
	Up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	cameraPlaneAlternate = new Camera(Eye, At, Up, (float)_WindowWidth, (float)_WindowHeight, 0.01f, 10000.0f);
	cameraPlaneAlternate->CalculateViewProjection();

	// Plane Perspective Camera calculation
	Eye = XMFLOAT3(planePos.x + 10.0f, planePos.y + 10.0f, planePos.z + 10.0f);
	At = XMFLOAT3(planePos.x, planePos.y, planePos.z);
	Up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	cameraPlanePerspective = new Camera(Eye, At, Up, (float)_WindowWidth, (float)_WindowHeight, 0.01f, 5000.0f);
	cameraPlanePerspective->CalculateViewProjection();
}

void Application::sPops()
{
	Appearance* appearance;
	Transform* transform;
	ParticleModel* particleModel;

	Geometry swG = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(6.5f * 2, 40.8f * 2, 0.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG2 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG2, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-7.5f * 2, 25.8f * 2, 2.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow2 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	Geometry swG3 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG3, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(14.5f * 2, 50.8f * 2, 14.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow3 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG4 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG4, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(8.5f * 2, 333.8f * 2, 10.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow4 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	Geometry swG5 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG5, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(4.5f * 2, 85.8f * 2, 9.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow5 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG6 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG6, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(9.5f * 2, 10.8f * 2, 11.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow6 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG7 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG7, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-9.5f * 2, 166.8f * 2, 12.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow7 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	Geometry swG8 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG8, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-3.5f * 2, 188.8f * 2, 1.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow8 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG9 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG9, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-5.5f * 2, 147.8f * 2, 5.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow9 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG10 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG10, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(7.5f * 2, 122.8f * 2, 4.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow10 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG11 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG11, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-10.5f * 2, 60.8f * 2, 4.8f * 2);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow11 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG12 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG12, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-2.5f * 2, 195.8f * 2, 2.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow12 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG13 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG13, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(0.5f * 2, 145.8f * 2, 8.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow13 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	Geometry swG14 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG14, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(5.5f * 2, 275.8f * 2, 0.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow14 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//
	Geometry swG15 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(swG15, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(10.5f * 2, 175.8f * 2, 14.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow15 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(14.5f * 2, 80.8f * 2, 2.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow16 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-15.5f * 2, 160.8f * 2, 4.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow17 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(1.5f * 2, 150.8f * 2, 1.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow18 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(10.5f * 2, 190.8f * 2, 14.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow19 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-1.5f * 2, 250.8f * 2, -13.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow20 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(10.5f * 2, 230.8f * 2, -10.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow21 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-10.5f * 2, 800.8f * 2, 14.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow22 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(3.5f * 2, 70.8f * 2, -3.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow23 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-2.5f * 2, 150.8f * 2, 4.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow24 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(7.5f * 2, 140.8f * 2, 7.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow25 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(1.5f * 2, 130.8f * 2, -14.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow26 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-10.5f * 2, 120.8f * 2, -15.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow27 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-14.5f * 2, 200.8f * 2, -0.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow28 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(10.5f * 2, 90.8f * 2, 14.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow29 = new GameObject("Snow", appearance, transform, particleModel);

	//-----------------------------------------------------------------------------------------//

	appearance = new Appearance(swG, noSpecMaterial);

	transform = new Transform();
	transform->SetPosition(-17.5f * 2, 120.8f * 2, 4.8f * 2);
	transform->SetScale(0.5f, 0.5f, 0.5f);

	particleModel = new SnowParticleModel(transform, 1.0f);

	snow30 = new GameObject("Snow", appearance, transform, particleModel);

	snowP = new SnowParticle(snow); snow2P = new SnowParticle(snow2); snow3P = new SnowParticle(snow3); snow4P = new SnowParticle(snow4); snow5P = new SnowParticle(snow5);
	snow6P = new SnowParticle(snow6);snow7P = new SnowParticle(snow7);snow8P = new SnowParticle(snow8);snow9P = new SnowParticle(snow9);snow10P = new SnowParticle(snow10);
	snow11P = new SnowParticle(snow11);snow12P = new SnowParticle(snow12);snow13P = new SnowParticle(snow13);snow14P = new SnowParticle(snow14);snow15P = new SnowParticle(snow15);
	snow16P = new SnowParticle(snow16); snow17P = new SnowParticle(snow17); snow18P = new SnowParticle(snow18); snow19P = new SnowParticle(snow19); snow20P = new SnowParticle(snow20);
	snow21P = new SnowParticle(snow21); snow22P = new SnowParticle(snow22); snow23P = new SnowParticle(snow23); snow24P = new SnowParticle(snow24); snow25P = new SnowParticle(snow25);
	snow26P = new SnowParticle(snow26); snow27P = new SnowParticle(snow27); snow28P = new SnowParticle(snow28); snow29P = new SnowParticle(snow29); snow30P = new SnowParticle(snow30);


	snowArray.push_back(snowP); snowArray.push_back(snow2P); snowArray.push_back(snow3P); snowArray.push_back(snow4P); snowArray.push_back(snow5P);
	snowArray.push_back(snow6P); snowArray.push_back(snowP); snowArray.push_back(snowP); snowArray.push_back(snow8P); snowArray.push_back(snow10P);
	snowArray.push_back(snow11P); snowArray.push_back(snow12P); snowArray.push_back(snow13P); snowArray.push_back(snow14P); snowArray.push_back(snow15P);
	snowArray.push_back(snow16P); snowArray.push_back(snow17P); snowArray.push_back(snow18P); snowArray.push_back(snow19P); snowArray.push_back(snow20P);
	snowArray.push_back(snow21P); snowArray.push_back(snow22P); snowArray.push_back(snow23P); snowArray.push_back(snow24P); snowArray.push_back(snow25P);
	snowArray.push_back(snow26P); snowArray.push_back(snow27P); snowArray.push_back(snow28P); snowArray.push_back(snow29P); snowArray.push_back(snow30P);
	

}

void Application::bPops()
{
	Appearance* appearance;
	Transform* transform;
	ParticleModel* particleModel;

	Geometry ballG = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	//transform->SetPosition(500.415f, 150.0f, 75.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);

	particleModel = new PlaneParticleModel(transform, 0.2f);

	ball = new GameObject("Ball", appearance, transform, particleModel);

	//----------------------------------------------------------------------
	Geometry ballG2 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG2, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	transform->SetPosition(145.415f, 15.0f, -250.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	ball2 = new GameObject("Ball", appearance, transform, particleModel);
	//----------------------------------------------------------------------
	Geometry ballG3 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG3, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	transform->SetPosition(-115.415f, 150.0f, 20.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 0.5f);

	ball3 = new GameObject("Ball", appearance, transform, particleModel);
	//----------------------------------------------------------------------
	Geometry ballG4 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG4, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	transform->SetPosition(-200.415f, 150.0f, 200.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 0.3f);

	ball4 = new GameObject("Ball", appearance, transform, particleModel);
	//----------------------------------------------------------------------
	Geometry ballG5 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG5, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	transform->SetPosition(-575.415f, 150.0f, -25.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 0.4f);

	ball5 = new GameObject("Ball", appearance, transform, particleModel);
	//----------------------------------------------------------------------
	Geometry ballG6 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG6, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	transform->SetPosition(700.415f, 150.0f, 33.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 0.3f);

	ball6 = new GameObject("Ball", appearance, transform, particleModel);
	//----------------------------------------------------------------------
	Geometry ballG7 = OBJLoader::Load("Objects/sphere.obj", _pd3dDevice);

	appearance = new Appearance(ballG7, shinyMaterial);
	appearance->SetTextureRV(_pTextureRV);

	transform = new Transform();
	transform->SetPosition(-75.415f, 150.0f, -100.63f);
	transform->SetScale(10.0f, 10.0f, 10.0f);
	transform->SetRotation(XMConvertToRadians(0.0f), 0.0f, 0.0f);

	particleModel = new PlaneParticleModel(transform, 0.2f);

	ball7 = new GameObject("Ball", appearance, transform, particleModel);


	bArray.push_back(ball);
	bArray.push_back(ball2);
	bArray.push_back(ball3);
	bArray.push_back(ball4);
	bArray.push_back(ball5);
	bArray.push_back(ball6);
	bArray.push_back(ball7);


}

void Application::InitPlaneObjects()
{
	Appearance* appearance;
	Transform* transform;
	ParticleModel* particleModel;

	// Plane Objects

	// Plane Body
	Geometry planeGeometry = OBJLoader::Load("Objects/Plane Objects/F-35_Lightning_II.obj", _pd3dDevice);

	//XMFLOAT3 planePos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 planePos = XMFLOAT3(-415.415f, 20.0f, -15.63f);

	appearance = new Appearance(planeGeometry, shinyMaterial);
	appearance->SetTextureRV(_pPlaneTex);

	transform = new Transform();
	transform->SetScale(1.0f, 1.0f, 1.0f);
	transform->SetPosition(planePos);

	particleModel = new PlaneParticleModel(transform, 1.0f);
	particleModel->SetCollisionRadius(10.0f);

	GameObject* planeBody = new GameObject("Plane", appearance, transform, particleModel);

	// Plane Tyre Front Right
	Geometry carTyreFrontRGeometry = OBJLoader::Load("Objects/Plane Objects/carTyreFrontR.obj", _pd3dDevice);

	appearance = new Appearance(carTyreFrontRGeometry, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f,0.1f,0.1f);
	transform->SetPosition(planePos.x,planePos.y,planePos.z);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	GameObject* carTyreFrontR = new GameObject("Car Tyre", appearance, transform, particleModel);

	// Car Tyre Back Right
	Geometry carTyreBackRGeometry = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(carTyreBackRGeometry, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(3.5f*2, 1.8f*2, 8.8f*2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	GameObject* carTyreBackR = new GameObject("Car Tyre", appearance, transform, particleModel);

	// Car Tyre Back Left
	Geometry carTyreBackLGeometry = OBJLoader::Load("Objects/Plane Objects/carTyreBackL.obj", _pd3dDevice);

	appearance = new Appearance(carTyreBackLGeometry, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(3.5*2, 1.8f*2, -8.8f*2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	GameObject* carTyreBackL = new GameObject("Car Tyre", appearance, transform, particleModel);

	// Add Wheels to Plane Scene Graph
	vector < GameObject* > planeWheels;
	planeWheels.push_back(carTyreFrontR);
	planeWheels.push_back(carTyreBackR);
	planeWheels.push_back(carTyreBackL);

	planeBody->AddChild(carTyreFrontR);
	planeBody->AddChild(carTyreBackR);
	planeBody->AddChild(carTyreBackL);

	myPlane = new Plane(planeBody, planeWheels, waypoints);
}

void Application::InitAIPlaneObjects()
{
	Appearance* appearance;
	Transform* transform;
	ParticleModel* particleModel;

	// Plane Objects

	// Plane Body
	Geometry planeGeometry = OBJLoader::Load("Objects/Plane Objects/F-35_Lightning_II.obj", _pd3dDevice);

	//XMFLOAT3 planePos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 planePos = XMFLOAT3(-415.415f, 20.0f, 25.63f);

	appearance = new Appearance(planeGeometry, shinyMaterial);
	appearance->SetTextureRV(_pAIPlaneTex);

	transform = new Transform();
	transform->SetScale(1.0f, 1.0f, 1.0f);
	transform->SetPosition(planePos);

	particleModel = new PlaneParticleModel(transform, 1.0f);
	particleModel->SetCollisionRadius(10.0f);

	GameObject* planeBody = new GameObject("AIPlane", appearance, transform, particleModel);

	// Plane Tyre Front Right
	Geometry carTyreFrontRGeometry = OBJLoader::Load("Objects/Plane Objects/carTyreFrontR.obj", _pd3dDevice);

	appearance = new Appearance(carTyreFrontRGeometry, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(planePos.x, planePos.y, planePos.z);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	GameObject* carTyreFrontR = new GameObject("Car Tyre", appearance, transform, particleModel);

	// Car Tyre Back Right
	Geometry carTyreBackRGeometry = OBJLoader::Load("Objects/Plane Objects/carTyreBackR.obj", _pd3dDevice);

	appearance = new Appearance(carTyreBackRGeometry, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(3.5f * 2, 1.8f * 2, 8.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	GameObject* carTyreBackR = new GameObject("Car Tyre", appearance, transform, particleModel);

	// Car Tyre Back Left
	Geometry carTyreBackLGeometry = OBJLoader::Load("Objects/Plane Objects/carTyreBackL.obj", _pd3dDevice);

	appearance = new Appearance(carTyreBackLGeometry, shinyMaterial);
	appearance->SetTextureRV(_pCarTex);

	transform = new Transform();
	transform->SetScale(0.1f, 0.1f, 0.1f);
	transform->SetPosition(3.5 * 2, 1.8f * 2, -8.8f * 2);

	particleModel = new PlaneParticleModel(transform, 1.0f);

	GameObject* carTyreBackL = new GameObject("Car Tyre", appearance, transform, particleModel);

	// Add Wheels to Plane Scene Graph
	vector < GameObject* > planeWheels;
	planeWheels.push_back(carTyreFrontR);
	planeWheels.push_back(carTyreBackR);
	planeWheels.push_back(carTyreBackL);

	planeBody->AddChild(carTyreFrontR);
	planeBody->AddChild(carTyreBackR);
	planeBody->AddChild(carTyreBackL);

	AIplane = new Plane(planeBody, planeWheels, waypoints);
}

HRESULT Application::InitShadersAndInputLayout()
{
	HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "VS", "vs_4_0", &pVSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the vertex shader
	hr = _pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &_pVertexShader);

	if (FAILED(hr))
	{	
		pVSBlob->Release();
        return hr;
	}

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile(L"DX11 Framework.fx", "PS", "ps_4_0", &pPSBlob);

    if (FAILED(hr))
    {
        MessageBox(nullptr,
                   L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
        return hr;
    }

	// Create the pixel shader
	hr = _pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &_pPixelShader);
	pPSBlob->Release();

    if (FAILED(hr))
        return hr;
	
    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
	hr = _pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(), &_pVertexLayout);
	pVSBlob->Release();

	if (FAILED(hr))
        return hr;

    // Set the input layout
    _pImmediateContext->IASetInputLayout(_pVertexLayout);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = _pd3dDevice->CreateSamplerState(&sampDesc, &_pSamplerLinear);

	return hr;
}

HRESULT Application::InitVertexBuffer()
{
	HRESULT hr;

    // Create vertex buffer
    SimpleVertex vertices[] =
    {
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
    };

    D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(SimpleVertex) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;

    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pVertexBuffer);

    if (FAILED(hr))
        return hr;

	// Create vertex buffer
	SimpleVertex planeVertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 5.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(5.0f, 5.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(5.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
	};

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = planeVertices;

	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPlaneVertexBuffer);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Application::InitIndexBuffer()
{
	HRESULT hr;

    // Create index buffer
    WORD indices[] =
    {
		3, 1, 0,
		2, 1, 3,

		6, 4, 5,
		7, 4, 6,

		11, 9, 8,
		10, 9, 11,

		14, 12, 13,
		15, 12, 14,

		19, 17, 16,
		18, 17, 19,

		22, 20, 21,
		23, 20, 22
    };

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 36;     
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = indices;
    hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pIndexBuffer);

    if (FAILED(hr))
        return hr;

	// Create plane index buffer
	WORD planeIndices[] =
	{
		0, 3, 1,
		3, 2, 1,
	};

	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 6;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = planeIndices;
	hr = _pd3dDevice->CreateBuffer(&bd, &InitData, &_pPlaneIndexBuffer);

	if (FAILED(hr))
		return hr;

	return S_OK;
}

HRESULT Application::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"TutorialWindowClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    _hInst = hInstance;
    RECT rc = {0, 0, 960, 540};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    _hWnd = CreateWindow(L"TutorialWindowClass", L"FGGC Semester 2 Framework", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
                         nullptr);
    if (!_hWnd)
		return E_FAIL;

    ShowWindow(_hWnd, nCmdShow);

    return S_OK;
}

HRESULT Application::CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* pErrorBlob;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

    if (FAILED(hr))
    {
        if (pErrorBlob != nullptr)
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());

        if (pErrorBlob) pErrorBlob->Release();

        return hr;
    }

    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}

HRESULT Application::InitDevice()
{
    HRESULT hr = S_OK;

    UINT createDeviceFlags = 0;

#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	UINT sampleCount = 4;

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Width = _renderWidth;
    sd.BufferDesc.Height = _renderHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = _hWnd;
	sd.SampleDesc.Count = sampleCount;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        _driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain(nullptr, _driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                           D3D11_SDK_VERSION, &sd, &_pSwapChain, &_pd3dDevice, &_featureLevel, &_pImmediateContext);
        if (SUCCEEDED(hr))
            break;
    }

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = _pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    if (FAILED(hr))
        return hr;

    hr = _pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &_pRenderTargetView);
    pBackBuffer->Release();

    if (FAILED(hr))
        return hr;

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)_renderWidth;
    vp.Height = (FLOAT)_renderHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    _pImmediateContext->RSSetViewports(1, &vp);

	InitShadersAndInputLayout();

	InitVertexBuffer();
	InitIndexBuffer();

    // Set primitive topology
    _pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the constant buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
    hr = _pd3dDevice->CreateBuffer(&bd, nullptr, &_pConstantBuffer);

    if (FAILED(hr))
        return hr;

	D3D11_TEXTURE2D_DESC depthStencilDesc;

	depthStencilDesc.Width = _renderWidth;
	depthStencilDesc.Height = _renderHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = sampleCount;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	_pd3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, &_depthStencilBuffer);
	_pd3dDevice->CreateDepthStencilView(_depthStencilBuffer, nullptr, &_depthStencilView);

	_pImmediateContext->OMSetRenderTargets(1, &_pRenderTargetView, _depthStencilView);

	// Rasterizer
	D3D11_RASTERIZER_DESC cmdesc;

	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_NONE;
	hr = _pd3dDevice->CreateRasterizerState(&cmdesc, &RSCullNone);

	D3D11_DEPTH_STENCIL_DESC dssDesc;
	ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	_pd3dDevice->CreateDepthStencilState(&dssDesc, &DSLessEqual);

	ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));

	cmdesc.FillMode = D3D11_FILL_SOLID;
	cmdesc.CullMode = D3D11_CULL_BACK;

	cmdesc.FrontCounterClockwise = true;
	hr = _pd3dDevice->CreateRasterizerState(&cmdesc, &CCWcullMode);

	cmdesc.FrontCounterClockwise = false;
	hr = _pd3dDevice->CreateRasterizerState(&cmdesc, &CWcullMode);

    return S_OK;
}

// -------------- Cleanup -------------- //

void Application::Cleanup()
{
    if (_pImmediateContext) _pImmediateContext->ClearState();
	if (_pSamplerLinear) _pSamplerLinear->Release();

	if (_pTextureRV) _pTextureRV->Release();

	if (_pGrassTex) _pGrassTex->Release();

    if (_pConstantBuffer) _pConstantBuffer->Release();

    if (_pVertexBuffer) _pVertexBuffer->Release();
    if (_pIndexBuffer) _pIndexBuffer->Release();
	if (_pPlaneVertexBuffer) _pPlaneVertexBuffer->Release();
	if (_pPlaneIndexBuffer) _pPlaneIndexBuffer->Release();

    if (_pVertexLayout) _pVertexLayout->Release();
    if (_pVertexShader) _pVertexShader->Release();
    if (_pPixelShader) _pPixelShader->Release();
    if (_pRenderTargetView) _pRenderTargetView->Release();
    if (_pSwapChain) _pSwapChain->Release();
    if (_pImmediateContext) _pImmediateContext->Release();
    if (_pd3dDevice) _pd3dDevice->Release();
	if (_depthStencilView) _depthStencilView->Release();
	if (_depthStencilBuffer) _depthStencilBuffer->Release();

	if (DSLessEqual) DSLessEqual->Release();
	if (RSCullNone) RSCullNone->Release();

	if (CCWcullMode) CCWcullMode->Release();
//	if (CWcullMode) CWcullMode->Release();

	if (_camera)
	{
		delete _camera;
		_camera = nullptr;
	}

	for (auto gameObject : _cubes)
	{
		if (gameObject)
		{
			delete gameObject;
			gameObject = nullptr;
		}
	}
}

void Application::CameraInput(float t)
{
	// If Camera 1 then Check Free Camera input
	if (camNum == 1)
	{
		float cameraMoveSpeed = 0.1f;

		// Zoom Out
		if (GetAsyncKeyState(VK_DOWN))
		{
			eyeX -= cameraMoveSpeed;
		}

		// Zoom In
		if (GetAsyncKeyState(VK_UP))
		{
			eyeX += cameraMoveSpeed; // or call camera setX function appropriately
		}

		// Pan Up
		if (GetAsyncKeyState(0x54))
		{
			eyeY += cameraMoveSpeed;
		}

		// Pan Down
		if (GetAsyncKeyState(0x47))
		{
			eyeY -= cameraMoveSpeed;
		}

		// Pan Left
		if (GetAsyncKeyState(0x46))
		{
			eyeZ += cameraMoveSpeed;
		}
		// Pan right
		if (GetAsyncKeyState(0x48))
		{
			eyeZ -= cameraMoveSpeed;
		}
	}

	// If Camera 1, 2 or 3 then Check Plane Input
	if (camNum == 1 || camNum == 2 || camNum == 3)
	{
		myPlane->Input(t);
	}

	// Camera Input Check
	if (GetAsyncKeyState('1'))
	{
		camNum = 1;
	}
	else if (GetAsyncKeyState('2'))
	{
		camNum = 2;
	}
	else if (GetAsyncKeyState('3'))
	{
		camNum = 3;
	}
	else if (GetAsyncKeyState('4'))
	{
		camNum = 4;
	}
	else if (GetAsyncKeyState('5'))
	{
		camNum = 5;
	}

	XMFLOAT3 Eye;
	XMFLOAT3 At;

	XMFLOAT3 planePos = myPlane->GetPlanePosition();
	XMFLOAT3 planeDirection = myPlane->GetForwardVector();

	// Check Camera Number
	if (camNum == 1)
	{
		Eye = XMFLOAT3(eyeX, eyeY, eyeZ);
		At = XMFLOAT3(eyeX + 1.0f, eyeY, eyeZ);

		cameraMain->SetEye(Eye);
		cameraMain->SetAt(At);
		cameraMain->CalculateViewProjection();

		cameraCurrent = cameraMain;
	}
	if (camNum == 1 || camNum == 2)
	{
		cameraCurrent = cameraPlaneMain;

		cameraPlanePos.x = planePos.x;
		cameraPlanePos.y = planePos.y;
		cameraPlanePos.z = planePos.z;

		Eye = XMFLOAT3(cameraPlanePos.x - (planeDirection.x * 15.0f), cameraPlanePos.y + 10.0f, cameraPlanePos.z - (planeDirection.z * 15.0f));
		At = XMFLOAT3(planePos.x + (planeDirection.x * 10.0f), planePos.y + (planeDirection.y * 10.0f), planePos.z + (planeDirection.z * 10.0f));

		cameraPlaneMain->SetEye(Eye);
		cameraPlaneMain->SetAt(At);
		cameraPlaneMain->CalculateViewProjection();
	}
	else if (camNum == 3)
	{
		cameraCurrent = cameraPlaneAlternate;

		cameraPlanePos.x = planePos.x - planeDirection.x;
		cameraPlanePos.y = planePos.y - planeDirection.y;
		cameraPlanePos.z = planePos.z - planeDirection.z;

		Eye = XMFLOAT3(cameraPlanePos.x + (planeDirection.x * 15.0f), cameraPlanePos.y + 10.0f, cameraPlanePos.z + (planeDirection.z * 15.0f));
		At = XMFLOAT3(cameraPlanePos.x - (planeDirection.x * 15.0f), cameraPlanePos.y - 4.0f, cameraPlanePos.z - (planeDirection.z * 15.0f));

		cameraPlaneAlternate->SetEye(Eye);
		cameraPlaneAlternate->SetAt(At);
		cameraPlaneAlternate->CalculateViewProjection();
	}
	else if (camNum == 4)
	{
		cameraCurrent = cameraTopView;

		Eye = XMFLOAT3(planePos.x + 0.01f, planePos.y + 15.0f, planePos.z + 0.01f);
		At = XMFLOAT3(planePos.x, planePos.y, planePos.z);

		cameraTopView->SetEye(Eye);
		cameraTopView->SetAt(At);
		cameraTopView->CalculateViewProjection();
	}
	else if (camNum == 5)
	{
		cameraCurrent = cameraPlanePerspective;

		Eye = XMFLOAT3(planePos.x + 10.0f, planePos.y + 10.0f, planePos.z + 10.0f);
		At = XMFLOAT3(planePos.x, planePos.y, planePos.z);

		cameraPlanePerspective->SetEye(Eye);
		cameraPlanePerspective->SetAt(At);
		cameraPlanePerspective->CalculateViewProjection();
	}
}

// -------------- Updates ------------- //

void Application::PlaneUpdate(float t)
{
	CameraInput(t);

	// Plane Body Updates
	myPlane->Update(t);
	AIplane->Update(t);
	//AIplane1->Update(t);
	
}

void Application::Update(float t)
{
    // Update our time
    static float timeSinceStart = 0.0f;
    static DWORD dwTimeStart = 0;

    DWORD dwTimeCur = GetTickCount();

    if (dwTimeStart == 0)
        dwTimeStart = dwTimeCur;

	timeSinceStart = (dwTimeCur - dwTimeStart) / 1000.0f;

	// Plane Object Updates
	PlaneUpdate(t);

	// Update Ground Plane
	groundPlane->Update(t);

	// Update Mountain
	mountain->Update(t);

	// Update Race Track
	raceTrack->Update(t);

	waypoint1->Update(t);
	waypoint2->Update(t);
	waypoint3->Update(t);
	waypoint4->Update(t);
	waypoint5->Update(t);

	for (int i = 0; i < bArray.size(); i++)
	{
		XMFLOAT3 bCurrPos = bArray[i]->GetTransform()->GetPosition();

		bArray[i]->Update(t);
		bArray[0]->GetParticleModel()->Update(t);
		bArray[3]->GetParticleModel()->Update(t);
		bArray[5]->GetParticleModel()->Update(t);
		bArray[2]->GetParticleModel()->Update(t);

		if (bArray[i]->GetParticleModel()->CollisionCheck(myPlane->GetPlaneBody()->GetTransform()->GetPosition(), myPlane->GetPlaneBody()->GetParticleModel()->GetCollisionRadius()))
			bArray[i]->GetParticleModel()->ResolveCollision(myPlane->GetPlaneBody()->GetParticleModel());

		bArray[i]->GetParticleModel()->BaseCollisionCheck(groundPlane->GetTransform()->GetPosition());

		if (bArray[i]->GetTransform()->GetPosition().y <= 10.0f)
		{
			bArray[i]->GetTransform()->SetPosition(bCurrPos.x, 150.0f, bCurrPos.z);
		}

	}

	for (int i = 0; i < snowArray.size(); i++)
	{
		snowArray[i]->Update(t);

		snowArray[i]->GetSnowParticles()->GetParticleModel()->BaseCollisionCheck(groundPlane->GetTransform()->GetPosition());

	}

	// Sky Box Update
	skyBox->Update(t);

	myPlane->GetPlaneBody()->GetParticleModel()->BaseCollisionCheck(groundPlane->GetTransform()->GetPosition());
	AIplane->GetPlaneBody()->GetParticleModel()->BaseCollisionCheck(groundPlane->GetTransform()->GetPosition());

	if (AIplane->GetPlaneBody()->GetParticleModel()->CollisionCheck(myPlane->GetPlaneBody()->GetTransform()->GetPosition(), myPlane->GetPlaneBody()->GetParticleModel()->GetCollisionRadius()))
		AIplane->GetPlaneBody()->GetParticleModel()->ResolveCollision(myPlane->GetPlaneBody()->GetParticleModel());

}

// --------------- Draw --------------- //

void Application::Draw()
{
    // Clear buffers
	float ClearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f }; // red,green,blue,alpha
    _pImmediateContext->ClearRenderTargetView(_pRenderTargetView, ClearColor);
	_pImmediateContext->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Setup buffers and render scene
	_pImmediateContext->IASetInputLayout(_pVertexLayout);

	_pImmediateContext->VSSetShader(_pVertexShader, nullptr, 0);
	_pImmediateContext->PSSetShader(_pPixelShader, nullptr, 0);

	_pImmediateContext->VSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetConstantBuffers(0, 1, &_pConstantBuffer);
	_pImmediateContext->PSSetSamplers(0, 1, &_pSamplerLinear);


	// Setup View and Projection
	XMMATRIX view;
	XMMATRIX projection;

	if (camNum == 1)
	{
		XMFLOAT4X4 cameraView = cameraMain->GetView();
		view = XMLoadFloat4x4(&cameraView);

		XMFLOAT4X4 cameraProjection = cameraMain->GetProjection();
		projection = XMLoadFloat4x4(&cameraProjection);
	}
	else
	{
		XMFLOAT4X4 cameraView = cameraCurrent->GetView();
		view = XMLoadFloat4x4(&cameraView);

		XMFLOAT4X4 cameraProjection = cameraCurrent->GetProjection();
		projection = XMLoadFloat4x4(&cameraProjection);
	}

	// Render Objects
	ConstantBuffer cb;

	cb.View = XMMatrixTranspose(view);
	cb.Projection = XMMatrixTranspose(projection);
	
	cb.light = basicLight;
	cb.EyePosW = _camera->GetPosition();

	Material material;

	// --------------- Draw Sky Box ---------------- //

	material = skyBox->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(skyBox->GetTransform()->GetWorldMatrix());

	// Set texture
	if (skyBox->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = skyBox->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Draw object
	skyBox->Draw(_pImmediateContext);

	// ------------- Draw Ground Plane ------------- //

	material = groundPlane->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(groundPlane->GetTransform()->GetWorldMatrix());

	// Set texture
	if (groundPlane->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = groundPlane->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Draw object
	groundPlane->Draw(_pImmediateContext);

	// --------------- Draw Mountain --------------- //

	material = mountain->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(mountain->GetTransform()->GetWorldMatrix());

	// Set texture
	if (mountain->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = mountain->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Draw object
	mountain->Draw(_pImmediateContext);


	// -------------- Draw Race Track -------------- //

	material = raceTrack->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(raceTrack->GetTransform()->GetWorldMatrix());

	// Set texture
	if (raceTrack->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = raceTrack->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	
	// Draw object
	raceTrack->Draw(_pImmediateContext);

	//---------------------Draw ball -----------------------------//
	for (int i = 0; i < bArray.size(); i++)
	{
		material = bArray[i]->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(bArray[i]->GetTransform()->GetWorldMatrix());

	// Set texture
	if (bArray[i]->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = bArray[i]->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Draw object
	bArray[i]->Draw(_pImmediateContext);

	}

	////---------------------Draw ball -----------------------------//
	for (int i = 0; i < snowArray.size(); i++)
	{
		GameObject* snowBody = snowArray[i]->GetSnowParticles();
		material = snowBody->GetAppearance()->GetMaterial();

		// Copy material to shader
		cb.surface.AmbientMtrl = material.ambient;
		cb.surface.DiffuseMtrl = material.diffuse;
		cb.surface.SpecularMtrl = material.specular;

		// Set world matrix
		cb.World = XMMatrixTranspose(snowBody->GetTransform()->GetWorldMatrix());

		// Set texture
		if (snowBody->GetAppearance()->HasTexture())
		{
			ID3D11ShaderResourceView* textureRV = snowBody->GetAppearance()->GetTextureRV();
			_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
			cb.HasTexture = 1.0f;
		}
		else
		{
			cb.HasTexture = 0.0f;
		}

		// Update constant buffer
		_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

		// Draw object
		snowBody->Draw(_pImmediateContext);

	}
	

	// ------------Draw waypoint ------------ //

	for (int i = 0; i < waypoints.size(); i++)
	{
		material = waypoints[i]->GetAppearance()->GetMaterial();

		// Copy material to shader
		cb.surface.AmbientMtrl = material.ambient;
		cb.surface.DiffuseMtrl = material.diffuse;
		cb.surface.SpecularMtrl = material.specular;

		// Set world matrix
		cb.World = XMMatrixTranspose(waypoints[i]->GetTransform()->GetWorldMatrix());

		// Set Wheel texture
		if (waypoints[i]->GetAppearance()->HasTexture())
		{
			ID3D11ShaderResourceView* textureRV = waypoints[i]->GetAppearance()->GetTextureRV();
			_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
			cb.HasTexture = 1.0f;
		}
		else
		{
			cb.HasTexture = 0.0f;
		}

		// Update constant buffer
		_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

		// Draw object
		waypoints[i]->Draw(_pImmediateContext);
	}

	// ------------- Draw Plane Body ------------- //

	GameObject* planeBody = myPlane->GetPlaneBody();
	material = planeBody->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(planeBody->GetTransform()->GetWorldMatrix());

	// Set plane texture
	if (planeBody->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = planeBody->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Draw object
	planeBody->Draw(_pImmediateContext);

	// ------------- Draw AIPlane Body ------------- //

	GameObject* AIplaneBody = AIplane->GetPlaneBody();
	material = AIplaneBody->GetAppearance()->GetMaterial();

	// Copy material to shader
	cb.surface.AmbientMtrl = material.ambient;
	cb.surface.DiffuseMtrl = material.diffuse;
	cb.surface.SpecularMtrl = material.specular;

	// Set world matrix
	cb.World = XMMatrixTranspose(AIplaneBody->GetTransform()->GetWorldMatrix());

	// Set plane texture
	if (AIplaneBody->GetAppearance()->HasTexture())
	{
		ID3D11ShaderResourceView* textureRV = AIplaneBody->GetAppearance()->GetTextureRV();
		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
		cb.HasTexture = 1.0f;
	}
	else
	{
		cb.HasTexture = 0.0f;
	}

	// Update constant buffer
	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	// Draw object
	AIplaneBody->Draw(_pImmediateContext);

	// ------------ Draw Plane Wheels ------------ //

	//vector < GameObject* > planeWheels = planeBody->GetChildren();

	//for each (GameObject* wheel in planeWheels)
	//{
	//	material = wheel->GetAppearance()->GetMaterial();

	//	// Copy material to shader
	//	cb.surface.AmbientMtrl = material.ambient;
	//	cb.surface.DiffuseMtrl = material.diffuse;
	//	cb.surface.SpecularMtrl = material.specular;

	//	// Set world matrix
	//	cb.World = XMMatrixTranspose(wheel->GetTransform()->GetWorldMatrix());

	//	// Set Wheel texture
	//	if (wheel->GetAppearance()->HasTexture())
	//	{
	//		ID3D11ShaderResourceView* textureRV = wheel->GetAppearance()->GetTextureRV();
	//		_pImmediateContext->PSSetShaderResources(0, 1, &textureRV);
	//		cb.HasTexture = 1.0f;
	//	}
	//	else
	//	{
	//		cb.HasTexture = 0.0f;
	//	}

	//	// Update constant buffer
	//	_pImmediateContext->UpdateSubresource(_pConstantBuffer, 0, nullptr, &cb, 0, 0);

	//	// Draw object
	//	wheel->Draw(_pImmediateContext);
	//}

    // Present our back buffer to our front buffer
    _pSwapChain->Present(0, 0);
}