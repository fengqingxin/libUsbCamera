#ifndef __UVC_CAMERA_H__
#define __UVC_CAMERA_H__

#include <string>
//#include "opencv2/core/mat.hpp"
#include "uvcCapture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UVCCapture UVCCapture;

enum UVCCaptureProperties {
  UVC_CAP_PROP_POS_MSEC       = 0, //!< Current position of the video file in milliseconds.
  UVC_CAP_PROP_POS_FRAMES     = 1, //!< 0-based index of the frame to be decoded/captured next.
  UVC_CAP_PROP_POS_AVI_RATIO  = 2, //!< Relative position of the video file: 0=start of the film, 1=end of the film.
  UVC_CAP_PROP_FRAME_WIDTH    = 3, //!< Width of the frames in the video stream.
  UVC_CAP_PROP_FRAME_HEIGHT   = 4, //!< Height of the frames in the video stream.
  UVC_CAP_PROP_FPS            = 5, //!< Frame rate.
  UVC_CAP_PROP_FOURCC         = 6, //!< 4-character code of codec. see VideoWriter::fourcc .
  UVC_CAP_PROP_FRAME_COUNT    = 7, //!< Number of frames in the video file.
  UVC_CAP_PROP_FORMAT         = 8, //!< Format of the %Mat objects returned by VideoCapture::retrieve().
  UVC_CAP_PROP_MODE           = 9, //!< Backend-specific value indicating the current capture mode.
  UVC_CAP_PROP_BRIGHTNESS    = 10, //!< Brightness of the image (only for those cameras that support).
  UVC_CAP_PROP_CONTRAST      = 11, //!< Contrast of the image (only for cameras).
  UVC_CAP_PROP_SATURATION    = 12, //!< Saturation of the image (only for cameras).
  UVC_CAP_PROP_HUE           = 13, //!< Hue of the image (only for cameras).
  UVC_CAP_PROP_GAIN          = 14, //!< Gain of the image (only for those cameras that support).
  UVC_CAP_PROP_EXPOSURE      = 15, //!< Exposure (only for those cameras that support).
  UVC_CAP_PROP_CONVERT_RGB   = 16, //!< Boolean flags indicating whether images should be converted to RGB.
  UVC_CAP_PROP_WHITE_BALANCE_BLUE_U = 17, //!< Currently unsupported.
  UVC_CAP_PROP_RECTIFICATION = 18, //!< Rectification flag for stereo cameras (note: only supported by DC1394 v 2.x backend currently).
  UVC_CAP_PROP_MONOCHROME    = 19,
  UVC_CAP_PROP_SHARPNESS     = 20,
  UVC_CAP_PROP_AUTO_EXPOSURE = 21, //!< DC1394: exposure control done by camera, user can adjust reference level using this feature.
  UVC_CAP_PROP_GAMMA         = 22,
  UVC_CAP_PROP_TEMPERATURE   = 23,
  UVC_CAP_PROP_TRIGGER       = 24,
  UVC_CAP_PROP_TRIGGER_DELAY = 25,
  UVC_CAP_PROP_WHITE_BALANCE_RED_V = 26,
  UVC_CAP_PROP_ZOOM          = 27,
  UVC_CAP_PROP_FOCUS         = 28,
  UVC_CAP_PROP_GUID          = 29,
  UVC_CAP_PROP_ISO_SPEED     = 30,
  UVC_CAP_PROP_BACKLIGHT     = 32,
  UVC_CAP_PROP_PAN           = 33,
  UVC_CAP_PROP_TILT          = 34,
  UVC_CAP_PROP_ROLL          = 35,
  UVC_CAP_PROP_IRIS          = 36,
  UVC_CAP_PROP_SETTINGS      = 37, //!< Pop up video/camera filter dialog (note: only supported by DSHOW backend currently. The property value is ignored)
  UVC_CAP_PROP_BUFFERSIZE    = 38,
  UVC_CAP_PROP_AUTOFOCUS     = 39,
  UVC_CAP_PROP_SAR_NUM       = 40, //!< Sample aspect ratio: num/den (num)
  UVC_CAP_PROP_SAR_DEN       = 41, //!< Sample aspect ratio: num/den (den)
  UVC_CAP_PROP_OPEN_STAT     = 42,
  UVC_CAP_PROP_DECODE_RESULT = 43,
  UVC_CAP_PROP_SN_SG_NXP     = 44,
  UVC_CAP_PROP_DECODE_TRIG   = 45,
  UVC_CAP_PROP_ROI           = 46,
  UVC_CAP_PROP_DECODE_TYPE   = 47,
  UVC_CAP_PROP_DECODE_MODE   = 48,
  UVC_CAP_PROP_CAMERA_MODE   = 49,
  UVC_CAP_PROP_PREFIX_STRING = 50,
  UVC_CAP_PROP_POST_STRING   = 51,
  UVC_CAP_PROP_MATCH_STRING  = 52,
  UVC_CAP_PROP_REREAD_TIMEOUT= 53,
  UVC_CAP_PROP_DECODE_TIMEOUT= 54,
  UVC_CAP_PROP_PRODUCT_INFO  = 55,
  UVC_CAP_PROP_SCALE_ZOOM_MOD= 56,
  UVC_CAP_PROP_FLIP_MIRROR   = 57,
  UVC_CAP_PROP_UPDATE_PLG    = 58,
  UVC_CAP_PROP_USER_CMD      = 59,
  UVC_CAP_PROP_CAMERA_INFO   = 60,
  UVC_CAP_PROP_END,
};

enum UVCCaptureModes {
  UVC_CAP_MODE_BGR  = 0, //!< BGR24 (default)
  UVC_CAP_MODE_GRAY = 1, //!< Y8
  UVC_CAP_MODE_R    = 2, //!< R8
  UVC_CAP_MODE_B    = 3, //!< G8
  UVC_CAP_MODE_G    = 4  //!< B8
};

enum UVCCaptureFormat {
  UVC_CAP_MODE_YUV420P = 1,
  UVC_CAP_MODE_YUYV  = 3,
  UVC_CAP_MODE_MJPEG = 7
};


class SgCamera {
 public:
  explicit SgCamera();
  explicit SgCamera(int index);
  explicit SgCamera(const std::string& cameraSN);

  virtual ~SgCamera();

  bool open(const std::string& cameraSN);
  bool open(int index);
  bool HidSwitchToUvcMode(void);
  bool getUsbList(char* pDevList);
  bool isOpened() const;
  void close();
  bool read(uint8_t* image);
  int set(int propId, double value);
  double get(int propId) const;

 protected:
  UVCCapture* m_Cap;

};

/************  Extern function declare *****************/
extern int setHidCameraMode(int vid, int pid, int mode);
extern int getUsbCameraList(char* pDeviceList);

#ifdef __cplusplus
}
#endif

#endif //__CAPTURE_SONIX_HPP__
