// This file contains the Bluetooth API for comminucating with sensors

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <fstream>


string device_name = "";

struct ZephyrData
{
public:
	double BREATHRATE, HEARTRATE, HRV;

	bool EXISTS()
	{
		if(BREATHRATE > 0 && BREATHRATE < 701 && /*invalid = 6553.5*/
		   HEARTRATE > 0 && HEARTRATE < 241 && /*invalid = 65535*/
		   HRV > 0 && HRV < 256) /*invalid = 65535*/
		{
			return true;
		}
		return false;
	}

	ZephyrData()
	{
		//BREATHRATE = -1;
		//HEARTRATE = -1;
		//HRV = -1;
	}

	ZephyrData(double breathrate, double heartrate, double hrv)
	{
		BREATHRATE = breathrate;
		HEARTRATE = heartrate;
		HRV = hrv;
	}
};

struct zephyrCalibrateParams
{
public:
	SOCKET socket;
	bool isStressed;

	zephyrCalibrateParams(SOCKET sock, bool stressed)
	{
		socket = sock;
		isStressed = stressed;
	}
};

class ZephyrUtils
{
public:
	unsigned char CRC;

	ZephyrUtils()
	{
		CRC = 0;
	}

	/** Zephyr packet constants */
	static int const checksumPolynomial = 0x8C;
	/** HXM message id */
	static char const HXM_ID = 0x26;
	/** End of text */
	static char const ETX = 0x03;
	/** Start of text */
	static char const STX = 0x02;
	/** HXM packet size */
	static char const HXM_DLC = 0x37;

	// Merge two bytes into a unsigned integer
	int mergeUnsigned(char low, char high)
	{
		int lint = low & 0xff;
		int hint = high & 0xff;
		return (int)( hint << 8 | lint );
	}

	// print packet contents in a radable manner  
	void print(char packet[], int bufLen)
	{  
		if (packet == NULL) return;
		for(int c = 0; c < bufLen; ++c)
		{
			printf("[%i] \t hex : %02x\n", c, packet[c]);
		}
	}

	// calculate CRC
	void crc8PushByte(unsigned char *crc, unsigned char ch)
	{
		unsigned char i;
		*crc = *crc ^ ch;
		for (i=0; i<8; i++)
		{
			if (*crc & 1)
			{
				*crc = (*crc >> 1) ^0x8C;
			}
			else
			{
				*crc = (*crc >> 1);
			}
		}
	}

	unsigned char crc8PushBlock(unsigned char *pcrc, char *block, int count)
	{
		unsigned char crc = pcrc ? *pcrc : 0;

		for (; count>0; --count, block++)
		{
			crc8PushByte(&crc, *block);
		}

		if (pcrc)
			*pcrc = crc;
		printf("CRC: '%c'\n", crc);
		return crc;
	}

	/** Send the packet to enable data packets, 1 per second */
	bool setupSummary(SOCKET spp)
	{
		/*// turn on general packet
		char data[6];
		data[0] = STX;
		data[1] = 0x14; // msg ID
		data[2] = 0x01; // dlc
		data[3] = 0x01; // state
		data[4] = 0x5e; // crc
		data[5] = ETX;

		send(spp, data, 6, 0);
		char recvBuf[5];
		recv(spp, recvBuf, 5, 0);
		print(recvBuf, 5);*/

		// turn on summary packet 
		char data[7];
		data[0] = STX;
		data[1] = 0xbd; // msg ID
		data[2] = 0x02; // dlc
		//data[3] = 0x3c; // update period LS
		data[3] = 0x01; // update period LS
		data[4] = 0x00; // update period MS
		CRC = crc8PushBlock(NULL, &data[3], 2);
		data[5] = CRC;//0x5e; // crc
		data[6] = ETX;
		send(spp, data, 7, 0);
		char recvBuf[5];
		recv(spp, recvBuf, 5, 0);
		print(recvBuf, 5);
		if(+recvBuf[1] != -67)
		{
			return false;
		}

		return true;
				
	}

	bool setupBRwave(SOCKET spp)
	{				
		char data[6];
		data[0] = STX;
		data[1] = 0x15; // msg ID
		data[2] = 0x01; // dlc
		data[3] = 0x01; //
		data[4] = 0x5e; // crc
		data[5] = ETX;
		send(spp, data, 6, 0);
		char recvBuf[5];
		recv(spp, recvBuf, 5, 0);
		print(recvBuf, 5);

		return true;
	}

	/** Send reset command to bioharness */
	void resetBioharness(SOCKET spp)
	{
		// setup hr packets at one per second
		char data[6];
		data[0] = STX;
		data[1] = 0x1F; // msg ID
		data[2] = 0x07; // dlc
		data[4] = 0x5e; // crc
		data[5] = ETX;

		//replace with send command
		send(spp, data, 6, 0);
		char recvBuf[5];
		recv(spp, recvBuf, 5, 0);
		//print(recvBuf, 5);
	}
};

ZephyrUtils ZephUtils = ZephyrUtils();

BOOL CALLBACK auth_callback_ex(LPVOID pvParam, PBLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS authParams)
{
    BLUETOOTH_AUTHENTICATE_RESPONSE response;
    response.bthAddressRemote = authParams->deviceInfo.Address;
    response.authMethod = authParams->authenticationMethod; // == BLUETOOTH_AUTHENTICATION_METHOD_LEGACY

    UCHAR pin[] = "1234";
    copy(pin, pin+sizeof(pin), response.pinInfo.pin);
    response.pinInfo.pinLength = sizeof(pin)-1; //excluding '\0'

    response.negativeResponse = false;

	cout<<"Sending authentication response"<<endl;
    HRESULT err = BluetoothSendAuthenticationResponseEx(NULL, &response);
    if (err)
    {
        cout << "BluetoothSendAuthenticationResponseEx error = " << err << endl;
    }

    return true;
}

list<BLUETOOTH_DEVICE_INFO> Look_for_Hardware(BLUETOOTH_DEVICE_INFO &bt, HBLUETOOTH_DEVICE_FIND &hDevice)
{
	BLUETOOTH_DEVICE_SEARCH_PARAMS btSearchParams;
	list<BLUETOOTH_DEVICE_INFO> devices;

    btSearchParams.dwSize = sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS);
    btSearchParams.cTimeoutMultiplier = 5;  //5*1.28s search timeout
    btSearchParams.fIssueInquiry = true;    //new inquiry

    //return all known and unknown devices
    btSearchParams.fReturnAuthenticated = true;
    btSearchParams.fReturnConnected = true;
    btSearchParams.fReturnRemembered = true;
    btSearchParams.fReturnUnknown = true;

    btSearchParams.hRadio = NULL;   //search on all local radios

    HBLUETOOTH_DEVICE_FIND btDeviceFindHandle = NULL;

    hDevice = BluetoothFindFirstDevice(&btSearchParams, &bt);
	if(hDevice)
    {
		//wprintf(L"\t\tAuthenticated: %s\r\n", m_device_info.fAuthenticated ? L"true" : L"false");
		do
		{
			string device_name = ConvertWCHARToString(bt.szName);
			bool name_check = (regex_match(device_name, regex("(RN42)(.*)")) || regex_match(device_name, regex("(BH )(.*)"))) ? true :false;
			if(name_check)
			{
				if(bt.fAuthenticated == 0)
				{
					HBLUETOOTH_AUTHENTICATION_REGISTRATION authCallbackHandle = NULL;

					DWORD err = BluetoothRegisterForAuthenticationEx(&bt, &authCallbackHandle, &auth_callback_ex, NULL);
					if (err != ERROR_SUCCESS)
					{
						DWORD err = GetLastError();
						cout << "BluetoothRegisterForAuthentication Error" << err << endl; 
					}
					else
					{
						wprintf(L"Device: %s\n", bt.szName);
						cout<<"Registration callback for authentication succeeded"<<endl;
					}

					//Authenticating device
					DWORD auth_err = BluetoothAuthenticateDeviceEx(NULL, NULL, &bt, NULL, MITMProtectionRequired);
					if(auth_err != ERROR_SUCCESS)
					{
						cout<<"Device Authentication error: "<<GetLastError()<<endl;
					}
					else
					{
						cout<<"Device Authentication succeeded"<<endl;
						BluetoothUnregisterAuthentication(authCallbackHandle);
					}
				}
				devices.push_back(bt);
			}
		}while(BluetoothFindNextDevice(hDevice, &bt));
	}
	return devices;
}

//connect a socket to the zephyr
SOCKET connectToZephyr()
{
	list<BLUETOOTH_DEVICE_INFO> devices;
	BLUETOOTH_DEVICE_INFO btDeviceInfo;
    ZeroMemory(&btDeviceInfo, sizeof(BLUETOOTH_DEVICE_INFO));   //"initialize"
    btDeviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO);
    HBLUETOOTH_DEVICE_FIND btDeviceFindHandle = NULL;

	devices = Look_for_Hardware(btDeviceInfo, btDeviceFindHandle);

	if(devices.size() > 0)
    {
		WSADATA wsaData;
        int err = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (err)
        {
            //std::cout << "WSAStartup error = " << err << std::endl;
        }
		for(list<BLUETOOTH_DEVICE_INFO>::iterator it = devices.begin(); it != devices.end(); ++it)
		{
			device_name = ConvertWCHARToString(it->szName);
			cout<<"Device Name: "<< device_name <<endl;
			if(regex_match(device_name, regex("(BH )(.*)")))
			{
				SOCKET s = socket (AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
				SOCKADDR_BTH btSockAddr;
				btSockAddr.addressFamily = AF_BTH;
				btSockAddr.btAddr = it->Address.ullLong;
				btSockAddr.serviceClassId = RFCOMM_PROTOCOL_UUID; //SerialPortServiceClass_UUID (no difference)
				btSockAddr.port = BT_PORT_ANY;

				err = connect(s, reinterpret_cast<SOCKADDR*>(&btSockAddr), sizeof(SOCKADDR_BTH));
				if(err)
				{
					return NULL;
				}
				else
				{
					//setup zephyr for streaming
					//ZephUtils.setupBRwave(s);
					ZephUtils.setupSummary(s);
					//if(!ZephUtils.setupBioharness(s))
					//{
						//cout << "ERROR! Try Again!" << endl;
						//closesocket(s);
						//return NULL;
					//}
					//else
					Sleep(5);
					return s;
				}
			}
		}
	}
	BluetoothFindDeviceClose(btDeviceFindHandle);
	
	//if it fails completely
	return NULL;
}

// keep zephyr alive
DWORD WINAPI zephyrTimeoutThreadFunc(LPVOID lpParam)
{
	SOCKET sock = (SOCKET)lpParam;
	Sleep(2000);
	while(isAlive)
	{
		Sleep(500);
		if(!alive)
		{
			ZephUtils.resetBioharness(sock);

			//ZephUtils.setupBRwave(sock);
			Sleep(1000);
		}
		alive = false;
	}
	return 0;
}