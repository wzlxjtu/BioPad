// BioPad is a tool to leverage off-the-shelf video games for biofeedback training.
// The software is written in C++.

#include "main.h"
#include "Zephyr.h"
#include "Cronus.h"
#include "Interface.h"
#include "calc.h"


// This thread is for the GUI
DWORD WINAPI InterfaceThreadFunc(LPVOID lpParam)
{
	WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = GUIProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        L"BioCronus",
        WS_MINIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
		400, 540,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
	return 0;
}

// This thread is for processing the Controller data from Cronus
DWORD WINAPI CronusThreadFunc(LPVOID lpParam)
{
	HINSTANCE cronus = (HINSTANCE)lpParam;
	int8_t output[GCAPI_OUTPUT_TOTAL] = { 0 };
	GCAPI_REPORT report;
	//  Load the Cronus API library
	CronusInst = LoadLibrary(TEXT("gcdapi.dll"));
	if (CronusInst == NULL) {
		cout << "Error loading gcdapi.dll" << endl;
		return NULL;
	}
	//  Set up pointers to API functions
	gcapi_IsConnected = (GCAPI_IsConnected)GetProcAddress(CronusInst, "gcapi_IsConnected");
	gcapi_GetFWVer = (GCAPI_GetFWVer)GetProcAddress(CronusInst, "gcapi_GetFWVer");
	gcapi_Read = (GCAPI_Read)GetProcAddress(CronusInst, "gcapi_Read");
	gcapi_Write = (GCAPI_Write)GetProcAddress(CronusInst, "gcapi_Write");
	gcapi_GetTimeVal = (GCAPI_GetTimeVal)GetProcAddress(CronusInst, "gcapi_GetTimeVal");
	gcapi_CalcPressTime = (GCAPI_CalcPressTime)GetProcAddress(CronusInst, "gcapi_CalcPressTime");
	gcdapi_Load = (GCDAPI_Load)GetProcAddress(CronusInst, "gcdapi_Load");
	gcdapi_Unload = (GCDAPI_Unload)GetProcAddress(CronusInst, "gcdapi_Unload");
	//  Error Check
	if (gcdapi_Load == NULL || gcdapi_Unload == NULL || gcapi_IsConnected == NULL || gcapi_GetFWVer == NULL ||
		gcapi_Read == NULL || gcapi_Write == NULL || gcapi_GetTimeVal == NULL || gcapi_CalcPressTime == NULL) {
		FreeLibrary(CronusInst);
		cout << "Error setting up gcdapi.dll functions" << endl;
		return NULL;
	}

	//  Allocate resources and initialize the API
	if (!gcdapi_Load()) {
		FreeLibrary(CronusInst);
		cout << "Error initializing Cronus API" << endl;
		return NULL;
	}

	SYSTEMTIME syst;

	while (Cronus_Connected)
	{
		gcapi_Read(&report);

		for (uint8_t i = 0; i<GCAPI_INPUT_TOTAL; i++)
		{
			output[i] = report.input[i].value;
		}

		if (Logging && LogActive)
		{
			GetLocalTime(&syst);
			controller_file << "[0 TIME " << syst.wMonth << "/" << syst.wDay << "/" << syst.wYear << " " << syst.wHour << ":" << syst.wMinute
				<< ":" << syst.wSecond << "]\n[Stress] " << STRESS << endl;
			for (int i = 1; i < 21; i++)
			{
				controller_file << "[" << buttons[i] << "]      " << int(output[i]) << endl;
			}
		}
		Logging = 0;
		if (!NormalPlay)
		{
			// Select different adaptation function here.
			switch (I_mode)
			{
			case 0:
				break;
			case 1:
				Jittered_Mod(output, STRESS);
				break;
			case 2:
				Break_Mod(output, STRESS);
				break;
			case 3:
				Speed_Only(output, STRESS);
				break;
			case 4:
				Sensitive_Mod(output, STRESS, Sensitivity);
				break;
			case 5:
				Speed_Increase(output, STRESS);
				break;
			case 6:
				Speed_Decrease(output, STRESS);
				break;
			default:
				break;
			}
		}		
		gcapi_Write(output);
		//processControllerSignals(report);
	}
	//  Deallocate API resources and unload the library
	gcdapi_Unload();
	return 0;
}


// This is the thread for logging user information and data
DWORD WINAPI LogFileThreadFunc(LPVOID lpParam)
{
	GetWindowText(Name_Box, user_name, sizeof(user_name)-1);
	char* USER = new char[TCHAR2STRING(user_name).length()+1];
	strcpy(USER, TCHAR2STRING(user_name).c_str());

	_mkdir(USER);
	string EVENTS_FILE = Concatenate(USER,"\\Events Log.txt");
	string CONTROLLER_FILE = Concatenate(USER,"\\Controller Log.txt");
	string ZEPHYR_FILE = Concatenate(USER,"\\Zephyr Data Log.txt");
	event_file.open(EVENTS_FILE);
	controller_file.open(CONTROLLER_FILE);
	zephyr_file.open(ZEPHYR_FILE);

	GetLocalTime(&sys);
	event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
		<< " " << sys.wSecond << " " << "Initialize Logging" << endl;
	return 0;
} 


// This is the main function
int WINAPI WinMain(HINSTANCE hInstance_t,
	HINSTANCE hPrevInstance_t,
	LPSTR lpCmdLine_t,
	int nCmdShow_t)
{

#pragma region Head
	hInstance = hInstance_t;
	hPrevInstance = hPrevInstance_t;
	lpCmdLine = lpCmdLine_t;
	nCmdShow = nCmdShow_t;
	
	Interface_thread = CreateThread(NULL, 0, InterfaceThreadFunc, (LPVOID)InterfaceInst, 0, &Interface_thread_ID);
	int year, heartrate, breathrate, hrv;
	int BR_OLD = 0, BR_NEW = 0;
	int count = 0;
	const int BUF_LEN = 60;
	char recvBuf[BUF_LEN];
	int relax_times = 0;
	bool RESP_INIT = 1;
	bool discard = false;
	int RESP_COUNT = 0;
	float RR_average;
#pragma endregion
#pragma region Zephyr to STRESS
	while (program_alive)
	{
		SendMessage(hwndPB, PBM_SETPOS, STRESS * 100, 0);
		if (ClockActive)
			Button_SetText(Time_Box,int_to_CString((clock()-start_time)/CLOCKS_PER_SEC));
		if (Zephyr_Connected)
		{
			count = recv(zephyr, recvBuf, BUF_LEN, 0);
			if (count > 0)
			{
				year = ZephUtils.mergeUnsigned(recvBuf[4], recvBuf[5]);
				heartrate = ZephUtils.mergeUnsigned(recvBuf[13], recvBuf[14]);
				breathrate = ZephUtils.mergeUnsigned(recvBuf[15], recvBuf[16]);
				hrv = ZephUtils.mergeUnsigned(recvBuf[38], recvBuf[39]);
				if (year == currentYear &&
					heartrate <256 && heartrate >-1 &&
					breathrate <256 && breathrate >-1)
				{
					Actual_BR = breathrate*1.0 / 10;
					// Calculate the average breathing rate within 60s
					Ave_RR.push_back(Actual_BR);
					if (Ave_RR.size() > Average_BR * 60)
						 Ave_RR.erase(Ave_RR.begin());
				    RR_average = accumulate( Ave_RR.begin(), Ave_RR.end(), 0.0 )/ Ave_RR.size();
				    
					Button_SetText(HRV_Box,int_to_CString(hrv));
					Button_SetText(HR_Box,int_to_CString(heartrate));
					Button_SetText(RR_Box,int_to_CString(Actual_BR));
					if (RR_average > L_RR)
						Button_SetText(AVE_Box,int_to_CString(RR_average + 0.5));
					else
						Button_SetText(AVE_Box,L"Finished!");
					
					if (BR_NEW != breathrate)
					{
						BR_NEW = breathrate;

						if (!ManualMode)
							STRESS = RESP2STRESS(Actual_BR);
						
						if (LogActive && hrv != 65535)
							zephyr_file << "[1 HRV] " << hrv << "\n[2 HEART RATE] " << heartrate << "\n[3 BREATH RATE] " << breathrate << endl;

						//If breathing rate is going down for several consecutive seconds, we won't give penalty
						if (BR_OLD > BR_NEW)
						{
							relax_times++;
							if (relax_times > recovery_time - 1)
								STRESS = 0;
						}
						else
							relax_times = 0;

						BR_OLD = BR_NEW;
					}
				}
			}
		}
		else
		{
			Button_SetText(HRV_Box,L"#");
			Button_SetText(HR_Box,L"#");
			Button_SetText(RR_Box,L"#");
			Button_SetText(AVE_Box,L"#");
			Sleep(1000);
		}
	}
#pragma endregion
	return true;
}


// This defines the GUI buttons and functions
LRESULT CALLBACK GUIProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
		case WM_CREATE:
#pragma region Window_Elements
			Name_Box = CreateWindow(L"EDIT",L"USER",
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				30, 30, 100, 20,
				hwnd, NULL, NULL, NULL);
			User_Button = CreateWindow(L"Button", L"SET",
				WS_VISIBLE | WS_CHILD,
				135, 30, 40, 20,
				hwnd, (HMENU) 'u', NULL, NULL);
			CreateWindow(L"STATIC", L"Sensor: ", WS_VISIBLE | WS_CHILD, 30, 72, 80, 30, hwnd, NULL, NULL, NULL);
			Zephyr_Button = CreateWindow(L"Button", L"Connect",
				WS_VISIBLE | WS_CHILD,
				85, 65, 80, 30,
				hwnd, (HMENU) 'z', NULL, NULL);
			Device_Box = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 180, 67, 80, 30, hwnd, NULL, NULL, NULL);
			CreateWindow(L"STATIC", L"Manual Intensity: ", WS_VISIBLE | WS_CHILD, 30, 114, 130, 20, hwnd, NULL, NULL, NULL);
			Manual_Box = CreateWindow(L"EDIT",L"1",
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				150, 113, 40, 20,
				hwnd, NULL, NULL, NULL);
			Manual_Button = CreateWindow(L"Button", L"Activate",
				WS_VISIBLE | WS_CHILD,
				200, 108, 80, 30,
				hwnd, (HMENU) 'm', NULL, NULL);
			CreateWindow(L"STATIC", 
				L" CRONUS                               LOG  ", 
				WS_VISIBLE | WS_CHILD, 40, 155, 400, 20, hwnd, NULL, NULL, NULL);
			Cronus_Button = CreateWindow(L"Button", L"Connect",
				WS_VISIBLE | WS_CHILD,
				34, 175, 80, 30,
				hwnd, (HMENU) 'c', NULL, NULL);
			Log_Button = CreateWindow(L"Button", L"Start",
				WS_VISIBLE | WS_CHILD,
				200, 175, 80, 30,
				hwnd, (HMENU) 'l', NULL, NULL);

			CreateWindow(L"STATIC", L"Interference Mode:", WS_VISIBLE | WS_CHILD, 34, 220, 150, 40, hwnd, NULL, NULL, NULL);
			Jitter_Button = CreateWindow(L"Button", L"Jitter",
				WS_VISIBLE | WS_CHILD,
				34, 240, 50, 20,
				hwnd, (HMENU) '1', NULL, NULL);
			Break_Button = CreateWindow(L"Button", L"Break",
				WS_VISIBLE | WS_CHILD,
				94, 240, 60, 20,
				hwnd, (HMENU) '2', NULL, NULL);
			Speed_Button = CreateWindow(L"Button", L"Speed",
				WS_VISIBLE | WS_CHILD,
				34, 265, 50, 20,
				hwnd, (HMENU) '3', NULL, NULL);
			Sensitive_Button = CreateWindow(L"Button", L"Sensitivity",
				WS_VISIBLE | WS_CHILD,
				89, 265, 80, 20,
				hwnd, (HMENU) '4', NULL, NULL);
			Increase_Button = CreateWindow(L"Button", L"Increase",
				WS_VISIBLE | WS_CHILD,
				24, 290, 70, 20,
				hwnd, (HMENU) '5', NULL, NULL);
			Decrease_Button = CreateWindow(L"Button", L"Decrease",
				WS_VISIBLE | WS_CHILD,
				98, 290, 70, 20,
				hwnd, (HMENU) '6', NULL, NULL);
			Clear_Button = CreateWindow(L"Button", L"Clear",
				WS_VISIBLE | WS_CHILD,
				105, 315, 50, 20,
				hwnd, (HMENU) '0', NULL, NULL);

			Button_Enable(Log_Button, false);
			CreateWindow(L"STATIC", L"TIME:", WS_VISIBLE | WS_CHILD, 180, 235, 40, 20, hwnd, NULL, NULL, NULL);
			Time_Box = CreateWindow(L"STATIC", L"0", WS_VISIBLE | WS_CHILD, 230, 235, 40, 20, hwnd, NULL, NULL, NULL);
			CreateWindow(L"STATIC", L"   RR:", WS_VISIBLE | WS_CHILD, 180, 255, 40, 20, hwnd, NULL, NULL, NULL);
			RR_Box = CreateWindow(L"STATIC", L"#", WS_VISIBLE | WS_CHILD, 230, 255, 30, 20, hwnd, NULL, NULL, NULL);
			CreateWindow(L"STATIC", L"  AVE:", WS_VISIBLE | WS_CHILD, 255, 255, 40, 20, hwnd, NULL, NULL, NULL);
			AVE_Box = CreateWindow(L"STATIC", L"#", WS_VISIBLE | WS_CHILD, 300, 255, 60, 20, hwnd, NULL, NULL, NULL);
			CreateWindow(L"STATIC", L"   HR:", WS_VISIBLE | WS_CHILD, 180, 275, 40, 20, hwnd, NULL, NULL, NULL);
			HR_Box = CreateWindow(L"STATIC", L"#", WS_VISIBLE | WS_CHILD, 230, 275, 40, 20, hwnd, NULL, NULL, NULL);
			CreateWindow(L"STATIC", L" HRV:", WS_VISIBLE | WS_CHILD, 180, 295, 40, 20, hwnd, NULL, NULL, NULL);
			HRV_Box = CreateWindow(L"STATIC", L"#", WS_VISIBLE | WS_CHILD, 230, 295, 40, 20, hwnd, NULL, NULL, NULL);
			CreateWindow(L"STATIC", L"Intensity:", WS_VISIBLE | WS_CHILD, 30, 330, 60, 20, hwnd, NULL, NULL, NULL);
			hwndPB = CreateWindowEx(0, PROGRESS_CLASS, NULL,
				WS_VISIBLE | WS_CHILD | PBS_SMOOTH | WS_BORDER,
				30, 350, 325, 35,
				hwnd, NULL, hInstance, NULL);
			SendMessage(hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessage(hwndPB, PBM_SETSTEP, (WPARAM) 1, 0);
			CreateWindow(L"STATIC", L"Penalty Intensity: ", WS_VISIBLE | WS_CHILD, 130, 407, 120, 20, hwnd, NULL, NULL, NULL);
			Intensity_Box = CreateWindow(L"EDIT",int_to_CString(intensity),
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				250, 406, 30, 20,
				hwnd, NULL, NULL, NULL);
			Intensity_Button = CreateWindow(L"Button", L"Apply",
				WS_VISIBLE | WS_CHILD,
				285, 401, 60, 30,
				hwnd, (HMENU) 'i', NULL, NULL);
			CreateWindow(L"STATIC", L"Recovery Time(s): ", WS_VISIBLE | WS_CHILD, 128, 441, 120, 20, hwnd, NULL, NULL, NULL);
			Recovery_Box = CreateWindow(L"EDIT",int_to_CString(Recovery_Time),
				WS_VISIBLE | WS_CHILD | WS_BORDER,
				250, 440, 30, 20,
				hwnd, NULL, NULL, NULL);
			Recovery_Button = CreateWindow(L"Button", L"Apply",
				WS_VISIBLE | WS_CHILD,
				285, 435, 60, 30,
				hwnd, (HMENU) 'r', NULL, NULL);			
		break;
#pragma endregion
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case 'u':
#pragma region User_Profile
					LogTimeHandle = CreateThread(NULL, 0, LogTimeThreadFunc, NULL, 0, &LogTime_ID);
					LogFile_thread = CreateThread(NULL, 0, LogFileThreadFunc, (LPVOID)LogFileInst, 0, &LogFile_thread_ID);
					Button_Enable(User_Button, false);
					Button_Enable(Log_Button, true);
					
				break;
#pragma endregion
				case 'z':
#pragma region Zephyr Button
					if (!Zephyr_Connected)
						{
						GetLocalTime(&sys);
						event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
							<< " " << sys.wSecond << " " << "Connecting to Zephyr!" << endl;
						Button_SetText(Zephyr_Button,L"Waiting...");
						zephyr = connectToZephyr();
						
						if (zephyr == NULL)
						{
							cout << "Failed to connect Zephyr, please try again\n" << endl;
							Button_SetText(Zephyr_Button,L"Connect");
							GetLocalTime(&sys);
							event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
							<< " " << sys.wSecond << " " << "Zephyr connection unsuccessful!" << endl;
						}
						else
						{
							Zephyr_thread = CreateThread(NULL, 0, zephyrTimeoutThreadFunc, (LPVOID)zephyr, 0, &Zephyr_thread_ID);
							Zephyr_Connected = true;
							//Button_Enable(Zephyr_Button, false);
							Button_SetText(Zephyr_Button,L"Disconnect");
							Button_SetText(Device_Box,s2ws(device_name).c_str());
							GetLocalTime(&sys);
							event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
							<< " " << sys.wSecond << " " << "Zephyr Connected Successfully!" << endl;
						}
					}
					else
					{
						closesocket(zephyr);
						Zephyr_Connected = false;
						zephyr = NULL;
						Button_SetText(Zephyr_Button,L"Connect");
						GetLocalTime(&sys);
							event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
							<< " " << sys.wSecond << " " << "Zephyr Disconnected!" << endl;
					}
				break;
#pragma endregion
				case 'c':
#pragma region Cronus Button
					if (!Cronus_Connected)
					{
						Cronus_Connected = true;
						Cronus_thread = CreateThread(NULL, 0, CronusThreadFunc, (LPVOID)CronusInst, 0, &Cronus_thread_ID);
						NormalPlay_thread = CreateThread(NULL, 0, NormalPlayThreadFunc, (LPVOID)NormalPlayInst, 0, &NormalPlay_thread_ID);
						Jitter_thread = CreateThread(NULL, 0, JitterThreadFunc, NULL, 0, &Jitter_thread_ID);
						Break_thread = CreateThread(NULL, 0, BreakThreadFunc, NULL, 0, &Break_thread_ID);
						Button_SetText(Cronus_Button,L"Disconnect");
						GetLocalTime(&sys);
						event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Cronus Connected!" << endl;
					}
					else
					{
						Cronus_Connected = false;
						Break = false;
						TerminateThread(Jitter_thread, 0);
						CloseHandle(Jitter_thread);
						TerminateThread(Break_thread, 0);
						CloseHandle(Break_thread);
						Button_SetText(Cronus_Button,L"Connect");
						GetLocalTime(&sys);
						event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Cronus Disconnected!" << endl;
					}
				break;
#pragma endregion
				case 'm':
#pragma region Set Manual Stress
					if (!ManualMode)
					{
						ManualMode = true;
						GetWindowText(Manual_Box, stress_str, sizeof(stress_str)-1);
						manual_stress = new char[TCHAR2STRING(stress_str).length()+1];
						strcpy(manual_stress, TCHAR2STRING(stress_str).c_str());
						STRESS = atof(manual_stress);
						Actual_BR = atof(manual_stress);
						if (STRESS > 1)
						{
							STRESS = 1;
							Button_SetText(Manual_Box,int_to_CString(1));
						}
						if (STRESS < 0)
						{
							STRESS = 0;
							Button_SetText(Manual_Box,int_to_CString(0));
						}
						Button_SetText(Manual_Button,L"Deactivate");
						event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Activate Manual Stress to " << STRESS << endl;
					}
					else
					{
						ManualMode = false;
						STRESS = 0;
						Button_SetText(Manual_Button,L"Activate");
						Button_SetText(Manual_Box,L"0");
						event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Deactivate Manual Stress" << endl;
					}
				break;
#pragma endregion
				case 'l':
#pragma region Log Button
					if (LogActive)
					{
						LogActive = false;
						ClockActive = false;
						Button_SetText(Log_Button,L"Start");
						GetLocalTime(&sys);
						event_file << session << " " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Logging Paused" << endl;
						session++;
					}
					else
					{
						start_time = clock(); 
						LogActive = true;
						ClockActive = true;
						Button_SetText(Log_Button,L"Pause");
						GetLocalTime(&sys);
						event_file << session << " " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Logging Started" << endl;
						controller_file << session << " " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Logging Started" << endl;
						zephyr_file << session << " " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Logging Started" << endl;
					}
				break;
#pragma endregion
				case 'r':
#pragma region Recovery Button
					GetWindowText(Recovery_Box, recovery_str, sizeof(recovery_str)-1);
					manual_recovery = new char[TCHAR2STRING(recovery_str).length()+1];
					strcpy(manual_recovery, TCHAR2STRING(recovery_str).c_str());
					recovery_time = atoi(manual_recovery);
					GetLocalTime(&sys);
					event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Change Recovery Time to " << recovery_time <<endl;
				break;
#pragma endregion
				case 'i':
#pragma region Intensity Button
					GetWindowText(Intensity_Box, intensity_str, sizeof(intensity_str)-1);
					manual_intensity = new char[TCHAR2STRING(intensity_str).length()+1];
					strcpy(manual_intensity, TCHAR2STRING(intensity_str).c_str());
					intensity = atof(manual_intensity);
					GetLocalTime(&sys);
					event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Changed Biofeedback Intensity to " << intensity << endl;
				break;
#pragma endregion
				case '0':
					I_mode = 0;
					break;
				case '1':
					I_mode = 1;
					break;
				case '2':
					I_mode = 2;
					break;
				case '3':
					I_mode = 3;
					break;
				case '4':
					I_mode = 4;
					break;
				case '5':
					I_mode = 5;
					break;
				case '6':
					I_mode = 6;
					break;
			}
		break;
#pragma region Other Messages
		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)CreateSolidBrush(RGB(255, 255, 255));
		}
        case WM_CLOSE:
			Cronus_Connected = false;
			GetLocalTime(&sys);
			event_file << "0 " << sys.wYear << " " << sys.wMonth << " " << sys.wDay << " " << sys.wHour << " " << sys.wMinute
						<< " " << sys.wSecond << " " << "Program Terminated" << endl;
			event_file.close();
			zephyr_file.close();
			controller_file.close();
            DestroyWindow(hwnd);
			system("taskkill /IM joycur.exe");
			program_alive = false;
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
#pragma endregion
    }
    return 0;
}