#ifndef __CAP_UVC_CAPTURE_H__
#define __CAP_UVC_CAPTURE_H__

// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <limits.h>
// #include <ctype.h>
// #include <assert.h>  // FIXIT remove this
// #include <string>
// #include <vector>
// #include <mutex>
// #include <condition_variable>
#include <sys/time.h>

/* SG UVC Camera  */
#define VID 0x2084
#define PID 0x0505

/*  Microdia Camera  */
//#define VID 0x6366
//#define PID 0x3370

/* SG HID Camera  */
#define  HID_VID        0x1fc9
#define  HID_PID        0x00a0


#if defined _WIN32 || defined WINCE
#if !defined _WIN32_WINNT
#ifdef HAVE_MSMF
#define _WIN32_WINNT 0x0600 // Windows Vista
#else
#define _WIN32_WINNT 0x0501 // Windows XP
#endif
#endif

#include <windows.h>
#undef small
#undef min
#undef max
#undef abs
#endif

enum {
  UVC_STATUS_READY  = 0,
  UVC_STATUS_OPEN   = 1,
  UVC_STATUS_CLOSE  = 2,
  UVC_STATUS_RECON  = 3,
  UVC_STATUS_ERR    = 4,
  UVC_STATUS_EXIT   = 15,
};

// ”√ªß√¸¡Ó
typedef enum{
    UVC_USER_CMD_SET_DEV_NAME   = 0,
    UVC_USER_CMD_SET_SERIAL_NUM = 1,
    UVC_USER_CMD_RESET          = 2,

}UVC_USER_CMD_e;

#pragma pack(1)
typedef struct cameraList {
  char cSn[16 + 1];
  int id;
  uint8_t isOpened;
  uint8_t hubPortNum;
} cameraList_t;

#pragma pack()

extern struct timeval m_Start;
extern struct timeval m_Mid;
extern double gTick;

//#define DEBUG_LOG
#ifdef DEBUG_LOG

#define START_LOG     do{   \
                  gettimeofday(&m_Start, NULL);  \
              }while(0);

#define PRINTF_LOG(x)       do{   \
                gettimeofday(&m_Mid, NULL);  \
                  gTick = (m_Mid.tv_sec - m_Start.tv_sec) * 1000.0 + (double)(m_Mid.tv_usec - m_Start.tv_usec) / 1000.0; \
                printf("\n[%ds:%dms][%s][%s]\n",(int)gTick/1000, ((int)gTick%1000), __func__, x); \
                }while(0);
#else
#define START_LOG
#define PRINTF_LOG(x)
#endif

struct UVCCapture {
  virtual ~UVCCapture() {}
  virtual double getProperty(int) {
    return 0;
  }
  virtual bool setProperty(int, double) {
    return 0;
  }
  virtual int grabFrame(unsigned char** data) {
    return true;
  }
};

UVCCapture* uvcCreateCameraCapture(int index);
UVCCapture* uvcCreateCameraCapture(const char* deviceSN);

#endif /* __VIDEOIO_H_ */
