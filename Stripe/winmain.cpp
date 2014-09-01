/*
Main routine for OpenGl open and closed loop stripes
Dan Turner-Evans
08/20/14

winmain - create the windows and run the main loop
balloffset - programs to calculate the offset of the stripe for open and closed loop stripes
glmain - contains the OpenGl rendering code
system - system shutdown

Adapted from:
	www.thepixels.net
	by:    Greg Damon
	        gregd@thepixels.net
	and segments of Jim Strother's Win API code
*/

#include "glmain.h"
#include <Windows.h>
#include "winmain.h"
#include "system.h"
#include "balloffset.h"

/*
#include "winmain.h"
#include "system.h"
*/

// Open loop or closed loop
int closed;

// Create device contexts and window handles for three windows
HDC hdc1;
HDC hdc2;
HDC hdc3;
HWND hwnd1;
HWND hwnd2;
HWND hwnd3;

// Create OpenGl context
HGLRC hglrc;

// Let the while loop run
bool quit = false;

// Initialize the pixel format index
int indexPixelFormat = 0;

// CreateWnd: creates three full-screen windows, one on each projector
void CreateWnd(HINSTANCE &hinst, int width, int height, int depth)
{
	// Find the projectors
	POINT pt1, pt2, pt3;
	pt1.x = -SCRWIDTH / 2;
	pt2.x = -3 * SCRWIDTH / 2;
	pt3.x = -5 * SCRWIDTH / 2;
	pt1.y = pt2.y = pt3.y = 100;

	HMONITOR hmon1, hmon2, hmon3; // monitor handles
	hmon1 = MonitorFromPoint(pt1, MONITOR_DEFAULTTONEAREST);
	hmon2 = MonitorFromPoint(pt2, MONITOR_DEFAULTTONEAREST);
	hmon3 = MonitorFromPoint(pt3, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi1, mi2, mi3;
	mi1.cbSize = mi2.cbSize = mi3.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hmon1, &mi1);
	GetMonitorInfo(hmon2, &mi2);
	GetMonitorInfo(hmon3, &mi3);

	// Set the window position based on the projector locations
	int posx1 = mi1.rcMonitor.left;
	int posy1 = mi1.rcMonitor.top;
	int posx2 = mi2.rcMonitor.left;
	int posy2 = mi2.rcMonitor.top;
	int posx3 = mi3.rcMonitor.left;
	int posy3 = mi3.rcMonitor.top;

	// Constants for fullscreen mode
	long wndStyle = WS_POPUP | WS_VISIBLE;

	// create the windows
	hwnd1 = CreateWindowEx(NULL,
		WNDCLASSNAME,
		WNDNAME,
		wndStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		posx1, posy1,
		width, height,
		NULL,
		NULL,
		hinst,
		NULL);

	hdc1 = GetDC(hwnd1);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		SCRDEPTH,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		32,
		0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	indexPixelFormat = ChoosePixelFormat(hdc1, &pfd);
	SetPixelFormat(hdc1, indexPixelFormat, &pfd);

	// Setup OpenGL
	hglrc = wglCreateContext(hdc1);
	wglMakeCurrent(hdc1, hglrc);
	glewExperimental = GL_TRUE;
	glewInit();
	InitOpenGL();
	ShowWindow(hwnd1, SW_SHOW);		// everything went OK, show the window
	UpdateWindow(hwnd1);

	hwnd2 = CreateWindowEx(NULL,
		WNDCLASSNAME,
		WNDNAME,
		wndStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		posx2, posy2,
		width, height,
		NULL,
		NULL,
		hinst,
		NULL);

	hdc2 = GetDC(hwnd2);
	indexPixelFormat = ChoosePixelFormat(hdc2, &pfd);
	SetPixelFormat(hdc2, indexPixelFormat, &pfd);
	wglMakeCurrent(hdc2, hglrc);
	glewInit();
	ShowWindow(hwnd2, SW_SHOW);		// everything went OK, show the window
	UpdateWindow(hwnd2);

	hwnd3 = CreateWindowEx(NULL,
		WNDCLASSNAME,
		WNDNAME,
		wndStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		posx3, posy3,
		width, height,
		NULL,
		NULL,
		hinst,
		NULL);

	hdc3 = GetDC(hwnd3);
	indexPixelFormat = ChoosePixelFormat(hdc3, &pfd);
	SetPixelFormat(hdc3, indexPixelFormat, &pfd);
	wglMakeCurrent(hdc3, hglrc);
	glewInit();
	ShowWindow(hwnd3, SW_SHOW);		// everything went OK, show the window
	UpdateWindow(hwnd3);
}

// The event handler
LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			break;
		}
		case WM_DESTROY:
		{
			SysShutdown();
			break;
		}
		case WM_SIZE:
		{
			break;
		}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
}

// The main loop
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int nshowcmd)
{
	MSG msg;

	// Create a windows class for subsequently creating windows
	WNDCLASSEX ex;
	ex.cbSize = sizeof(WNDCLASSEX);
	ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	ex.lpfnWndProc = WinProc;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hinstance;
	ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = NULL;
	ex.lpszMenuName = NULL;
	ex.lpszClassName = WNDCLASSNAME;
	ex.hIconSm = NULL;

	if (!RegisterClassEx(&ex))
	{
		MessageBox(NULL, "Failed to register the windows class", "Window Reg Error", MB_OK);
		return 1;
	}

	// Create the windows
	CreateWnd(hinstance, SCRWIDTH, SCRHEIGHT, SCRDEPTH);

	// Ask if open or closed loop
	HWND hwndoc;

	//Switch from open loop to closed loop
	int	msgboxID = MessageBox(NULL, "Closed Loop?", "Open or Closed Loop", MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFAULT_DESKTOP_ONLY | MB_SETFOREGROUND);
	closed = 7 - msgboxID;
	if (closed != 0 || closed != 1)
	{
		quit == true;
	}

	// Prompt the user for a filename and directory
	OPENFILENAME ofn;       // common dialog box structure
	ZeroMemory(&ofn, sizeof(ofn));
	char szFile[260];       // buffer for file name
	HWND hwndsave;

	// create the save as window
	hwndsave = CreateWindowEx(NULL,
		WNDCLASSNAME,
		WNDNAME,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		100, 100,
		600, 600,
		NULL,
		NULL,
		hinstance,
		NULL);
	HANDLE hf;              // file handle

	szFile[0] = '\0';
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndsave;
	ofn.lpstrFilter = "Text\0*.TXT\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(*szFile);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = NULL;
	ofn.lpstrInitialDir = (LPSTR)NULL;
	ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = "Specify the filename";
	ofn.lpstrDefExt = "txt";

	GetSaveFileName(&ofn);

	std::string fname = ofn.lpstrFile;
	std::size_t pos = fname.find(".");
	std::string lightname = fname.substr(0,pos) + "_Stripe.txt";
	std::string darkname = fname.substr(0, pos) + "_Dark.txt";
	const char * lightfname = lightname.c_str();
	const char * darkfname = darkname.c_str();

	//Create a file to store the offset position at each point in time
	FILE *str;
	fopen_s(&str, lightfname, "w");
	FILE *strdark;
	fopen_s(&strdark, darkfname, "w");

	// Print local time as a string.
	char s[30];
	size_t i;
	struct tm tim;
	time_t now;
	now = time(NULL);
	localtime_s(&tim, &now);
	i = strftime(s, 30, "%b %d, %Y; %H:%M:%S\n", &tim);
	fprintf(str, "Current date and time: %s\n", s);
	fprintf(strdark, "Current date and time: %s\n", s);


	float dx0Now = 0.0f;
	float dx1Now = 0.0f;
	float dy0Now = 0.0f;
	float dy1Now = 0.0f;
	InitOffset();

	// The main loop
	while (!quit)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				quit = true;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		// Timestamp closed loop output in order to recreate later
		QueryPerformanceCounter(&li);
		float netTime = (li.QuadPart - CounterStart) / PCFreq;

		if ((netTime > 0 && netTime < 5 * 60) || (netTime > 10 * 60 && netTime < 15 * 60) || (netTime > 20 * 60 && netTime < 25 * 60))
		{
			wglMakeCurrent(hdc1, hglrc);
			RenderFrame(0);
			SwapBuffers(hdc1);
			wglMakeCurrent(hdc2, hglrc);
			RenderFrame(1);
			SwapBuffers(hdc2);
			wglMakeCurrent(hdc3, hglrc);
			RenderFrame(2);
			SwapBuffers(hdc3);

			// Timestamp closed loop output in order to recreate later
			QueryPerformanceCounter(&li);
			netTime = (li.QuadPart - CounterStart) / PCFreq;

			io_mutex.lock();
			dx0Now = dx0;
			dx1Now = dx1;
			dy0Now = dy0;
			dy1Now = dy1;
			io_mutex.unlock();

			//Print the elapsed time
			fprintf(str, "Elapsed time:\t%f\t", netTime);
			//Print the offset to the log file
			fprintf(str, "Offset:\t%f\t", BallOffsetNow);
			fprintf(str, "dx0:\t%f\t", dx0Now);
			fprintf(str, "dx1:\t%f\t", dx1Now);
			fprintf(str, "dy0:\t%f\t", dy0Now);
			fprintf(str, "dy1:\t%f\n", dy1Now);
		}
		else
		{
			wglMakeCurrent(hdc1, hglrc);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			SwapBuffers(hdc1);
			wglMakeCurrent(hdc2, hglrc);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			SwapBuffers(hdc2);
			wglMakeCurrent(hdc3, hglrc);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			SwapBuffers(hdc3);

			// Timestamp closed loop output in order to recreate later
			QueryPerformanceCounter(&li);
			netTime = (li.QuadPart - CounterStart) / PCFreq;

			io_mutex.lock();
			BallOffsetNow = BallOffset;
			dx0Now = dx0;
			dx1Now = dx1;
			dy0Now = dy0;
			dy1Now = dy1;
			io_mutex.unlock();

			//Print the elapsed time
			fprintf(strdark, "Elapsed time:\t%f\t", netTime);
			//Print the offset to the log file
			fprintf(strdark, "Offset:\t%f\t", BallOffsetNow);
			fprintf(strdark, "dx0:\t%f\t", dx0Now);
			fprintf(strdark, "dx1:\t%f\t", dx1Now);
			fprintf(strdark, "dy0:\t%f\t", dy0Now);
			fprintf(strdark, "dy1:\t%f\n", dy1Now);
		}
		

		if (GetAsyncKeyState(VK_ESCAPE) || (netTime > 30 * 60))
			SysShutdown();
	}
	GLShutdown();
	fclose(str);
	fclose(strdark);
	return msg.lParam;
}