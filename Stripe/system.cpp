#include <windows.h>
#include "system.h"
#include "winmain.h"

void SysShutdown(void)
{
	wglMakeCurrent(hdc1, NULL);		// release device context
	wglMakeCurrent(hdc2, NULL);
	wglMakeCurrent(hdc3, NULL);
	wglDeleteContext(hglrc);		// delete rendering context
	PostQuitMessage(0);				// make sure the window will be destroyed
}
