// This file defines the graphical overlay
#include <iostream>
#include <windows.h>
#include <d3d9.h>
#include <Dwmapi.h> 
#include <TlHelp32.h>  
#include <d3dx9.h>
#include <fstream>
#include <string>
#include <vector>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "d3dx9.lib")

#define Overlay_Intensity 300 // Number of "#"
#define Overlay_Font 100
#define Overlay_Time 1000
//#define Overlay_Intensity 200 // Number of "#"
//#define Overlay_Font 300
//#define Overlay_Time 1000

#define Overlay_Texture "."
#define Show_Number true
#define Color_Adaptation true
//#define COL_R 66
//#define COL_G 94
//#define COL_B 71
#define COL_R 205
#define COL_G 201
#define COL_B 201
using namespace std;

vector <int> x_pos;
vector <int> y_pos;

bool centered_pattern = 0; // 0 for Random, 1 for Gaussian
extern float STRESS;
int font_sz = Overlay_Font;
char *texture = Overlay_Texture;

int s_width = 800;
int s_height = 600;

bool Overlay_1st = true;
HINSTANCE hInstance;
HINSTANCE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;

#define CENTERX (GetSystemMetrics(SM_CXSCREEN)/2)-(s_width/2)
#define CENTERY (GetSystemMetrics(SM_CYSCREEN)/2)-(s_height/2)
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;
HWND hWnd;
const MARGINS  margin = { 0, 0, s_width, s_height };
LPD3DXFONT pFont;

int clr_R = COL_R, clr_G = COL_G, clr_B = COL_B;
HDC hDC;
COLORREF clr;
extern double Guassian (double mu, double sigma);
extern float Actual_BR;
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		DwmExtendFrameIntoClientArea(hWnd, &margin);
	}break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

	D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

	ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
	d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;     // set the back buffer format to 32-bit
	d3dpp.BackBufferWidth = s_width;    // set the width of the buffer
	d3dpp.BackBufferHeight = s_height;    // set the height of the buffer

	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	// create a device class using this information and the info from the d3dpp struct
	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	D3DXCreateFont(d3ddev, font_sz, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, (LPCWSTR)"Arial", &pFont);

}

void ApplyPixelColor(int x, int y)
{
	clr = ::GetPixel(hDC, x, y); // Get color from a pixel
	clr_R = GetRValue(clr);
	clr_G = GetGValue(clr);
	clr_B = GetBValue(clr);
}

int DrawFont(int x, int y, LPD3DXFONT g_pFont, char* fmt)
{
    RECT Font;
    Font.bottom = 0;
    Font.left = x - font_sz/2;
    Font.top = y - font_sz/2;
    Font.right = 0;
	if (Color_Adaptation)
		g_pFont->DrawTextA(0, fmt, strlen(fmt), &Font, DT_NOCLIP, D3DCOLOR_ARGB(255, int(clr_R - STRESS * 0.8 * clr_R), int(clr_G - STRESS * 0.8 * clr_G), int(clr_B - STRESS * 0.8 * clr_B)));
	else
		g_pFont->DrawTextA(0, fmt, strlen(fmt), &Font, DT_NOCLIP, D3DCOLOR_ARGB(255, clr_R, clr_G, clr_B));
    return 0;
}

// Reder the stuff on the overlay
void render()
{
	// clear the window alpha
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	d3ddev->BeginScene();    // begins the 3D scene
	if (OverlayActive)
	{
		x_pos.clear();
		y_pos.clear();
		//RANDOM
		//Generate the positions of the interference
		if (STRESS > 0)
		{
			for (int i = 0; i < intensity * Overlay_Intensity * STRESS * 6; i++)
			{
				if (half_screen)
					x_pos.push_back((s_width / 1.28) + (rand()/(RAND_MAX + 1.0) - 0.5) * 2 * s_width / 4 - 90);
				else
					x_pos.push_back((s_width / 1.88) + (rand()/(RAND_MAX + 1.0) - 0.5) * 2 * s_width / 2);
				y_pos.push_back(s_height / 2.5 + (rand()/(RAND_MAX + 1.0) - 0.5) * 2 * s_height / 2 + 100);
				//ApplyPixelColor(x_pos[i],y_pos[i]);
				DrawFont(x_pos[i],y_pos[i], pFont, texture);
			}
		}
		//GAUSSIAN
		//if(centered_pattern && STRESS > 0)
		//{
		//	for (int i = 0; i < intensity * Overlay_Intensity * (0.9 + 0.1 * STRESS) * 7; i++)
		//	{
		//		if (half_screen)
		//			x_pos.push_back(int(Guassian((s_width / 1.28), 500)));
		//		else
		//			x_pos.push_back(int(Guassian((s_width / 1.88), 500)));
		//		y_pos.push_back(int(Guassian((s_height / 2.5), 300)));
		//		DrawFont(x_pos[i],y_pos[i], pFont, texture);
		//	}
		//}
	}
	//Draw breathing rate on the screen
	char BR[512];
	if (Show_Number)
		sprintf(BR,"%d",int(Actual_BR));
	else
		sprintf(BR,"",int(Actual_BR));
	RECT Font;
	Font.bottom = 0;
	Font.left = s_width / 1.35 - font_sz/20;
	Font.top = s_height - font_sz / 1.2;
	Font.right = 0;
	if (Actual_BR > 8)
		pFont->DrawTextA(0, BR, strlen(BR), &Font, DT_NOCLIP, D3DCOLOR_ARGB(255, 160, 20, 15));
	else
		pFont->DrawTextA(0, BR, strlen(BR), &Font, DT_NOCLIP, D3DCOLOR_ARGB(255, 200, 200, 200));

	d3ddev->EndScene();    // ends the 3D scene
	d3ddev->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
	if (Overlay_1st == false)
		Sleep(Overlay_Time);
	else
		Overlay_1st = false;
}

