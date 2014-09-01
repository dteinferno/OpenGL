#include "glmain.h"
#include <windows.h>
#include "balloffset.h""

LARGE_INTEGER li;
float PCFreq = 0;
__int64 CounterStart;


// FOR CLOSED LOOP STRIPE - store the offsets
boost::mutex io_mutex;
float BallOffset = 0.0f;
float dx0 = 0.0f;
float dx1 = 0.0f;
float dy0 = 0.0f;
float dy1 = 0.0f;

//Declare global FTDI variables for communicating with the treadmill
FT_HANDLE ftHandle;
unsigned char wBuffer[1024];
DWORD txBytes = 0;

// Initialize the offset variables and the treadmill
void InitOffset()
{
	// Get the PC frequency and starting time
	QueryPerformanceFrequency(&li);
	PCFreq = li.QuadPart;
	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;

	//Initialize the treadmill
	TreadMillStart();

	//Generate a random starting offset
	srand(time(0));
	io_mutex.lock();
	BallOffset = fmod(rand(), 240) - 120.0f;
	io_mutex.unlock();

	boost::thread thrd(&TreadMillDat);

}

// FOR OPEN LOOP STRIPE
// Set the loop duration for the stripe and calculate the offset due to time passing
void TimeOffset(float &tOffset, __int64 start) {
	// get delta time for this iteration:
	QueryPerformanceCounter(&li);
	float fDeltaTime = (li.QuadPart - start) / PCFreq;
	float period = 20;
	float gain = 360 / period;
	tOffset = fDeltaTime*gain;
}


// A routine to initialize the treadmill 
void TreadMillStart()
{
	//FTDI Variables
	FT_STATUS ftStatus = FT_OK;
	DWORD numDevs, devID;


	//Detect the number of FTDI devices connected
	// You may have multiple devices using FTDI interfaces
	//  This is commonly the case with RS232-USB adaptors
	//  In this case, you'll have to determine who's who by trial and error
	FT_CreateDeviceInfoList(&numDevs);
	//Case of no devices plugged in
	if (numDevs == 0){
		printf("No FTDI Devices Found");
		Sleep(2000);
	}
	//if only one FTDI device found, assume it's a treadmill
	if (numDevs == 1){
		printf("1 Device Detected, Connecting to Dev:0");
		devID = 0;
	}
	else{ //Allow user to specify which device ID to use
		printf("%d Devices Detected, Enter device ID (0-%d): ", numDevs, numDevs - 1);
		scanf_s("%d", &devID);
		printf("\n\nConnecting to Dev:%d", devID);
	}
	printf("\n\n");
	Sleep(1000);

	//Configure and Connect to Treadmill serial interface
	ftStatus |= FT_Open(devID, &ftHandle);
	ftStatus |= FT_ResetDevice(ftHandle);
	ftStatus |= FT_SetTimeouts(ftHandle, 2000, 2000);
	ftStatus |= FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	ftStatus |= FT_SetFlowControl(ftHandle, FT_FLOW_NONE, NULL, NULL);
	ftStatus |= FT_SetBaudRate(ftHandle, 1250000);  //1.25MBaud Communication rate
	if (ftStatus != FT_OK) { printf("Error connecting to FTDI interface\n"); Sleep(1000); }

	//Stop any existing data stream from the treadmill
	wBuffer[0] = 254;
	wBuffer[1] = 0;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);
	Sleep(20);

	FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);

	//Set to independent mode
	wBuffer[0] = 246;
	wBuffer[1] = 1;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);

	//Start Motion Data @ High Speed (4kHz)
	wBuffer[0] = 255;
	wBuffer[1] = 0;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);
}

//Run the treadmill at 100 Hz and continuosly update the offset. 
void TreadMillDat()
{
	//Set the calibration factor
	float calibfact = 15;

	//Camera Data Bins
	int dx[2], dy[2];

	// FTDI variables
	unsigned char rBuffer[480];
	DWORD rxBytes;

	while (1)
	{
		//Read 40 packets of data (12 bytes per packet)
		FT_Read(ftHandle, rBuffer, 40 * 12, &rxBytes);

		if (rxBytes != 480){
			printf("Bad Read\n");
			Sleep(1);
		}

		//Accumulate Motion Data for this 100Hz chunk
		dx[0] = 0; dx[1] = 0; dy[0] = 0; dy[1] = 0;
		for (int i = 0; i < 480; i += 12){
			dx[0] += ((int)rBuffer[i + 2]) - 128;
			dy[0] += ((int)rBuffer[i + 3]) - 128;
			dx[1] += ((int)rBuffer[i + 4]) - 128;
			dy[1] += ((int)rBuffer[i + 5]) - 128;
		}

		//Update the offset given the ball movement
		io_mutex.lock();
		BallOffset += (float)(dx[0] + dx[1]) / calibfact;
		dx0 += dx[0];
		dx1 += dx[1];
		dy0 += dy[0];
		dy1 += dy[1];

		io_mutex.unlock();
	}

}

void CloseOffset()
{
	//thrd.detach();

	//Stop Acquisition
	wBuffer[0] = 254;
	wBuffer[1] = 0;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);

	//Close serial interface
	FT_Close(ftHandle);
}