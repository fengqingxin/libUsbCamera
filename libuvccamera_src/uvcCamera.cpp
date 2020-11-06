//#include "opencv2/opencv.hpp"
#include <string.h>
#include "uvcCamera.h"
#include <unistd.h>


using namespace std;

SgCamera::SgCamera()
  : m_Cap(NULL) {
}

SgCamera::SgCamera(int index)
  : m_Cap(NULL) {
  open(index);
}

SgCamera::~SgCamera() {
  close();
}


/**
\code
Function   :  open
Description:  Open a camera with index
Calls	   :  uvcCreateCameraCapture
              isOpened
Called By  :  N/A
Input	   :  CameraSN
Output	   :  N/A
Return	   :  true  : succ
              false : failed
ChangeList :
\encode
*/
bool SgCamera::open(int index) {
  if(isOpened()) {
    close();
  }

  UVCCapture* capture = uvcCreateCameraCapture(index);

  if(capture) {
    if(m_Cap) {
      delete m_Cap;
    }

    m_Cap = capture;
    return true;
  }

  return false;
}


/**
\code
Function   :  setHidMode
Description:  set Hid device Mode
Calls	   :  setHidMode
              isOpened
Called By  :  N/A
Input	   :  CameraSN
Output	   :  N/A
Return	   :  true  : succ
              false : failed
ChangeList :
\encode
*/
bool SgCamera::HidSwitchToUvcMode(void) {
  int vid = HID_VID;
  int pid = HID_PID;

  /*  设置相机工作模式
    CAMERA_WORK_MODE_UVC    = 1,
    CAMERA_WORK_MODE_HID    = 2,
  */
  if(setHidCameraMode(vid, pid, 1) <0) {
      printf("Set HID mode failed\n");
      return false;
  }

  return true;
}


/**
\code
Function   :  getUsbList
Description:  get Usb device List, result is a json string
Calls	   :  getUsbCameraList
Called By  :  N/A
Input	   :  CameraSN
Output	   :  N/A
Return	   :  true  : succ
              false : failed
ChangeList :
\encode
*/
bool SgCamera::getUsbList(char* pDevList)
{
  if(getUsbCameraList(pDevList) <0) {
     printf("Get Usb List failed\n");
     return false;
  }

  return true;
}


/**
\code
Function   :  isOpened
Description:  judge whether a camera is opened
Calls	   :  get
Called By  :  N/A
Input	   :  N/A
Output	   :  N/A
Return	   :  true  : opened
              false : close
ChangeList :
\encode
*/
bool SgCamera::isOpened() const {PRINTF_LOG("Start");
//    return !cap;  // legacy interface doesn't support closed files
  // add by yuliang
  if((m_Cap != NULL) && ((int)get(UVC_CAP_PROP_OPEN_STAT) == UVC_STATUS_OPEN)) {PRINTF_LOG("End1");
    return true;
  }
  PRINTF_LOG("End0");
  return false;
}


/**
\code
Function   :  release
Description:  release the handle for current Camera
Calls	   :  ~UVC4Linux
Called By  :  N/A
Input	   :  N/A
Output	   :  N/A
Return	   :  N/A
ChangeList :
\encode
*/
void SgCamera::close() {PRINTF_LOG("Start");

  if(m_Cap) {
    delete m_Cap;
  }

  m_Cap = NULL;
  PRINTF_LOG("End");
}


/**
\code
Function   :  read
Description:  Grab a image after camera is opened
Calls	   :  grabFrame
              get
Called By  :  N/A
Input	   :  N/A
Output	   :  N/A
Return	   :  true : succ
              false: failed
ChangeList :
\encode
*/
bool SgCamera::read(uint8_t* image) {
  PRINTF_LOG("Start");
  // modify by yuliang 20190426, Reordering to avoid zero width and height
  unsigned char* data = NULL;
  int length = m_Cap->grabFrame(&data);

  int iWidth = get(UVC_CAP_PROP_FRAME_WIDTH);
  int iHeight = get(UVC_CAP_PROP_FRAME_HEIGHT);
  int ichannel = (UVC_CAP_MODE_GRAY == get(UVC_CAP_PROP_MODE)) ? 1 : 3;
  const int ibufferSize = iWidth * iHeight * ichannel;

  if(UVC_CAP_MODE_MJPEG == get(UVC_CAP_PROP_FORMAT)) {
    //std::vector<uint8_t> jpgData(data, data + length);
    //img = cv::imdecode(jpgData, CV_LOAD_IMAGE_COLOR);

    if(UVC_CAP_MODE_GRAY == get(UVC_CAP_PROP_MODE)) {
      //cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
    }
  } else {
    if(ibufferSize < length) {
       printf("read length is err, copy length:%d\n", length);
       //return false;
    }
    //img = cv::Mat(iHeight, iWidth, CV_8UC(ichannel), data);
    memcpy(image, data, length);
  }

  //img.copyTo(image);
  PRINTF_LOG("End");
  return true;

}


/**
\code
Function   :  set
Description:  config camera's param item
Calls	   :  setProperty
Called By  :  N/A
Input	   :  N/A
Output	   :  N/A
Return	   :  true : succ
              false: failed
ChangeList :
\encode
*/
int SgCamera::set(int propId, double value) {
  return (m_Cap ? m_Cap->setProperty(propId, value) : -2);
  return false;
}


/**
\code
Function   :  get
Description:  Get the configure camera's param item
Calls	   :  getProperty
Called By  :  N/A
Input	   :  N/A
Output	   :  N/A
Return	   :  double : succ
              false  : failed
ChangeList :
\encode
*/
double SgCamera::get(int propId) const {
  return m_Cap ? m_Cap->getProperty(propId) : 0;
  return false;
}


