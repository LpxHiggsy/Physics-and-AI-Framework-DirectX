#include "Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Time Measurement
//	DWORD frameProcessingTime = GetCurrentTime();
	DWORD ElapsedTime = 0;
	DWORD timeStep = 25;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	Application* theApp = new Application();

	if (FAILED(theApp->Initialise(hInstance, nCmdShow)))
	{
		return -1;
	}

    // Main message loop
    MSG msg = {0};

    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
			bool handled = false;

			if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST)
			{
				handled = theApp->HandleKeyboard(msg);
			}
			else if (WM_QUIT == msg.message)
				break;

			if (!handled)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
        }
        else
        {
			float frameStartTime = (float)GetTickCount();

			theApp->Update((float)timeStep);
			theApp->Draw();

			ElapsedTime = GetTickCount() - (DWORD)frameStartTime;

			if (ElapsedTime < timeStep)
			{
				Sleep(timeStep - ElapsedTime);
			}
        }
    }

	delete theApp;
	theApp = nullptr;

    return (int) msg.wParam;
}