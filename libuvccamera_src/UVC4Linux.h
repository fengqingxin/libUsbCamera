#ifndef __CAP_UVC_4_LINUX_H__
#define __CAP_UVC_4_LINUX_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>  // FIXIT remove this
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include "libuvc.h"
#include "uvcCapture.h"
#include "PropertyRange.h"
#include <math.h>
#include "pthread.h"



#define  MAX_IMAGE_FORMAT_ITEM_NUM              200
#define  MAX_CAMERA_NUM                         8

#define  UVC_MAX_DECODE_PREFIX_LEN                 (20U)
#define  UVC_MAX_DECODE_POSTFIX_LEN                (20U)
#define  UVC_MAX_MATCH_STRING_LEN                  (20U)
#define  UVC_MAX_LEN_LONG_DECODE_RESULT            (200U)
#define  UVC_MAX_LEN_ZOOM_SCALE                    (10U)
#define  UVC_MAX_USER_DAT_LEN                      (32U)
#define  UVC_MAX_UPDATE_PKG_LEN                    (32U)
#define  MAX_CMD_SEND_SIZE                         1024U


/** 用于Cyclops兼容相机   */
#define  USE_CYCLPOS_SG8021
/**  用于Hi3516相机       */
//#define  USE_HI3516DV300
/**  CYCLOPS Flash特性   */
//#define  USE_CYCLOPS_FLASH


#pragma pack(1)

// 用户命令数据结构
typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t dat[20];  // 最多20bytes数据
}UvcUserCmdStruct_t;



typedef struct sFormatItem {
  float mixValue;
  float maxValue;
  float defValue;
  float curValue;
  uint8_t NeedUpdate;
} sFormatItem_t;

#define  MAX_TYPE_DIMENSION_SUPPORT       5

typedef struct sImageDimension {
  uint16_t width;
  uint16_t height;
} sImageDimension_t;

// 图像格式
typedef struct sImageParam {
  uint8_t  format;
  sImageDimension_t dimension[MAX_TYPE_DIMENSION_SUPPORT];
} sImageParam_t;

// 开窗与缩放参数结构体
typedef struct {
    uint8_t bScale;
    uint16_t wOffsetX;
    uint16_t wOffsetY;
    uint16_t wWidth;
    uint16_t wHeight;
}uvc_image_scale_zoom_mode_t;

#pragma pack()

struct UVC4Linux : public UVCCapture {
 public:
  UVC4Linux();
  virtual ~UVC4Linux();

  bool open(int hubPortNum);
//  bool open(const char* _deviceSN);

  virtual double getProperty(int);
  virtual bool setProperty(int, double);
  virtual int grabFrame(unsigned char** data);

  PropertyRange getRange(int property_id) const;
  //void* pthread_monitor(void* param);
  uint8_t updateStatus(void);
  void setStatus(uint8_t  stat);
  bool openVideoStream();
  bool closeVideoStream();
  int getID(void);
  bool Reopen(void);
  int8_t FindIndexFromDevList(uint8_t hubPortNum, std::vector<uvc_device_t*> list);
  bool FreshProperty(int propId, double value);


 public:
  bool m_bInit;
//  int deviceHandle;
//  int bufferIndex;
  uint8_t hubPortNum;
  std::string m_strDeviceSN;
  bool m_bReadFrame;
  pthread_mutex_t m_mutexReadFrame;
//  std::condition_variable_any m_t;
//  char* memoryMap;
//  // IplImage frame;
  int m_iPalette;
  int m_iFormat;
  int m_iStillWidth, m_iStillHeight;
  int m_iWidth, m_iHeight;
  int m_iWidth_set, m_iHeight_set;
  int m_iFps;
//  bool convert_rgb;
//  bool frame_allocated;

//  std::vector<unsigned char> vSerialNumber;      //add by Murphy 180412
//  void* camLed;                                 //add by Murphy 180412
  bool m_bIsVideoScreamStart;                      //add by Murphy 190327
//
  /* UVC variables */
  std::vector<uvc_device_t*> m_lsCap;
  uvc_context_t* m_ctx;
  uvc_device_t* m_dev;
  uvc_device_handle_t* m_devh;
  uvc_stream_ctrl_t m_ctrl;
  uvc_still_ctrl_t m_still_ctrl;
  uvc_frame_t* m_buffers;

//  /* V4L2 control variables */
  PropertyRange focus, brightness, contrast, saturation, hue, gain, exposure;
  // add by yuliang , stream id
  int8_t id;
  void* m_pCamLed;
  void* m_pCamFlash;
  // realtime Connect status add by yuliang
  uvc_StatusCallBack  gStatusCallBack;
  sFormatItem_t  gImageFormatConfig[MAX_IMAGE_FORMAT_ITEM_NUM];
  uint8_t gCamStatus;    //add by yuliang 190423
  pthread_mutex_t  m_statusMutex;//[MAX_CAMERA_NUM];  //modify by yuliang
  pthread_t pthread_monitor_tid;  // modify by yuliang
  int shmid;  // add by yuliang  20190516
  cameraList_t*  pCamList;
  uint8_t gucIsStillTrigger;
  char m_SN[16];
  char m_DecodeResult[200];
  uint8_t m_Roi[10];
  char m_DecodePrefix[UVC_MAX_DECODE_PREFIX_LEN];
  char m_DecodePostfix[UVC_MAX_DECODE_POSTFIX_LEN];
  char m_DecodeMatchString[UVC_MAX_MATCH_STRING_LEN];
  char m_ProductInfo[UVC_MAX_LEN_LONG_DECODE_RESULT];
  uint8_t m_ScaleZoomMode[UVC_MAX_LEN_ZOOM_SCALE];
  uint16_t m_LedBrightness;
  uint8_t m_UserCmdReadBuf[UVC_MAX_USER_DAT_LEN];
};

#endif /* __VIDEOIO_H_ */
