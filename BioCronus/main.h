#include <winsock2.h>
#include <ws2bth.h>
#include <BluetoothAPIs.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <bthsdpdef.h>
#include <Ws2bth.h>
#include <bluetoothapis.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <regex>
#include <list>
#include <vector>
#include <algorithm>
#include <math.h>
#include <chrono>
#include <deque>
#include <cstdlib>
#include <sys/types.h>
#include <sys/timeb.h>
#include <ctime>
#include <tchar.h>
#include <strsafe.h>
#include <tuple>
#include <iomanip>
#include <random>
#include <time.h>
#include <direct.h>
#include <string.h>
#include <atlstr.h>
#include<ctime> 
#include <numeric>
#include <fcntl.h>
#include <io.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "irprops.lib")
#pragma comment(lib, "bthprops.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "Comctl32.lib")

using namespace std;
#define Average_BR			3//average Breathing rate within 3 minutes 
#define JitterTimeOut		500//0.1 sec
#define LNoJitterTimeOut	5000// Time without jittering when stress is low
#define HNoJitterTimeOut	1000// Time without jittering when stress is high
#define BreakTimeOut		1500//1.5 sec
#define NoBreakTimeOut		9000//10 sec
#define Recovery_Time		15//If stress keeps falling down for Recovery_Time, then cancel the penalty
#define NormalPlayTimeOut	0//10000//10 sec
#define LoggingTimeOut		100//0.1 sec
#define Max_Jitter_P		1//50% Maximum Jitter Probability
#define Sensitivity			3//1 for normal
#define MSG_DELIMITER				'|'
#define PI					3.1415926535897

// interference mode, 0 for none, 1-6 for different interference modes
int I_mode = 0; 
string buttons[] = {"Xbox","Back", "Start","RB","RT","RS","LB","LT","LS","RX","RY","LX","LY",
	"D-Up","D-Down","D-Left","D-Right","Y","B","A","X"};

ofstream event_file, controller_file, zephyr_file;

vector<int> Ave_RR;

float intensity = 1; //the intensity of biofeedback penalty
int recovery_time = Recovery_Time;
bool TIME = 1, NormalPlay = 1, Logging = 0;
bool Break = 0;
float STRESS = 0, Actual_BR = 0;
int session = 1;
double JitterDegree = 0, Jitter = 0;
char *manual_stress, *manual_recovery, *manual_intensity;

extern bool centered_pattern;

bool Zephyr_Connected = false, Cronus_Connected = false, OverlayActive = false, LogActive = false, PunishActive = false, ManualMode = false, ClockActive = false,
	isAlive=true, alive = true, program_alive = true, half_screen = true;;

clock_t start_time; 
time_t now = time(NULL);
struct tm *aTime = localtime(&now);
int currentYear = aTime->tm_year + 1900;
SYSTEMTIME sys;
SOCKET connectToZephyr();
SOCKET zephyr = NULL;

string ConvertWCHARToString(WCHAR * n)
{
	WCHAR *name = n;
	char ch[248];
	char DefChar = ' ';
	WideCharToMultiByte(CP_ACP,0,n,-1, ch,260,&DefChar, NULL);
	string ss(ch);
	return ss;
}
char *Concatenate(char *str1, char *str2)
{
	char *file = (char *) malloc(1 + strlen(str1)+ strlen(str2));
	strcpy(file, str1);
	strcat(file, str2);
	return file;
}
CString int_to_CString(int i)
{
	CString t;
	t.Format(_T("%d"), i);
	return t;
}
std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}
double Guassian (double mu, double sigma)
{
  double U1, U2, W, mult;
  static double X1, X2;
  static int call = 0;

  if (call == 1)
    {
      call = !call;
      return (mu + sigma * (double) X2);
    }

  do
    {
      U1 = -1 + ((double) rand () / RAND_MAX) * 2;
      U2 = -1 + ((double) rand () / RAND_MAX) * 2;
      W = pow (U1, 2) + pow (U2, 2);
    }
  while (W >= 1 || W == 0);

  mult = sqrt ((-2 * log (W)) / W);
  X1 = U1 * mult;
  X2 = U2 * mult;

  call = !call;

  return (mu + sigma * (double) X1);
}

DWORD WINAPI BreakThreadFunc(LPVOID lpParam)
{
	while (TIME)
	{
		Break = false;
		if (STRESS != 0)
		{
			Sleep(NoBreakTimeOut * (1 - STRESS) + BreakTimeOut * 2);
		}
		Break = true;
		Sleep(BreakTimeOut);
	}
	return 0;
}
DWORD WINAPI NormalPlayThreadFunc(LPVOID lpParam)
{
	Sleep(NormalPlayTimeOut);
	NormalPlay=0;
	return 0;
}
DWORD WINAPI LogTimeThreadFunc(LPVOID lpParam)
{
	while (TIME)
	{
		Sleep(LoggingTimeOut);
		Logging = 1;
	}
	return 0;
}
DWORD WINAPI JitterThreadFunc(LPVOID lpParam)
{
	double SineDegree = 0;
	float normal_slot, jitter_slot;
	while (TIME)
	{
		Jitter = 0;
		normal_slot = Guassian((HNoJitterTimeOut - LNoJitterTimeOut) * STRESS + LNoJitterTimeOut, HNoJitterTimeOut / 2);
		jitter_slot = (JitterTimeOut - 200) * STRESS + 200;
		if (normal_slot < 0)
			normal_slot = 0;
		Sleep(normal_slot);
		if (STRESS != 0)
		{
			//JitterByTimeSlot//Sleep(NoJitterTimeOut - STRESS * (NoJitterTimeOut - JitterTimeOut));
			if (rand()/(RAND_MAX + 1.0) < Max_Jitter_P)// * STRESS)//Decide Jitter or not
			{
				if (rand()/(RAND_MAX + 1.0) > 0.5)
				{
					for (int i =0; i < 50; i++)
					{
						SineDegree = SineDegree + PI / 50;
						Sleep(jitter_slot / 50);
						Jitter = sin(SineDegree);
					}
					SineDegree = 0;
					//Jitter = 1;
					//Sleep(JitterTimeOut);
				}
				else
				{
					for (int i =0; i < 50; i++)
					{
						SineDegree = SineDegree + PI / 50;
						Sleep(jitter_slot / 50);
						Jitter = -sin(SineDegree);
					}
					SineDegree = 0;
					//Jitter = -1;
					//Sleep(JitterTimeOut);
				}
			}
		}
	}
	return 0;
}

