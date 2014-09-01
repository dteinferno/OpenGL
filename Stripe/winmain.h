/*
Header for main routine
08/20/14
*/

#include <string>

// Define the projector dimensions
#define SCRWIDTH 720
#define SCRHEIGHT 1280
#define SCRDEPTH 32

// Define constants for the windows
#define WNDCLASSNAME "GLClass"
#define WNDNAME "Stripe"

// Define handles for the three windows, one for each projector
extern HDC hdc1; //HDC = handle device context
extern HDC hdc2;
extern HDC hdc3;
extern HWND hwnd1; //HWND = window handle
extern HWND hwnd2;
extern HWND hwnd3;

// Define OpenGl rendering context
extern HGLRC hglrc;

// Define index pixel format for the window
extern int indexPixelFormat;

// Define a constant to control the running of the main while loop
extern bool quit;

