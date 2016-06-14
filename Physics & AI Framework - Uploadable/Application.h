#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include "resource.h"
#include <vector>

#include "Structures.h"
#include "Plane.h"
#include "SnowParticle.h"
#include "Camera.h"
#include "PlaneParticleModel.h"
#include "SnowParticleModel.h"
#include "DDSTextureLoader.h"
#include "GameObject.h"
#include "OBJLoader.h"

using namespace DirectX;

class Application
{
private:
	bool temp = true;

	HINSTANCE               _hInst;
	HWND                    _hWnd;
	D3D_DRIVER_TYPE         _driverType;
	D3D_FEATURE_LEVEL       _featureLevel;
	ID3D11Device*           _pd3dDevice;
	ID3D11DeviceContext*    _pImmediateContext;
	IDXGISwapChain*         _pSwapChain;
	ID3D11RenderTargetView* _pRenderTargetView;
	ID3D11VertexShader*     _pVertexShader;
	ID3D11PixelShader*      _pPixelShader;
	ID3D11InputLayout*      _pVertexLayout;

	ID3D11Buffer*           _pVertexBuffer;
	ID3D11Buffer*           _pIndexBuffer;

	ID3D11Buffer*           _pPlaneVertexBuffer;
	ID3D11Buffer*           _pPlaneIndexBuffer;

	ID3D11Buffer*           _pConstantBuffer;

	ID3D11DepthStencilView* _depthStencilView = nullptr;
	ID3D11Texture2D* _depthStencilBuffer = nullptr;

	// Textures
	ID3D11ShaderResourceView* _pTextureRV = nullptr;
	ID3D11ShaderResourceView* _pRaceTrackTex = nullptr;
	ID3D11ShaderResourceView* _pGrassTex = nullptr;
	ID3D11ShaderResourceView* _pMountainTex = nullptr;
	ID3D11ShaderResourceView* _pStoneTex = nullptr;
	ID3D11ShaderResourceView* _pSkyTex = nullptr;
	

	// Car Texture for wheels
	ID3D11ShaderResourceView* _pCarTex = nullptr;

	//aircraft textures
	ID3D11ShaderResourceView* _pAIPlaneTex = nullptr;
	ID3D11ShaderResourceView* _pPlaneTex = nullptr;

	ID3D11SamplerState* _pSamplerLinear = nullptr;

	Light basicLight;

	// Camera
	Camera* _camera;
	float _cameraOrbitRadius = 7.0f;
	float _cameraOrbitRadiusMin = 2.0f;
	float _cameraOrbitRadiusMax = 50.0f;
	float _cameraOrbitAngleXZ = -90.0f;
	float _cameraSpeed = 2.0f;

	UINT _WindowHeight;
	UINT _WindowWidth;

	// Render dimensions - Change here to alter screen resolution
	UINT _renderHeight = 1080;
	UINT _renderWidth = 1920;

	ID3D11DepthStencilState* DSLessEqual;
	ID3D11RasterizerState* RSCullNone;

	ID3D11RasterizerState* CCWcullMode;
	ID3D11RasterizerState* CWcullMode;

	// Material Types
	Material shinyMaterial;
	Material noSpecMaterial;

	// Game Objects
	Plane* myPlane;
	Plane* AIplane;


	// Other Objects
	Geometry cubeGeometry;
	GameObject* slidingCube;
	vector< GameObject* > _cubes;
	vector< GameObject* > bullets;
	int bulletIndex;
	GameObject* raceTrack;
	GameObject* groundPlane;
	GameObject* mountain;
	GameObject* skyBox;

	//ball
	vector <GameObject*> bArray;
	GameObject* ball;
	GameObject* ball2;
	GameObject* ball3;
	GameObject* ball4;
	GameObject* ball5;
	GameObject* ball6;
	GameObject* ball7;

	//snow
	
	GameObject* snow; GameObject* snow2;GameObject* snow3;GameObject* snow4;GameObject* snow5;GameObject* snow6;
	GameObject* snow7;GameObject* snow8;GameObject* snow9;GameObject* snow10;GameObject* snow11;GameObject* snow12;GameObject* snow13;GameObject* snow14;
	GameObject* snow15;GameObject* snow16;GameObject* snow17;GameObject* snow18;GameObject* snow19;GameObject* snow20;GameObject* snow21;GameObject* snow22;
	GameObject* snow23;GameObject* snow24;GameObject* snow25;GameObject* snow26;GameObject* snow27;GameObject* snow28;GameObject* snow29;GameObject* snow30;
	
	vector<SnowParticle*> snowArray;
	SnowParticle* snowP; SnowParticle* snow2P; SnowParticle* snow3P; SnowParticle* snow4P; SnowParticle* snow5P; SnowParticle* snow6P;
	SnowParticle* snow7P; SnowParticle* snow8P; SnowParticle* snow9P; SnowParticle* snow10P; SnowParticle* snow11P; SnowParticle* snow12P; SnowParticle* snow13P; SnowParticle* snow14P;
	SnowParticle* snow15P; SnowParticle* snow16P; SnowParticle* snow17P; SnowParticle* snow18P; SnowParticle* snow19P; SnowParticle* snow20P; SnowParticle* snow21P; SnowParticle* snow22P;
	SnowParticle* snow23P; SnowParticle* snow24P; SnowParticle* snow25P; SnowParticle* snow26P; SnowParticle* snow27P; SnowParticle* snow28P; SnowParticle* snow29P; SnowParticle* snow30P;

	// Waypoints
	vector < GameObject* > waypoints;
	GameObject* waypoint1;
	GameObject* waypoint2;
	GameObject* waypoint3;
	GameObject* waypoint4;
	GameObject* waypoint5;

	// Cameras
	Camera* cameraMain;
	Camera* cameraPlaneMain;
	Camera* cameraPlaneAlternate;
	Camera* cameraTopView;
	Camera* cameraPlanePerspective;
	Camera* normPerspective;

	XMFLOAT3 cameraPlanePos;

	Camera* cameraCurrent;

	int camNum;

	float eyeX;
	float eyeY;
	float eyeZ;

private:
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	HRESULT InitDevice();
	void Cleanup();
	HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);
	HRESULT InitShadersAndInputLayout();
	HRESULT InitVertexBuffer();
	HRESULT InitIndexBuffer();

	// From Semester 1
	void InitObjects();
	void InitPlaneObjects();
	void InitAIPlaneObjects();
	void PlaneUpdate(float t);
	void CameraInput(float t);
	void bPops();
	void sPops();
	void MoveForward(int objectNumber);

	// Draw Methods
	void CarDraw(ConstantBuffer* cb, ID3D11DeviceContext* _pImmediateContext);
	void CubeDraw(ConstantBuffer* cb, ID3D11DeviceContext* _pImmediateContext);
	void OtherDraw(ConstantBuffer* cb, ID3D11DeviceContext* _pImmediateContext);

public:
	Application();
	~Application();

	HRESULT Initialise(HINSTANCE hInstance, int nCmdShow);

	bool HandleKeyboard(MSG msg);

	void Update(float t);
	void Draw();
};

