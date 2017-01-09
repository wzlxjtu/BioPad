#include <windows.h>
#include <windowsx.h>
#include <string>
#include <Dwmapi.h> 

// define the GUI parameters
HINSTANCE hInstance;
HINSTANCE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;
HWND hWnd;
HDC hDC;
LPCWSTR g_szClassName = L"MainWindow";
TCHAR user_name[50], stress_str[10], recovery_str[10], intensity_str[10];
HWND Log_Button, Name_Box, User_Button, Zephyr_Button, Cronus_Button, Punish_Button, Manual_Button, Manual_Box, Jitter_Button, Break_Button, Speed_Button, Increase_Button,
	Decrease_Button, Clear_Button, Sensitive_Button, HRV_Box, RR_Box, HR_Box, AVE_Box, hwndPB, Time_Box, Device_Box, Recovery_Box, Recovery_Button, Intensity_Box, Intensity_Button;

DWORD Interface_thread_ID, Cronus_thread_ID, Overlay_thread_ID, CenteredOverlay_thread_ID, NormalPlay_thread_ID, Zephyr_thread_ID, LogFile_thread_ID, LogTime_ID, Break_thread_ID, Jitter_thread_ID, OverlayPattern_ID;
HANDLE Interface_thread, Cronus_thread, Overlay_thread, CenteredOverlay_thread, NormalPlay_thread, Zephyr_thread, LogFile_thread, Break_thread, Jitter_thread, OverlayPattern_thread, LogTimeHandle;
HINSTANCE InterfaceInst = NULL, OverlayInst = NULL, CenteredOverlayInst = NULL, NormalPlayInst = NULL, LogFileInst = NULL;

std::string TCHAR2STRING(TCHAR *STR)
{
 int iLen = WideCharToMultiByte(CP_ACP, 0,STR, -1, NULL, 0, NULL, NULL);
 char* chRtn =new char[iLen*sizeof(char)];
 WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
 std::string str(chRtn);
 return str;
}

// Step 4: call the Window Process
LRESULT CALLBACK GUIProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
