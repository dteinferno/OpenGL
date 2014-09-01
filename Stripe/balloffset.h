// Header for interfacing with the treadmill
#include "ftd2xx.h"

// Boost headers for setting up the data server
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// Counter to serve as a reference for timestamping
extern __int64 CounterStart;
extern LARGE_INTEGER li;
extern float PCFreq;

//Declare global variables to lock (mutex) and store the net offset (for closed loop)
extern boost::mutex io_mutex;
extern float BallOffset;
extern float dx0;
extern float dx1;
extern float dy0;
extern float dy1;

void InitOffset(void);

void TimeOffset(float &tOffset, int dir, time_t start);

void TreadMillStart(void);

void TreadMillDat(void);

void CloseOffset(void);