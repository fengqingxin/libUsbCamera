
#include <pthread.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <errno.h>
#include <signal.h>
#include "libusb.h"
#include "UVC4Linux.h"
#include "capSonixCtrls.h"
#include "uvcCamera.h"
#include <math.h>
#include <stdlib.h>
#include "libuvc_internal.h"


using namespace std;
struct timeval m_Start;
struct timeval m_Mid;
double gTick = 0;



static void* pthread_monitor(void* param);
int8_t isThreadExist(int idx);
int getCaptureList(uvc_context_t* m_ctx, std::vector<uvc_device_t*>& list);
//void uvcSyncShmStatus(int shmid, uint8_t port, cameraList_t* pCamList, uint8_t status, uint8_t id);
//int uvcCreateShm(int* shmid);
//void uvcGetShmStatus(int shmid, cameraList_t cameraList[]);


/**
\code
Function   :  format_to_String
Description:  format index to String
Calls      :  N/A
Called By  :  AdjustParam
Input      :  int format ,
              char* sFormat
Output     :  N/A
Return     :  N/A
ChangeList : add by yu 20200201
\encode
*/
static void format_to_String(int format, char* sFormat)
{
    switch(format){
    case UVC_CAP_MODE_MJPEG:
        sprintf(sFormat, "MJPEG");
        break;
    case UVC_CAP_MODE_YUYV:
        sprintf(sFormat, "YUYV");
        break;
    default:
        break;
    }
}

/**
\code
Function   :  AdjustWidthAndHeight
Description:  Adjust width and height for input parameter
Calls    :  N/A
Called By  :  icvSetPropertyCAM_V4L
Input    :  capture handle
              srcwid
              srcHei
Output     :  N/A
Return     :  true  : succ
              false : failed
ChangeList : add by yu 20200201
\encode
*/
void AdjustParam(UVC4Linux* capture, int srcWid, int srcHei)
{
#define  FORMAT_NUM_CUR_CAMERA        2
    sImageParam_t uvcImageFormat[FORMAT_NUM_CUR_CAMERA] = {
        {UVC_CAP_MODE_MJPEG, {
               {640, 480},
               {320, 240},
               {800, 600},
               {1280, 720}
               }
        },
        {UVC_CAP_MODE_YUYV, {
               {1280, 720},
               {640, 480},
               {320, 240},
               {800, 600}}
        }
    };

    for(int idx =0; idx < FORMAT_NUM_CUR_CAMERA; idx++) {
        if(capture->m_iFormat != uvcImageFormat[idx].format) {
            continue;
        }

        char pcFormatString[50];
        format_to_String(uvcImageFormat[idx].format, pcFormatString);

        // set width and height
        for(int ptr =0; ptr< MAX_TYPE_DIMENSION_SUPPORT;  ptr++) {
//        if((0 != uvcImageFormat[idx].dimension[ptr].width) &&
//           (0 != uvcImageFormat[idx].dimension[ptr].height) &&
//           (srcWid == uvcImageFormat[idx].dimension[ptr].width) &&
//           (srcHei == uvcImageFormat[idx].dimension[ptr].height) )  {
            if( (0 != uvcImageFormat[idx].dimension[ptr].width) ||
                (0 != uvcImageFormat[idx].dimension[ptr].height) ) { // 以宽为主
                if( (srcWid == uvcImageFormat[idx].dimension[ptr].width) ||
                    (srcHei == uvcImageFormat[idx].dimension[ptr].height) )  {
                    capture->m_iWidth  = uvcImageFormat[idx].dimension[ptr].width;
                    capture->m_iHeight = uvcImageFormat[idx].dimension[ptr].height;
                    // 打印格式，分辨率
                    printf("Format: %s, SetResolution: w =%d, h=%d\n", pcFormatString, capture->m_iWidth, capture->m_iHeight);
                    break;
                }
            }
        }
    }
}


/**
\code
Function   :  icvGetPropertyCAM_V4L
Description:  get Property through libuvc protocol
Calls	   :  uvc_get_*
Called By  :  getProperty
Input	   :  capture handle
              property id
              value(double)
Output	   :  N/A
Return	   :  double type
ChangeList :
\encode
*/
static double icvGetPropertyCAM_V4L(UVC4Linux* capture,
                                    int property_id) {
  uvc_error_t ret;
  /*  add by yuliang, avoid crash when execute uvc access operation after uvc_break */
  //if( (capture->gCamStatus !=UVC_STATUS_OPEN) && (property_id != UVC_CAP_PROP_OPEN_STAT))
  //return -1.0;

  switch(property_id)
  {
     case UVC_CAP_PROP_FRAME_WIDTH:
        if(capture->gucIsStillTrigger ==1)
	       return capture->m_iStillWidth;
	    else
           return capture->m_iWidth;

     case UVC_CAP_PROP_FRAME_HEIGHT:
  	    if(capture->gucIsStillTrigger ==1)
	       return capture->m_iStillHeight;
	    else
    	    return capture->m_iHeight;

     case UVC_CAP_PROP_FPS:
        return capture->m_iFps;

     case UVC_CAP_PROP_MODE:   //Color mode
        return capture->m_iPalette;

     case UVC_CAP_PROP_FORMAT:   //Frame Format
        return capture->m_iFormat;

     case UVC_CAP_PROP_OPEN_STAT: {
        //printf("camStatus: %d\n", capture->gCamStatus);
        return (double)(long)capture->gCamStatus;
     }

     case UVC_CAP_PROP_GUID:                   //add by Murphy 180412
        return (double)(long long)capture->m_strDeviceSN.c_str();
  }

  //The following need to determine whether m_dev exists.
  if((UVC_STATUS_OPEN != capture->gCamStatus)
      && (UVC_STATUS_CLOSE != capture->gCamStatus)) {
    return -1.0;
  }

  switch(property_id) {
  case UVC_CAP_PROP_BRIGHTNESS: {
    PropertyRange range = capture->getRange(property_id);
    int16_t uvcValue;
    uvc_get_brightness(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_CONTRAST: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue;
    uvc_get_contrast(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_SATURATION: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue;
    uvc_get_saturation(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_HUE: {
    PropertyRange range = capture->getRange(property_id);
    int16_t uvcValue;
    uvc_get_hue(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_GAIN: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue;
    uvc_get_gain(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_AUTO_EXPOSURE: {
    PropertyRange range = capture->getRange(property_id);
    uint8_t uvcValue;
    uvc_get_ae_mode(capture->m_devh, &uvcValue, UVC_GET_CUR);
    // 超出, 限制范围
    uvcValue = (uvcValue > range.end) ? range.end: uvcValue;
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_EXPOSURE: {
    PropertyRange range = capture->getRange(property_id);
    uint32_t uvcValue;
    uvc_get_exposure_abs(capture->m_devh, &uvcValue, UVC_GET_CUR);
    // 超出, 限制范围
    uvcValue = (uvcValue > range.end) ? range.end: uvcValue;
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_AUTOFOCUS: {
    PropertyRange range = capture->getRange(property_id);
    uint8_t uvcValue;
    uvc_get_focus_auto(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  case UVC_CAP_PROP_FOCUS: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue;
    uvc_get_focus_abs(capture->m_devh, &uvcValue, UVC_GET_CUR);
    return ((double)uvcValue - range.start) / range.size();
  }

  /* Used for STILL Image event  */
  case UVC_CAP_PROP_TRIGGER: {
  	return capture->gucIsStillTrigger;
  }

  case UVC_CAP_PROP_CONVERT_RGB:
    break;

//    return capture->convert_rgb;
  case UVC_CAP_PROP_SETTINGS:
    break;

//    return capture->deviceHandle;
  case UVC_CAP_PROP_BUFFERSIZE:
//    return capture->bufferSize;
    break;

  case UVC_CAP_PROP_DECODE_RESULT:
	 char pProductInfo[UVC_MAX_LEN_LONG_DECODE_RESULT];  // Attention: ProductInfo maxim length is 300 bytes
     ret = uvc_get_xu_data(capture->m_devh, UVC_XU_CONTROL_DECODE_RESULT_LONG_ID, (uint8_t*)pProductInfo, UVC_MAX_LEN_LONG_DECODE_RESULT);
	 if(ret <0)  {
        printf("Get LongResult err:%d\n", ret);
	    return -1;
	 } else  {
        memcpy(capture->m_ProductInfo, pProductInfo, UVC_MAX_LEN_LONG_DECODE_RESULT);
        return (double)(long long)capture->m_ProductInfo;
	 }
	 break;

  // 获取相机的描述符信息
  case UVC_CAP_PROP_CAMERA_INFO:
     uvc_print_diag(capture->m_devh, stderr);
	 break;

  // 用户命令参数获取
  case UVC_CAP_PROP_USER_CMD:{
    uint8_t pbReadUserCmdBuf[UVC_MAX_USER_DAT_LEN]= {0};
    // Read User Command Test
    uvc_error_t ret = uvc_get_xu_data(capture->m_devh, UVC_XU_CONTROL_USER_CMD_PROP_ID, pbReadUserCmdBuf, UVC_MAX_USER_DAT_LEN);
    if(ret <0)  {
        printf("Get UserCmd err: %d\n", ret);
	    return -1;
    } else {
        memset(capture->m_UserCmdReadBuf, 0, UVC_MAX_USER_DAT_LEN);
        memcpy(capture->m_UserCmdReadBuf, pbReadUserCmdBuf, UVC_MAX_USER_DAT_LEN);
        return (double)(long long)capture->m_UserCmdReadBuf;
    }
    break;
  }

  default:
     printf("GetProperty Param err: %d\n", property_id);
	 break;
  }

  return -1;
}


/**
\code
Function   :  icvSetPropertyCAM_V4L
Description:  set Property through libuvc protocol
Calls	   :  openvideostream
              SetValue
              closeVideoStream
              uvc_set_*
Called By  :  setProperty
Input	   :  capture handle
              property id
              value(double)
Output	   :  N/A
Return	   :  true  : succ
              false : failed
ChangeList :
\encode
*/
static int icvSetPropertyCAM_V4L(UVC4Linux* capture,
                                 int property_id, double value) {
  bool retval = false;

  /*  add by yuliang, avoid crash when execute uvc access operation after uvc_break */
  //if( (capture->gCamStatus !=UVC_STATUS_OPEN) && (property_id!= UVC_CAP_PROP_OPEN_STAT))
  //  return -1.0;

  /* two subsequent calls setting WIDTH and HEIGHT will change
     the video size */

  switch(property_id) {
  case UVC_CAP_PROP_OPEN_STAT: {
    if(value > 0) {
      if(false == capture->m_bIsVideoScreamStart) {
        retval = capture->openVideoStream();
      } else {
        retval = true;
      }
    } else {
      if(false == capture->m_bIsVideoScreamStart) {
        retval = true;
      } else {
        retval = capture->closeVideoStream();
      }
    }

    return retval;
  }
  break;

  }//end of switch

  //The following need to determine whether m_dev exists.
  if((UVC_STATUS_OPEN != capture->gCamStatus)
      && (UVC_STATUS_CLOSE != capture->gCamStatus)) {
    return 0;
  }

  switch(property_id) {
  case UVC_CAP_PROP_FRAME_WIDTH: {
    int& width = capture->m_iWidth_set;
    int& height = capture->m_iHeight_set;
    width = round(value);
    retval = width != 0;

    if(width != 0 || height != 0) {
      AdjustParam(capture, width, height);   // add by yuliang  20190515
      retval = capture->closeVideoStream();
//      capture->m_iWidth = width;
//      capture->m_iHeight = height;
      retval = capture->openVideoStream();
      width = height = 0;
    }
  }
  break;

  case UVC_CAP_PROP_FRAME_HEIGHT: {
    int& width = capture->m_iWidth_set;
    int& height = capture->m_iHeight_set;
    height = round(value);
    retval = height != 0;

    if(width != 0 || height != 0) {
      AdjustParam(capture, width, height);   // add by yuliang  20190515
      retval = capture->closeVideoStream();
//      capture->m_iWidth = width;
//      capture->m_iHeight = height;
      retval = capture->openVideoStream();
      width = height = 0;
    }
  }
  break;

  case UVC_CAP_PROP_FPS:
    capture->m_iFps = value;
    retval = capture->closeVideoStream();  // Add by yl , restart stream
    retval = capture->openVideoStream();
    break;

  case UVC_CAP_PROP_MODE: {
    capture->m_iPalette = value;
  }
  break;

  /* Used to Send STILL Image Event to Device  */
  case UVC_CAP_PROP_TRIGGER: {
	  uvc_error_t res;

	  capture->gucIsStillTrigger = round(value);
	  if(capture->gucIsStillTrigger ==1) {
	  	  res = uvc_trigger_still(
    	  		capture->m_devh,
    	  		&capture->m_still_ctrl);
		  retval = (res == UVC_SUCCESS);
	  }
  }
  break;

  case UVC_CAP_PROP_BRIGHTNESS: {
    PropertyRange range = capture->getRange(property_id);
    int16_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_brightness(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_CONTRAST: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_contrast(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_SATURATION: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_saturation(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_HUE: {
    PropertyRange range = capture->getRange(property_id);
    int16_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_hue(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_GAIN: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_gain(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_AUTO_EXPOSURE: {
    PropertyRange range = capture->getRange(property_id);
    uint8_t uvcValue = (value > 0.5) ? 2 : 1;
    retval = (UVC_SUCCESS == uvc_set_ae_mode(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_EXPOSURE: {
    PropertyRange range = capture->getRange(property_id);
    uint32_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_exposure_abs(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_AUTOFOCUS: {
    PropertyRange range = capture->getRange(property_id);
    uint8_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_focus_auto(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_FOCUS: {
    PropertyRange range = capture->getRange(property_id);
    uint16_t uvcValue = value * range.size() + range.start;
    retval = (UVC_SUCCESS == uvc_set_focus_abs(capture->m_devh, uvcValue));
  }
  break;

  case UVC_CAP_PROP_FORMAT: {
    capture->m_iFormat = value;
    retval = capture->closeVideoStream();
    retval = capture->openVideoStream();
  }
  break;

  // 设置: 产品名称，生产序列号，复位等功能
  case UVC_CAP_PROP_USER_CMD: {
     // 用户数据指针
     uint8_t* pUserCmdBuf = (uint8_t*)(long)value;
     // 发送用户命令
     uvc_error_t ret = uvc_set_xu_data(capture->m_devh, UVC_XU_CONTROL_USER_CMD_PROP_ID, (uint8_t*)pUserCmdBuf, UVC_MAX_USER_DAT_LEN);
	 if(ret <0)  {
        printf("Set UserCmd err: %d\n", ret);
		retval = false;
	 } else {
        //printf("Set UserCmd succ\n");
		retval = true;
	 }
     // Add by yuliang 20190428, If set operation is Succ, then Fresh ImageParameterTable ???
     //capture->FreshProperty(property_id, value);
  }
  break;

  // 更新包发送
  case UVC_CAP_PROP_UPDATE_PLG: {
    // 用户数据指针
    char* pUserCmdBuf = (char*)(long)value;
    // 用户命令缓冲区
    uint8_t pUserCmdDat[UVC_MAX_UPDATE_PKG_LEN] = {UVC_USER_CMD_SET_DEV_NAME, UVC_MAX_UPDATE_PKG_LEN-2, 0};

    // 总大小 4096Bytes
    int wUserCmdRemainSize = (uint8_t)pUserCmdBuf[1]*256 + (uint8_t)pUserCmdBuf[2];
    int wUserDatIndex =0;
    while(wUserCmdRemainSize >0) {
        uint8_t bPerSendSize;
        if(wUserCmdRemainSize >= (UVC_MAX_UPDATE_PKG_LEN-3))
            bPerSendSize = UVC_MAX_UPDATE_PKG_LEN-3;
        else
            bPerSendSize = wUserCmdRemainSize;

        // 填充数据
        pUserCmdDat[0] = (uint8_t)pUserCmdBuf[0];    // cmd value
        pUserCmdDat[1] = bPerSendSize;   // currrent size
        memcpy(pUserCmdDat+3, pUserCmdBuf+3+wUserDatIndex, bPerSendSize);

        // 偏移剩余数据字节数
        wUserCmdRemainSize -= bPerSendSize;
        // 偏移发送数据指针
        wUserDatIndex += bPerSendSize;

        if(wUserCmdRemainSize<=0)
            pUserCmdDat[2] = 1;  // Flag for End Of Frame
        else
            pUserCmdDat[2] = 0;

        // 发送单个包
        uvc_error_t ret = uvc_set_xu_data(capture->m_devh, UVC_XU_CONTROL_UPDATE_PKG_PROP_ID, pUserCmdDat, UVC_MAX_UPDATE_PKG_LEN);
        if(ret <0)  {
            printf("Set UpdatePkg err:%d\n", ret);
            retval = false;
            return -1;
        } else  {
            retval = true;
        }
    }
    break;
  }

  default:
    retval = false;
    break;
  }

  // Add by yuliang 20190428, If set operation is Succ, then Fresh ImageParameterTable
  if(retval == true) {
    ;//capture->FreshProperty(property_id, value);
  }

  /* return the the status */
  return retval;
}


/**
\code
Function   :  getProperty
Description:  get Property for camera
Calls	   :  icvGetPropertyCAM_V4L
Called By  :  SgCamera::get
Input	   :  propId
Output	   :  N/A
Return	   :  value  : double
ChangeList :
\encode
*/
double UVC4Linux::getProperty(int propId) {PRINTF_LOG("Start");
  //return icvGetPropertyCAM_V4L(this, propId);
  double ret = icvGetPropertyCAM_V4L(this, propId);PRINTF_LOG("End");
  return ret;
  return 0;
}


/**
\code
Function   :  setProperty
Description:  Set Property for camera
Calls	   :  icvSetPropertyCAM_V4L
Called By  :  SgCamera::set
Input	   :  propId
              value
Output	   :  N/A
Return	   :  false :  failed
              true  :  succ
ChangeList :
\encode
*/
bool UVC4Linux::setProperty(int propId, double value) {PRINTF_LOG("Start");
  //return icvSetPropertyCAM_V4L(this, propId, value);
  bool ret =icvSetPropertyCAM_V4L(this, propId, value);PRINTF_LOG("End");

  return ret;
  return 0;
}


/**
\code
Function   :  FreshProperty
Description:  Refresh the configuration table and write it to the camera
              In-memory configuration refresh write operation when reopening the device
Calls	   :  icvSetPropertyCAM_V4L
Called By  :  icvSetPropertyCAM_V4L
              reopen
Input	   :  propId
              value
Output	   :  N/A
Return	   :  false :  failed
              true  :  succ
ChangeList :
\encode
*/
bool UVC4Linux::FreshProperty(int propId, double value) {
  int idx = 0;
  bool ret = false;

  // fresh property Item
  uint8_t FreshItemArray[MAX_IMAGE_FORMAT_ITEM_NUM] = {
    UVC_CAP_PROP_BRIGHTNESS,
    UVC_CAP_PROP_CONTRAST,
    UVC_CAP_PROP_SATURATION,
    UVC_CAP_PROP_HUE,
    UVC_CAP_PROP_GAIN,
    UVC_CAP_PROP_EXPOSURE,
    UVC_CAP_PROP_AUTO_EXPOSURE,
    UVC_CAP_PROP_FOCUS,
    UVC_CAP_PROP_AUTOFOCUS,
    UVC_CAP_PROP_BACKLIGHT,
  };

  //printf("stat: %d, id: %d, val: %3f\n", gCamStatus, propId, value);

  switch(gCamStatus) {
  case UVC_STATUS_OPEN:

    // Update Current Item
    if(propId < UVC_CAP_PROP_END) {
      // If not equal, fresh configuration In-memory;
      if(fabs(value - gImageFormatConfig[propId].curValue) > 0.0001) {
        gImageFormatConfig[propId].curValue = value;
        gImageFormatConfig[propId].NeedUpdate = 1;
      }

      ret = true;
    } else if(propId == UVC_CAP_PROP_END) {
      // Update All Item
      for(idx = 0; idx < UVC_CAP_PROP_END; idx++) {   // Cannot bigger than UVC_CAP_PROP_END, it's a risk point
        if((FreshItemArray[idx] > 0) && (gImageFormatConfig[FreshItemArray[idx]].NeedUpdate == 1) ) {
          printf("[ImageParam] id: %d, val: %3f\n", FreshItemArray[idx],
                 gImageFormatConfig[FreshItemArray[idx]].curValue);
          ret |= icvSetPropertyCAM_V4L(this, FreshItemArray[idx],
                                       (double)gImageFormatConfig[FreshItemArray[idx]].curValue);
        }
      }
    }

    break;

  default:
    break;
  }

  return ret;
}


/**
\code
Function   :  uvcCreateCameraCapture
Description:  create Camera Handle and open it, according index
Calls	   :  UVC4Linux
              pthread_monitor
              open
Called By  :  SgCamera::open
Input	   :  int index
Output	   :  N/A
Return	   :  UVCCapture*
ChangeList :
\encode
*/
UVCCapture* uvcCreateCameraCapture(int index) {
  int ret;

  UVC4Linux* capture = new UVC4Linux();
#if 0
  /*
  * Add monitoring thread creation and detect presence
  */
  if(capture->gCamStatus == UVC_STATUS_READY) {
    ret = pthread_create(&capture->pthread_monitor_tid, NULL, pthread_monitor,
                         (void*)capture);
    if(ret == -1) {
      printf("monitor pthread create failed\npid:%d\n",
             (unsigned int)capture->pthread_monitor_tid);
      return NULL;
    }else
      printf("monitor pthread create succ\n");
  }
#endif // 0

  // Initialize the corresponding lock
  pthread_mutex_init(&capture->m_statusMutex, NULL);

  if(capture->open(index)) {PRINTF_LOG("End id");
    capture->id = index;  // Open successfully, save idx
    return capture;
  }

  delete capture;//PRINTF_LOG("End id0");
  return NULL;
  //return capture;
}


/**
\code
Function   :  uvcReleaseCameraCapture
Description:  destroy monitor pthread, and exit
Calls	   :  pthread_join
Called By  :  ~UVC4Linux
Input	   :  ptr for object
Output	   :  N/A
Return	   :  N/A
ChangeList :
\encode
*/
bool uvcReleaseCameraCapture(void* ptr) {
  PRINTF_LOG("Start");
  // Exit monitoring thread
  UVC4Linux* pcap = (UVC4Linux*)ptr;

  pcap->gCamStatus = UVC_STATUS_EXIT;
  // synchron status with sharedMem
//  uvcSyncShmStatus(pcap->shmid, pcap->hubPortNum+1, pcap->pCamList, UVC_STATUS_CLOSE, pcap->getID());

#if 0
  usleep(10 * 1000);
  if(0 != pcap->pthread_monitor_tid) {
    pthread_join(pcap->pthread_monitor_tid, NULL);
  }
#endif // 0
}


/**
\code
Function   :  getFrameCallBack
Description:  grab frame called by libuvc transfer function
Calls	   :  uvc_duplicate_frame
              uvc_allocate_frame
Called By  :  openvideostream
Input	   :  frame
              ptr
Output	   :  N/A
Return	   :  N/A
ChangeList :
\encode
*/
void getFrameCallBack(uvc_frame_t* frame, void* ptr) {
//  uvc_frame_t* bgr;
  uvc_error_t ret;

  UVC4Linux* pCap = (UVC4Linux*)ptr;
  bool bReadFrame = pCap->m_bReadFrame;

  if(!bReadFrame) {
    return;
  }

  /* it is still capture, wait to complete */
  if(pCap->gucIsStillTrigger==1)
     if(!frame->ucImageType)
	 	return;

  if(pCap->m_buffers) {
    uvc_free_frame(pCap->m_buffers);
  }

  printf("data_bytes: %d, width:%d, height:%d, format:%d\n", frame->data_bytes,
         frame->width,
         frame->height, frame->frame_format);

  if(UVC_CAP_MODE_MJPEG == pCap->m_iFormat) {
    uvc_duplicate_frame(frame, pCap->m_buffers);
    pthread_mutex_lock(&pCap->m_mutexReadFrame);
    pCap->m_bReadFrame = false;
    pthread_mutex_unlock(&pCap->m_mutexReadFrame);
    PRINTF_LOG("End");
    return;
  }

  if(UVC_CAP_MODE_GRAY == pCap->m_iPalette) {
    pCap->m_buffers = uvc_allocate_frame(frame->width * frame->height * 1);

    if(!pCap->m_buffers) {
      return;
    }

    ret = uvc_yuyv2y(frame, pCap->m_buffers);

    if(ret) {
      uvc_free_frame(pCap->m_buffers);
      return;
    }
  } else {    /* Do the BGR conversion */
    pCap->m_buffers = uvc_allocate_frame(frame->width * frame->height *
                                         3);  // Only yuyv to rgb
    if(!pCap->m_buffers) {
      return;
    }

    //ret = uvc_any2rgb(frame, pCap->m_buffers);
    ret = uvc_duplicate_frame(frame, pCap->m_buffers);  //Do Not convert YUYV to RGB888
    if(ret) {   // If failed, free buffers
      uvc_free_frame(pCap->m_buffers);
      return;
    }
  }

  // Identification
  pthread_mutex_lock(&pCap->m_mutexReadFrame);
  pCap->m_bReadFrame = false;
  pthread_mutex_unlock(&pCap->m_mutexReadFrame);
  return;
}


/**
\code
Function   :  UVC4Linux
Description:  Construct function
Calls	   :  uvc_init
Called By  :  uvcCreateCameraCapture
Input	   :  N/A
Output	   :  N/A
Return	   :  N/A
ChangeList :
\encode
*/
UVC4Linux::UVC4Linux()
  : m_bInit(false)
  , m_strDeviceSN("")
  , m_bReadFrame(false)
  , m_iPalette(UVC_CAP_MODE_BGR)  //UVC_CAP_PROP_MODE = UVC_CAP_MODE_BGR
#ifdef USE_CYCLPOS_SG8021
  , m_iFormat(UVC_FRAME_FORMAT_YUYV)  //UVC_CAP_PROP_FORMAT
  , m_iWidth(640)
  , m_iHeight(480)
  , m_iFps(5)   // 30fps for Hi3516dv300; 15fps for NXP; 15fps for Cyclops;
#endif // USE_CYCLPOS_SG8021

#ifdef    USE_HI3516DV300
  , m_iFormat(UVC_CAP_MODE_MJPEG)  //UVC_CAP_PROP_FORMAT
  , m_iWidth(640)
  , m_iHeight(360)
  , m_iFps(30)   // 30fps for Hi3516dv300; 5fps for NXP; 15fps for Cyclops;
#endif

  , m_iStillWidth(640)
  , m_iStillHeight(480)
  , gucIsStillTrigger(0)
  , m_iWidth_set(0)
  , m_iHeight_set(0)

  , m_bIsVideoScreamStart(false)
  , m_ctx(NULL)
  , m_dev(NULL)
  , m_devh(NULL)
  , m_buffers(NULL)
  , id(-1)
  , hubPortNum(0)
  , m_SN("")
  , m_DecodeResult("")
  , m_LedBrightness(15)
  , m_Roi("")
  , m_DecodePrefix("")
  , m_DecodePostfix("")
  , m_DecodeMatchString("")
  , pCamList(NULL)
  , m_pCamLed(NULL) {
  uvc_error_t res = uvc_init(&m_ctx, NULL);
//  m_bInit = (res < 0) ? 0 : 1;
  if(res < 0)
     printf("uvc_init failed\n");

  m_Roi[0] = 0;
  m_Roi[1] = 0;
  m_Roi[8] = 640;
  m_Roi[9] = 480;

  // Initialize thread pid = 0
  pthread_monitor_tid = 0;//, sizeof(MAX_CAMERA_NUM));

  // Initialization
  gStatusCallBack.pCameraControl = NULL;
  gCamStatus = UVC_STATUS_READY;   //add by yuliang 190423

  for(int idx=0; idx<MAX_IMAGE_FORMAT_ITEM_NUM;idx++)
    gImageFormatConfig[idx].NeedUpdate = 0;

  // If not exist, and create SharedMem, else open it
//  uvcCreateShm(&shmid);
}


/**
\code
Function   :  ~UVC4Linux
Description:  Deconstruct function
Calls	   :  uvc_close
              uvc_unref_device
              uvc_exit
              uvcReleaseCameraCapture
Called By  :  SgCamera::~SgCamera
Input	   :  N/A
Output	   :  N/A
Return	   :  N/A
ChangeList :
\encode
*/
UVC4Linux::~UVC4Linux() {
  PRINTF_LOG("start");
  if(m_bInit) {
    if(m_bIsVideoScreamStart) {
      closeVideoStream();
    }

//    if(m_pCamLed) {
//      delete((CaptureLED*)m_pCamLed);
//    }

    if(m_devh) {
      uvc_close(m_devh);
    }
    uvc_unref_device(m_dev);

    uvc_exit(m_ctx);
    m_bInit = false;
  }

  // exit monitor pthread
  uvcReleaseCameraCapture(this);

  pthread_mutex_destroy(&this->m_statusMutex);PRINTF_LOG("End");
}


/**
\code
Function   :  getCaptureList
Description:  Scan libusb, get camera list
Calls	   :  uvc_free_device_list
			  uvc_get_device_descriptor
			  uvc_get_device_list
Called By  :  open
			  reopen
			  ScanCameraList
Input	   :  m_ctx
              vector ptr for devlist
Output	   :  N/A
Return	   :  UVC_SUCCESS           : succ
			  UVC_ERROR_NOT_FOUND	: failed
ChangeList :
\encode
*/
int getCaptureList(uvc_context_t* m_ctx, std::vector<uvc_device_t*>& list) {
  uvc_device_t** uvclist;
  PRINTF_LOG("Start");
  uvc_error_t res = uvc_get_device_list(m_ctx, &uvclist);

  if(res != UVC_SUCCESS) {
    uvc_perror(res, "uvc_get_device_list");
    return res;
  }

  int dev_idx = 0;
  uvc_device_t* test_dev;
  unsigned int vid = VID;
  unsigned int pid = PID;

//  int number_of_dev_found = sizeof(*uvclist)/ sizeof(uvc_device_t*);

  while((test_dev = uvclist[dev_idx++]) != NULL) {
//  while(dev_idx++ < number_of_dev_found)  {
//    printf("dev_idx: %d\n", dev_idx-1);
//    test_dev = uvclist[dev_idx];
    uvc_device_descriptor_t* desc;

    if(uvc_get_device_descriptor(test_dev, &desc) != UVC_SUCCESS) {
      continue;
    }

    // add by yuliang 20200423
    //printf("VID:[0x%x], PID:[0x%x]", desc->idProduct, desc->idVendor);

    if((!vid || desc->idVendor == vid)
        && (!pid || desc->idProduct == pid))
//    if((desc->idVendor == vid)
//        && (desc->idProduct == pid))
    {
      list.push_back(test_dev);
      uvc_ref_device(test_dev);
    }

    uvc_free_device_descriptor(desc);
  }

  uvc_free_device_list(uvclist, 1);

  if(0 == list.size()) {
    return UVC_ERROR_NOT_FOUND;
  }
  PRINTF_LOG("End");
  return UVC_SUCCESS;
}


// HID Class-Specific Requests values. See section 7.2 of the HID specifications
#define HID_GET_REPORT                0x01
#define HID_GET_IDLE                  0x02
#define HID_GET_PROTOCOL              0x03
#define HID_SET_REPORT                0x09
#define HID_SET_IDLE                  0x0A
#define HID_SET_PROTOCOL              0x0B
#define HID_REPORT_TYPE_INPUT         0x01
#define HID_REPORT_TYPE_OUTPUT        0x02
#define HID_REPORT_TYPE_FEATURE       0x03


#define ERR_EXIT(errcode) do {  printf("   %s\n", libusb_strerror((enum libusb_error)errcode)); return -1; } while (0)
#define CALL_CHECK(fcall) do { int _r=fcall; if (_r < 0) ERR_EXIT(_r); } while (0)
#define CALL_CHECK_CLOSE(fcall, hdl) do { int _r=fcall; if (_r < 0) { libusb_close(hdl); ERR_EXIT(_r); } } while (0)
#define B(x) (((x)!=0)?1:0)
#define be_to_int32(buf) (((buf)[0]<<24)|((buf)[1]<<16)|((buf)[2]<<8)|(buf)[3])

/**
\code
Function   :  HidSetCameraMode
Description:  Switch Hid DevMode to UVC,
Calls	   :  None
Called By  :  camera->Set_HidMode(int)
Input	   :  int mode
Output	   :  N/A
Return	   :  0   : succ
			  other	: failed
ChangeList :
\encode
*/
int setHidCameraMode(int vid, int pid, int mode)
{
    libusb_device_handle *handle;
    int r;

    /* Initial USB device */
	printf("Using libusb_initial\n");
	r = libusb_init(NULL);
	if (r < 0)
		return r;

    /* Open device */
	printf("Opening device VID:[%04X], PID:[%04X]\n", vid, pid);
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle == NULL) {
		printf("  Failed.\n");
		return -1;
	}

//	dev = libusb_get_device(handle);
//	bus = libusb_get_bus_number(dev);
//    r = libusb_get_port_numbers(dev, port_path, sizeof(port_path));
//    if (r > 0) {
//			printf("\nDevice properties:\n");
//			printf("        bus number: %d\n", bus);
//			printf("         port path: %d", port_path[0]);
//			for (i=1; i<r; i++) {
//				printf("->%d", port_path[i]);
//			}
//			printf(" (from root hub)\n");
//    }

	uint8_t data[2] = {0, 0};
	data[1] = mode;

	CALL_CHECK(libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE,
		HID_SET_REPORT, (HID_REPORT_TYPE_OUTPUT<<8)|0x00, 0, data, 2, 1000));

	printf("Closing device...\n");
	libusb_close(handle);

	return 0;
}


/**
\code
Function   :  getCameraIndex
Description:  Scan libusb, get camera index
Calls	   :  uvc_free_device_list
			  uvc_get_device_descriptor
			  uvc_get_device_list
Called By  :  open
			  reopen
			  ScanCameraList
Input	   :  m_ctx
              vector ptr for devlist
Output	   :  N/A
Return	   :  UVC_SUCCESS           : succ
			  UVC_ERROR_NOT_FOUND	: failed
ChangeList :
\encode
*/
int getCameraIndex(uvc_context_t* m_ctx, std::vector<uvc_device_t*>& list) {
  uvc_device_t** uvclist;PRINTF_LOG("Start");
  uvc_error_t res = uvc_get_device_list(m_ctx, &uvclist);

  if(res != UVC_SUCCESS) {
    uvc_perror(res, "uvc_get_device_list");
    return res;
  }

  int dev_idx;

  dev_idx = 0;
  uvc_device_t* test_dev;
  unsigned int vid = VID;
  unsigned int pid = PID;

  while((test_dev = uvclist[dev_idx++]) != NULL) {
    uvc_device_descriptor_t* desc;

    if(uvc_get_device_descriptor(test_dev, &desc) != UVC_SUCCESS) {
      continue;
    }

    if((!vid || desc->idVendor == vid)
        && (!pid || desc->idProduct == pid)) {
      list.push_back(test_dev);
      uvc_ref_device(test_dev);
    }

    uvc_free_device_descriptor(desc);
  }

  uvc_free_device_list(uvclist, 1);

  if(0 == list.size()) {
    return UVC_ERROR_NOT_FOUND;
  }
  PRINTF_LOG("End");
  return UVC_SUCCESS;
}


/**
\code
Function   :  open
Description:  open libuvc interface according index, port number of Hub
Calls      :  getCaptureList
              getSN
			  uvc_open
			  uvc_get_stream_ctrl_format_size
              openVideoStream
Called By  :  uvcCreateCameraCapture
Input      :  hubPortNum:  port numbers for hub(0, 1,2, 3,4)
Output     :  N/A
Return     :  false : failed
              true  : succ
ChangeList :
\encode
*/
bool UVC4Linux::open(int hubPortNum) {PRINTF_LOG("start id");
  uvc_error_t res;

  if(UVC_SUCCESS != getCaptureList(this->m_ctx, m_lsCap)) {
    m_bInit = false;
    return false;
  }

  if(MAX_CAMERA_NUM< (unsigned int)hubPortNum+1) {
    m_bInit = false;
    return false;
  }
  // find index according port number
//  int8_t index = 0;//FindIndexFromDevList(hubPortNum+1, m_lsCap);
  if(hubPortNum > (m_lsCap.size())-1)
     printf("hubPortNum is overflow\n");
  if(hubPortNum <0) {
	m_bInit = false;
    return false;
  }

  auto& dev = m_lsCap[hubPortNum];
  // libuvc open device
  res = uvc_open(dev, &this->m_devh);
  if(res < 0) {
    uvc_perror(res, "uvc_open");
    uvc_exit(this->m_ctx);
    m_bInit = false;
    return false;
  }

  res = uvc_get_stream_ctrl_format_size(
          m_devh, &m_ctrl, /* result stored in ctrl */
          (enum uvc_frame_format)m_iFormat, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
          m_iWidth, m_iHeight, m_iFps /* width, height, fps */
        );

  if(res < 0) {
    printf("get stream format failed\n");
    uvc_exit(m_ctx);
    m_bInit = false;
    return false;
  }

  // uvc still image configure
  res = uvc_get_still_ctrl_format_size(
	  	m_devh,
	  	&m_ctrl,
	  	&m_still_ctrl,
	  	m_iStillWidth,
	  	m_iStillHeight);
  if(res < 0) {
      printf("uvc_get_still_ctrl_format_size failed: %d\n", res);
  } else {
      printf("uvc_get_still_ctrl_format_size succ\n");
  }

  m_dev = m_lsCap[hubPortNum];
  return openVideoStream();
}


/**
\code
Function   :  getID
Description:  Get the current object, the corresponding video stream ID
Calls      :  N/A
Called By  :  N/A
Input      :  void
Output     :  N/A
Return     :  this->id
ChangeList :
\encode
*/
int UVC4Linux::getID(void) {
  return this->id;
}


/**
\code
Function   :  grabFrame
Description:  grab a image through libuvc interface
Calls      :  N/A
Called By  :  SgCamera::read
Input      :  char** data
Output     :  N/A
Return     :   -1: failed
              >0: pic_size
ChangeList :
\encode
*/
int UVC4Linux::grabFrame(unsigned char** data) {
  pthread_mutex_lock(&this->m_mutexReadFrame);
  m_bReadFrame = true;
  pthread_mutex_unlock(&this->m_mutexReadFrame);

  while(true == m_bReadFrame) {  // Waiting for flag
     std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  if(UVC_FRAME_FORMAT_UNKNOWN == m_buffers->frame_format) {
     return -1;
  }

  // Not grayscale, the default is 3 channels
  int iChannel = (UVC_FRAME_FORMAT_GRAY8 == m_buffers->frame_format) ? 1 : 3;

  int iNeedSize = m_buffers->height * m_buffers->width * iChannel;
  // Data pointer, size
  *data = (unsigned char*)m_buffers->data;

  if(UVC_CAP_MODE_MJPEG == m_iFormat || UVC_CAP_MODE_YUYV == m_iFormat)  {
    iNeedSize = m_buffers->data_bytes;
  }

  return iNeedSize;
}


/**
\code
Function   :  getRange
Description:  get Range of Capture Parameter
Calls      :    PropertyRange
Called By  :    icvGetPropertyCAM_V4L
                icvSetPropertyCAM_V4L
Input      :  property_id
Output     :  N/A
Return     :  PropertyRange
ChangeList :
\encode
*/
PropertyRange UVC4Linux::getRange(int property_id) const {
  switch(property_id) {
  case UVC_CAP_PROP_BRIGHTNESS:
    return PropertyRange(0, 8);

  case UVC_CAP_PROP_CONTRAST:
    return PropertyRange(0, 6);

  case UVC_CAP_PROP_SATURATION:
    return PropertyRange(0, 6);

  case UVC_CAP_PROP_HUE:
    return PropertyRange(-40, 40);

  case UVC_CAP_PROP_GAIN:
    return PropertyRange(0, 100);

  case UVC_CAP_PROP_EXPOSURE:
    return PropertyRange(15, 5000);

  case UVC_CAP_PROP_FOCUS:
    return PropertyRange(0, 1000);

  case UVC_CAP_PROP_AUTOFOCUS:
    return PropertyRange(0, 1);

  case UVC_CAP_PROP_AUTO_EXPOSURE:
    return PropertyRange(1, 2);

  case UVC_CAP_PROP_BACKLIGHT:       //add by yuliang  (0~1)
	return PropertyRange(0, 191);

  case UVC_CAP_PROP_DECODE_TRIG:
	return PropertyRange(0, 1);

  default:
    return PropertyRange(0, 255);
  }
}


/**
\code
Function   :  ControlsStatusCallback
Description:  libuvc state response  callback
Calls      :  N/A
Called By  :  openvideostream
Input      :   status_class
               event
               selector
               status_attribute
               data
               data_len
               void* param
Output     :  N/A
Return     :  N/A
ChangeList :
\encode
*/
void ControlsStatusCallback(enum uvc_status_class status_class, int event,
                            int selector,
                            enum uvc_status_attribute status_attribute, void* data, size_t data_len,
                            void* ptr) {
  // videostream control status
  printf("Controls callback. class: %d, event: %d, selector: %d, attr: %d, data_len: %zu\n",
         status_class, event, selector, status_attribute, data_len);

  if(status_attribute == UVC_STATUS_ATTRIBUTE_VALUE_CHANGE) {
    // State type judgment
    switch(status_class) {
    default:
      cout << "status_class err" << endl;
      break;
    }
  }
}


/**
\code
Function   :  openVideoStream
Description:  open video stream
Calls      :  	uvc_get_stream_ctrl_format_size
				uvc_set_status_callback
				uvc_start_streaming
				setStatus
				setProperty
Called By  :  open
              icvSetPropertyCAM_V4L
Input      :  void* param
Output     :  N/A
Return     :  true : succ
              false: failed
ChangeList :
\encode
*/
bool UVC4Linux::openVideoStream() {PRINTF_LOG("Start");
  if(NULL == m_devh) {  // m_bInit flag should turn on after all operation is OK
    return false;
  }

  uvc_error_t res = uvc_get_stream_ctrl_format_size(
                      m_devh, &m_ctrl, /* result stored in ctrl */
                      (enum uvc_frame_format)m_iFormat, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
                      m_iWidth, m_iHeight, m_iFps /* width, height, fps */
                    );

  if(res < 0) {
    uvc_exit(m_ctx);
    m_bInit = false;
	printf("%s, get stream ctrl format failed\n", __func__);
    return false;
  }

  /* Print out the result */
  //uvc_print_stream_ctrl(&m_ctrl, stderr);   // add by yuliang

  // Initialize the pointer to the callback field, as well as the content
  gStatusCallBack.pCameraControl = (void*)this;
  gStatusCallBack.bStatus = 0;

  // Set the status response callback function
  uvc_set_status_callback(m_devh, &ControlsStatusCallback,
                          (void*)&gStatusCallBack);
  // Save still_ctrl pointer to m_ctrl object
  m_ctrl.pStillCtrl = &m_still_ctrl;

  res = uvc_start_streaming(m_devh, &m_ctrl, getFrameCallBack, (void*)this, 0);
  if(res < 0) {
    printf("start stream failed\n");
    return false;
  }

  m_bInit = true;
  m_bIsVideoScreamStart = true;
  setStatus(UVC_STATUS_OPEN);

  // set property for camera, Only Set Property when Open Status, 20190428  yuliang
//  setProperty(UVC_CAP_PROP_BACKLIGHT, 0.05);

// synchron status with sharedMem
//  uvcSyncShmStatus(shmid, this->hubPortNum+1, pCamList, UVC_STATUS_OPEN, getID());
  printf("Open videostream succ %s\n", this->m_strDeviceSN.c_str());
  PRINTF_LOG("End");
  return true;
}


/*
\code
Function   :  closeVideoStream
Description:  close video stream
Calls      :  uvc_stop_streaming
Called By  :  ~UVC4Linux
              icvSetPropertyCAM_V4L
Input      :  void* param
Output     :  N/A
Return     :  true : succ
              false: failed
ChangeList :
\encode
*/
bool UVC4Linux::closeVideoStream() {PRINTF_LOG("start");
  if((false == m_bInit) || (NULL == m_devh)) {
    printf("Stream Already Close!\n");
    return false;
  }

  uvc_stop_streaming(m_devh);
  m_bIsVideoScreamStart = false;
  // setStatus(UVC_STATUS_CLOSE);     // override it by yuliang 20190424
  printf("Close videostream succ\n");PRINTF_LOG("End");
  return true;
}


/**
\code
Function   :  pthread_monitor
Description:  UVC event monitoring thread,
              One video thread for each video stream
Calls      :  setStatus
		      updateStatus
		      reopen
Called By  :  open
Input      :  void* param
Output     :  N/A
Return     :  N/A
ChangeList :
\encode
*/
static void* pthread_monitor(void* param) {
  uint8_t bRetryCnt = 0;

  UVC4Linux* pcap = (UVC4Linux*)param;
  printf("Enter uvc monitor pthread------>\n");
  pcap->pthread_monitor_tid = 1;
  // delay for main thread to ready
  //sleep(1);

  while(1)  {
    switch(pcap->gCamStatus) {
    case UVC_STATUS_OPEN:
      if(pcap->gStatusCallBack.bStatus != LIBUSB_TRANSFER_COMPLETED) {
        printf("libusb status is changeed\nStatus: %d\n",
               pcap->gStatusCallBack.bStatus);
      }

      // Check whether is status callback
      switch(pcap->gStatusCallBack.bStatus)  {
      /** Transfer failed */
      case LIBUSB_TRANSFER_ERROR:
        // break;
      printf("transfer status err\n");

      /** Transfer timed out */
      case LIBUSB_TRANSFER_TIMED_OUT:
        // break;

      /** Transfer was cancelled */
      case LIBUSB_TRANSFER_CANCELLED:
        // break;

      /** Device was disconnected */
      case LIBUSB_TRANSFER_NO_DEVICE:
        printf("Device is disappeared\n");
        // Change status of video stream for current idx
        pcap->setStatus(UVC_STATUS_RECON);
        break;

      /** For bulk/interrupt endpoints: halt condition detected (endpoint
       * stalled). For control endpoints: control request not supported.
       */
      case LIBUSB_TRANSFER_STALL:
        break;

      /** Device sent more data than requested */
      case LIBUSB_TRANSFER_OVERFLOW:
        break;
      }

      // Clear callback Status bits
      pcap->gStatusCallBack.bStatus = LIBUSB_TRANSFER_COMPLETED;
      break;

    case UVC_STATUS_CLOSE:
      // close vs again
      usleep(1000*1000);

      if(pcap->closeVideoStream() == false) {
        printf("Stop VideoStream again faild\n");
        pcap->setStatus(UVC_STATUS_CLOSE);
        break;
      } else {
        pcap->setStatus(UVC_STATUS_RECON);
      }

    case UVC_STATUS_RECON:
      bRetryCnt++;

      // Reopen idx
      if(pcap->Reopen()) {
        printf("camera Reopen succ\nid: %d\n", pcap->id);PRINTF_LOG("Reopen succ");
      }

      break;

    case UVC_STATUS_EXIT:
      printf("UVC monitor exit ----->\n");PRINTF_LOG("Mon exit");
      goto EXIT;

    case UVC_STATUS_READY:
      pcap->updateStatus();

    default:
      break;
    }

    // Query once every 100ms and process it
    usleep(1000*1000);

  }

EXIT:
  pthread_exit(0);
}


/**
\code
Function   :  updateStatus
Description:  State machine, check status
Calls      :  setStatus
Called By  :  pthread_monitor
Input      :  void
Output     :  N/A
Return     :  N/A
ChangeList :
\encode
*/
uint8_t UVC4Linux::updateStatus(void) {
  if(this->id >= 0) { // initial succ
    if((true == this->m_bInit) && (NULL != this->m_devh)) {
      if(this->m_bIsVideoScreamStart == true) {
        setStatus(UVC_STATUS_OPEN); // start videostream
      }
    }
  }

  return 0;
}


/**
\code
Function   :  setStatus
Description:  Set the connection status
Calls      :  uvc_init
		      this->getCaptureList
Called By  :  pthread_monitor
Input      :  cameraList: The struct which Stored camera info
              num       : Max Numbers of camera
Output     :  N/A
Return     :  false
              true  reopen success
ChangeList :
\encode
*/
void UVC4Linux::setStatus(uint8_t stat) {
  pthread_mutex_lock(&this->m_statusMutex);
  this->gCamStatus = stat;
  pthread_mutex_unlock(&this->m_statusMutex);
}


/**
\code
Function   :  FindIndexFromDevList
Description:  Find Index From Device List
Calls      :  uvc_get_port_number
Called By  :  Reopen
Input      :  cameraList: The struct which Stored camera info
              hubPortNum: port numbers of hub
Output     :  N/A
Return     :  -1
              index for dev list
ChangeList :
\encode
*/
int8_t UVC4Linux::FindIndexFromDevList(uint8_t hubPortNum, std::vector<uvc_device_t*> list)
{
	uint8_t idx =0;
	for( auto& dev : list)  {
		if(hubPortNum == uvc_get_port_number(dev))
			return idx;
		idx++;
	}

	return -1;
}


/**
\code
Function   :  Reopen
Description:  Reopen usb interface, and restart videostream
Calls      :  uvc_init
		      this->getCaptureList
Called By  :  pthread_monitor
Input      :  cameraList: The struct which Stored camera info
              num       : Max Numbers of camera
Output     :  N/A
Return     :  false
              true  reopen success
ChangeList :
\encode
*/
bool UVC4Linux::Reopen(void) {
  uvc_error_t res;

  // clear DeviceInterface and clear Container
  if(m_devh != NULL) {
    m_lsCap.clear();
    uvc_break(m_devh);
    m_devh = NULL;
    printf("uvc_close without stream\n");
  }

  res = uvc_init(&m_ctx, NULL);
  if(res != UVC_SUCCESS) {
    //printf("timeout, uvc_exit\n");
    uvc_exit(m_ctx);   // fix the bug for long time disconnect, libusb_init failed
    return false;
  }
  m_lsCap.clear();	  // Clearing device list
  //  scan device list
  if(getCaptureList(this->m_ctx, m_lsCap) == UVC_SUCCESS) {
    m_devh = NULL;

    if(m_lsCap.size() > 0) { // Have equipment available
      printf("uvc insert A device succ\n");
      printf("uvc device Number:%d \n", m_lsCap.size());
	  // Insert a Match Port Number device
      m_lsCap.clear();  // Clearing device list
      // Re-open the device according to the sn code to avoid id confusion and open another device
      if(open(this->hubPortNum))  {
          // Fresh image Configure
          bool ret = FreshProperty(UVC_CAP_PROP_END, 0);
          if(ret == false) {
            printf("Fresh Image Param failed\n");
          }

          // Switch status after open finished
          setStatus(UVC_STATUS_OPEN);
          return true;
	  }

    }

    //m_lsCap.pop_back();  //????
  }

  return false;
}

/**
\code
Function   : getUsbCameraList()
Description: brief Get a list of the USB devices attached to the system
Calls      :  libUsb_init
              libusb_get_device_descriptor
		      libusb_get_config_descriptor
		      libusb_exit
Called By  :  getUsbList()
Input      :  pDeviceList , a string, maximun size is 300 bytes
              num       : Max Numbers of camera
Output     :  None
Return     :  Error for <0;
              >0 other for deviceNumbers
ChangeList :
\encode
*/
int getUsbCameraList(char* pDeviceList)
{
    struct libusb_device **usb_dev_list;
    struct libusb_device *dev;
    int num_usb_devices;
    struct libusb_context* ctx;
    int r, i =0;   /* return value */
    char id[100];

    // reset memory buffer
    memset(pDeviceList, 0, 300);
    /* initial  libusb  */
    //printf("libusb_initial\n");
    r = libusb_init(&ctx);
    if (r < 0)
        return r;

    num_usb_devices = libusb_get_device_list(ctx, &usb_dev_list);
    if(num_usb_devices < 0)
		return NULL;

    //printf("libusb getlist[%d] succ\n", num_usb_devices);
    while ((dev = usb_dev_list[i++]) != NULL) {
        /* Got Usb Device Descriptor */
		struct libusb_device_descriptor devDesc;
		r = libusb_get_device_descriptor(dev, &devDesc);
		if (r < 0)
			return r;   // if error, exit

        /* Got Usb Device Configure Descriptor  */
        struct libusb_config_descriptor *devConfigDesc;
        if (libusb_get_config_descriptor(dev, 0, &devConfigDesc) != 0)
            continue;

		if (devDesc.idVendor == VID && devDesc.idProduct == PID) {
            sprintf(id, "{\"Type\":\"UVC\", \"VID\":0x%4x, \"PID\":0x%4x}", devDesc.idVendor, devDesc.idProduct);
            //printf("ID: %s\n", id);
            strcat(pDeviceList, id);
        }
        if(devDesc.idVendor == HID_VID && devDesc.idProduct == HID_PID) {
            sprintf(id, "{\"Type\":\"HID\", \"VID\":0x%4x, \"PID\":0x%4x}", devDesc.idVendor, devDesc.idProduct);
            //printf("ID: %s\n", id);
            strcat(pDeviceList, id);
		}

		// 获取接口描述符
		struct libusb_interface_descriptor* pCtrlDesc;
		pCtrlDesc = (struct libusb_interface_descriptor*)devConfigDesc->interface[0].altsetting;
		printf("\n---------------------------\n");
		for(int i=0; i<pCtrlDesc->extra_length; i++)
            printf("0X%2x ", *(pCtrlDesc->extra+i));
        printf("\n---------------------------\n");

		// free memory for config descriptor
        libusb_free_config_descriptor(devConfigDesc);
	}

	//  free device list
    libusb_free_device_list(usb_dev_list, 1);
    if (ctx != NULL)
        libusb_exit(ctx);

    // return DeviceNumbers
    return num_usb_devices;
}


/**
\code
Function   :  ComposeSetUserCmd
Description:  Send User Command through Compose Winusb sys driver
Calls	   :  None
Called By  :  camera->Set_HidMode(int)
Input	   :  int vid,
                int pid,
                uint8_t* buf,
                int Len
Output	   :  N/A
Return	   :  0   : succ
			  other	: failed
ChangeList :
\encode
*/
int ComposeSetUserCmd(int vid, int pid, uint8_t* buf, int wLen)
{
    libusb_device_handle *handle;
    struct libusb_context *ctx;
    int r;

    /* Initial USB device */
	printf("Using libusb_initial\n");
	r = libusb_init(NULL);
	if (r < 0)
		return r;

    /* Open device */
	printf("Opening ComposeDevice VID:[%04X], PID:[%04X]\n", vid, pid);
	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle == NULL) {
		printf("Libusb Open Failed.\n");
		return -1;
	} else {
        printf("Libusb Open Succ\n");
	}

	uint8_t data[64] = {0};
	memcpy(data, buf, wLen);

	CALL_CHECK(libusb_control_transfer(handle, LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE,
		HID_SET_REPORT, (HID_REPORT_TYPE_OUTPUT<<8)|0x00, 0, data, 2, 1000));

	printf("Closing device...\n");
	libusb_close(handle);

	return 0;
}
