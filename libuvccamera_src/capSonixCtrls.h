#ifndef _CAP_SONIX_XU_CTRLS_H
#define _CAP_SONIX_XU_CTRLS_H

#include <vector>
#include <string>
#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#import "cap_UVC.h"
#elif defined(__linux__)
#include <linux/videodev2.h>
#include <linux/version.h>
// #if LINUX_VERSION_CODE > KERNEL_VERSION (3, 0, 36)
#include <linux/uvcvideo.h>
// #endif
#endif

#define   SN_LEN    16

typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned int __u32;

#define UBYTE unsigned char

#if defined(__APPLE__)
@interface SonixCtrls : NSObject {
  UVCCameraControl* cameraControl;
}

- (id)initWithControler:(UVCCameraControl*)handle;
- (int)XU_Set_Cur:(__u8)xu_unit selector:(__u8)xu_selector size:
  (__u16)xu_size data:(__u8*)xu_data;
- (int)XU_Get_Cur:(__u8)xu_unit selector:(__u8)xu_selector size:
  (__u16)xu_size data:(__u8*)xu_data;
- (int)XU_Asic_Read:(unsigned int)Addr data:(unsigned char*)AsicData;
- (int)XU_Asic_Write:(unsigned int)Addr data:(unsigned char)AsicData;
- (int)XU_I2C_ReadEx:(unsigned char)SlaveID addr:(unsigned int)I2C_Addr addrNum
  :(unsigned char)AddrNum data:(unsigned char*)pData dataLen:
  (unsigned char)DataLen;
- (int)XU_I2C_WriteEx:(unsigned char)SlaveID i2cAddr:(unsigned int)I2C_Addr
  addrNum:(unsigned char)AddrNum data:(unsigned int)Data dataLen:
  (unsigned char)DataNum;
- (int)XU_SF_Read:(unsigned int)Addr data:(unsigned char*)pData length:
  (unsigned int)Length;
- (void)SF_CMDread_Status;
- (void)DisableWriteProtect:(unsigned char)bWriteProtect;
- (void)DisableAddr:(unsigned short)Addr bit:(unsigned char)Addr_bit;
- (void)EnableAddr:(unsigned short)Addr bit:(unsigned char)Addr_bit;
- (int)SectorErase:(long)lStartAddr;
- (int)XU_SF_Write:(long)laddr data:(unsigned char*)uData length:(int)iLen;
- (int)ReadAsicBit:(long)Addr bit:(unsigned char)Addr_bit;
- (void)bSF_WaitRdy;
- (void)SF_ReadId:(unsigned char)ubCmd dummyNum:(unsigned char)ubDummyNum
  decIdNum:(unsigned char)ubDevIdNum;
- (int)SF_Identify;
@end

@interface CameraSN : NSObject {
  UVCCameraControl* cameraControl;
}
- (id)initWithControler:(UVCCameraControl*)handle;
- (bool)GetSN:(unsigned char)cSn length:(int)nLen;
@end

@interface CaptureLED : NSObject {
  UVCCameraControl* cameraControl;
  unsigned char regLED;
  unsigned char value[3];
}
- (id)initWithControler:(UVCCameraControl*)handle;
- (void)Init;
- (bool)SetValue:(int)nChannel value:(double)dValue;
- (double)GetValue:(int)nChannel;
@end

#else
class SonixCtrls {
 public:
  SonixCtrls(void* fd);
  virtual ~SonixCtrls();

 private:
  int XU_Set_Cur(__u8 xu_unit, __u8 xu_selector, __u16 xu_size, __u8* xu_data);
  int XU_Get_Cur(__u8 xu_unit, __u8 xu_selector, __u16 xu_size, __u8* xu_data);
  void SF_CMDread_Status();
  void DisableWriteProtect(unsigned char bWriteProtect);
  void DisableAddr(unsigned short Addr,  unsigned char Addr_bit);
  void EnableAddr(unsigned short Addr,  unsigned char Addr_bit);
  int PageErase(long lStartAddr);
  int SectorErase(long lStartAddr);
  int ReadAsicBit(long Addr,  unsigned char Addr_bit);
  void bSF_WaitRdy();
  void SF_ReadId(unsigned char ubCmd, unsigned char ubDummyNum,
                 unsigned char ubDevIdNum);

  UBYTE ubSFLib_GetIDSize(void);
  unsigned char ubSF_Search(void);
  int SF_Identify();

 public:
  int XU_Asic_Read(unsigned int Addr, unsigned char* AsicData);
  int XU_Asic_Write(unsigned int Addr, unsigned char AsicData);

  int XU_I2C_ReadEx(unsigned char SlaveID, unsigned int I2C_Addr,
                    unsigned char AddrNum, unsigned char* pData, unsigned char DataNum);
  int XU_I2C_WriteEx(unsigned char SlaveID, unsigned int I2C_Addr,
                     unsigned char AddrNum, unsigned int Data, unsigned char DataNum);
  int XU_SF_Read(unsigned int Addr, unsigned char* pData, unsigned int Length);
  int XU_SF_Write(long laddr, unsigned char uData[], int iLen);

 private:
  void* m_fd;
};

class CameraSN {
 public:
  CameraSN(void* fd);
  bool GetSN(std::string* strSn);
 private:
  void* m_fd;
};

class CaptureLED {
 public:
  CaptureLED(void* fd);
  virtual ~CaptureLED();
  void Init();
  bool SetValue(const int nChannel, const double dValue);
  double GetValue(const int nChannel) const;

 private:
  void* m_fd;
  unsigned char regLED;
  double m_dValue[3];
};

class CaptureFlash {
 public:
  CaptureFlash(void* fd);
  int Write(const std::string& str);
  int Read(std::string& str, int len);
 private:
  void* m_fd;
};

#endif
#endif
