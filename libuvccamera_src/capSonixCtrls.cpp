#define N_(x) x

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <chrono>
#include <thread>
// #include <sys/ioctl.h>
#include "capSonixCtrls.h"
#include "libuvc.h"

using namespace std;

SonixCtrls::SonixCtrls(void* fd) {
  m_fd  = fd;
}
SonixCtrls::~SonixCtrls() {

}

int SonixCtrls::XU_Asic_Read(unsigned int Addr, unsigned char* AsicData) {
  uvc_device_handle_t* devh = (uvc_device_handle_t*)m_fd;
  uvc_error_t ret = uvc_get_xu_asic(devh, Addr, AsicData);
  return (UVC_SUCCESS == ret) ? 0 : ret;
}

int SonixCtrls::XU_Asic_Write(unsigned int Addr, unsigned char AsicData) {
  uvc_device_handle_t* devh = (uvc_device_handle_t*)m_fd;
  uvc_error_t ret = uvc_set_xu_asic(devh, Addr, AsicData);
//  return (UVC_SUCCESS == ret) ? 0 : ret;

  if(ret== UVC_SUCCESS) {
    return 0;
  } else {
    printf("XU_Asic_Write failed\n");
    return ret;
  }
}

/*
int XU_I2C_Read(unsigned char slaveID,unsigned int address,long I2CDataLength,unsigned char* pData)
{
  int ret = 0;
  __u8 ctrldata[8];
  int i = 0;

  if(I2CDataLength < 1) return 0;
  if(I2CDataLength > 4) I2CDataLength = 4;

  __u8 xu_unit = 3;
  __u8 xu_selector = XU_SONIX_SYS_I2C_RW;
  __u16 xu_size= 8;
  __u8 *xu_data= ctrldata;

  xu_data[0] = slaveID;
  xu_data[1] = (char)I2CDataLength;
  xu_data[2] = (address & 0xff);
  xu_data[3] = ((address>>8) & 0xff);
  xu_data[7] = 0xff;


  if ((ret=XU_Set_Cur(xu_unit, xu_selector, xu_size, xu_data)) < 0)
  {
    TestAp_Printf(TESTAP_DBG_ERR,"   ioctl(UVCIOC_CTRL_SET) FAILED (%i) \n",ret);
    if(ret==EINVAL)     TestAp_Printf(TESTAP_DBG_ERR,"    Invalid arguments\n");
    return ret;
  }

  //Asic Read
  if ((ret=XU_Get_Cur(xu_unit, xu_selector, xu_size, xu_data)) < 0)
  {
    TestAp_Printf(TESTAP_DBG_ERR,"   ioctl(UVCIOC_CTRL_GET) FAILED (%i)\n",ret);
    if(ret==EINVAL)     TestAp_Printf(TESTAP_DBG_ERR,"    Invalid arguments\n");
    return ret;
  }
  for(i = 0;i < I2CDataLength;i++)
  {
    pData[i] = xu_data[4+i];
  }
  return ret;

}
int XU_I2C_Write(unsigned char slaveID,unsigned int address,long I2CDataLength,unsigned char* pData)
{
  int ret = 0;
  int i = 0;
  __u8 ctrldata[8];

  if(I2CDataLength < 1) return 0;
  if(I2CDataLength > 4) I2CDataLength = 4;

  __u8 xu_unit = 3;
  __u8 xu_selector = XU_SONIX_SYS_I2C_RW;
  __u16 xu_size= 8;
  __u8 *xu_data= ctrldata;

  xu_data[0] = slaveID;
  xu_data[1] = (char)I2CDataLength;
  xu_data[2] = (address & 0xff);
  xu_data[3] = ((address >> 8) & 0xff);
  for(i = 0;i < I2CDataLength;i++)
  {
    xu_data[4+i] = pData[i];
  }

  if ((ret=XU_Set_Cur(xu_unit, xu_selector, xu_size, xu_data)) < 0)
  {
    TestAp_Printf(TESTAP_DBG_ERR,"   ioctl(UVCIOC_CTRL_SET) FAILED (%i) \n",ret);
    if(ret==EINVAL)     TestAp_Printf(TESTAP_DBG_ERR,"    Invalid arguments\n");
    return ret;
  }

  return ret;

}
 */
int SonixCtrls::XU_I2C_ReadEx(unsigned char SlaveID, unsigned int I2C_Addr,
                              unsigned char AddrNum, unsigned char* pData, unsigned char DataNum) {
  // unsigned char byTmp = 0;
  int i = 0;
  unsigned char value = 0x00;

  if(DataNum == 0) {
    return -1;
  }

  if(AddrNum > 2) {
    AddrNum = 2;
  }

  if(DataNum > 2) {
    DataNum = 2;
  }

  unsigned char I2C_Speed = 0;
  XU_Asic_Read(0x10d0, &I2C_Speed);

  // IIC dummy write
  if(0 > XU_I2C_WriteEx(SlaveID, I2C_Addr, AddrNum, 0, 0)) {
    return -1;
  }

  //XU_Asic_Write(0x10d9, 0x01);  // I2C_MODE=1, POOL_SCL=0
  XU_Asic_Write(0x10d9, 0x01);  // modify by Saxen [2006/11/10]
  XU_Asic_Write(0x10d8, 0x01);  // SCL_SEL_OD=0

  if(I2C_Speed & 0x01) {
    XU_Asic_Write(0x10d0, 0x83 | (DataNum << 4));    // I2C_DEV=1, I2C_SEL_RD=1(R)
  } else {
    XU_Asic_Write(0x10d0, 0x82 | (DataNum << 4));    // I2C_DEV=1, I2C_SEL_RD=1(R)
  }

  XU_Asic_Write(0x10d1, SlaveID);
  XU_Asic_Write(0x10d2, 0x00);
  XU_Asic_Write(0x10d3, 0x00);
  XU_Asic_Write(0x10d4, 0x00);
  XU_Asic_Write(0x10d5, 0x00);
  XU_Asic_Write(0x10d6, 0x00);
  XU_Asic_Write(0x10d7, 0x10);  // I2C_RW_TRG=1

  // wait I2C ready (time-out 10ms)
  for(i = 0; (i < 10) && !(value & 0x04); ++i) {
    XU_Asic_Read(0x10d0, &value);
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  if((value & 0x0C) != 0x04) {
    return -1;
  }

  XU_Asic_Read(0x10d2, &value);
  XU_Asic_Read(0x10d3, &value);
  XU_Asic_Read(0x10d4, &value);
  XU_Asic_Read(0x10d5, &value);

  if(DataNum == 2) {
    *pData = (unsigned char)value << 8;
  }

  XU_Asic_Read(0x10d6, &value);

  if(DataNum == 2) {
    *pData |= value;
  } else if(DataNum == 1) {
    *pData = value;
  }

  XU_Asic_Read(0x10d0, &I2C_Speed);
  XU_Asic_Write(0x10d2, 0);
  XU_Asic_Write(0x10d3, 0);
  XU_Asic_Write(0x10d4, 0);
  XU_Asic_Write(0x10d5, 0);
  return 0;
}


int SonixCtrls::XU_I2C_WriteEx(unsigned char SlaveID, unsigned int I2C_Addr,
                               unsigned char AddrNum, unsigned int Data, unsigned char DataNum) {
  // unsigned char byTmp = 0;
  int i = 0;
  unsigned int i2c_data_addr;

  if(AddrNum > 2) {
    AddrNum = 2;
  }

  if(DataNum > 2) {
    DataNum = 2;
  }

  unsigned char I2C_Speed = 0;
  XU_Asic_Read(0x10d0, &I2C_Speed);

  XU_Asic_Write(0x10d9, 0x01);  // I2C_MODE=1, POOL_SCL=0
  XU_Asic_Write(0x10d8, 0x01);  // SCL_SEL_OD=0

  if(I2C_Speed & 0x01) {
    XU_Asic_Write(0x10d0, 0x81 | ((AddrNum + DataNum) <<
                                  4));    //I2C_DEV=1, I2C_SEL_RD=0(W)
  } else {
    XU_Asic_Write(0x10d0, 0x80 | ((AddrNum + DataNum) <<
                                  4));    // I2C_DEV=1, I2C_SEL_RD=0(W)
  }

  XU_Asic_Write(0x10d1, SlaveID);

  i2c_data_addr = 0x10d2;

  // write address
  XU_Asic_Write(0x10d2, (unsigned char)I2C_Addr);

  if(AddrNum > 1) {
    XU_Asic_Write(i2c_data_addr++, (unsigned char)(I2C_Addr >> 8));
    XU_Asic_Write(i2c_data_addr++, (unsigned char)I2C_Addr);
  } else {
    XU_Asic_Write(i2c_data_addr++, (unsigned char)I2C_Addr);
  }

  // write data
  if(DataNum > 1) {
    XU_Asic_Write(i2c_data_addr++, (unsigned char)(Data >> 8));
    XU_Asic_Write(i2c_data_addr++, (unsigned char)Data);
  } else {
    XU_Asic_Write(i2c_data_addr++, (unsigned char)Data);
  }

  // clear remaining buffer
  while(i2c_data_addr < 0x10d7) {
    XU_Asic_Write(i2c_data_addr++, 0x00);
  }

  XU_Asic_Write(0x10d7, 0x10);  // I2C_RW_TRG=1

  // wait I2C ready (time-out 10ms)
  unsigned char value = 0x00;

  for(i = 0; (i < 10) && !(value & 0x04); ++i) {
    XU_Asic_Read(0x10d0, &value);
    //std::this_thread::sleep_for(std::chrono::milliseconds(1));

  }

  //xuchuanxin add 2017/09/27
  XU_Asic_Write(0x10d0, I2C_Speed);
  XU_Asic_Write(0x10d2, 0);
  XU_Asic_Write(0x10d3, 0);
  XU_Asic_Write(0x10d4, 0);
  XU_Asic_Write(0x10d5, 0);

  if((value & 0x0c) == 0x04) {
    return 0;
  } else {
    return -1;
  }
}

int SonixCtrls::XU_SF_Read(unsigned int Addr, unsigned char* pData,
                           unsigned int Length) {
#define DEF_SF_RW_LENGTH 8
#define min(a,b) a<b?a:b

  if(0 == Length) {
    return -1;
  }

  uvc_error_t ret;
  unsigned int ValidLength = 0, loop = 0, remain = 0;
  unsigned char* pCopy = pData;

  uvc_device_handle_t* devh = (uvc_device_handle_t*)m_fd;

  ValidLength = min(0x10000 - (Addr & 0xffff), Length);

  loop = ValidLength / DEF_SF_RW_LENGTH;
  remain = ValidLength % DEF_SF_RW_LENGTH;

  //get sf data
  for(unsigned int i = 0; i < loop; i++) {
    const unsigned int secAddr = Addr + i * DEF_SF_RW_LENGTH;
    ret = uvc_get_xu_flash(devh, secAddr, DEF_SF_RW_LENGTH, pCopy);

    if(UVC_SUCCESS != ret) {
      return -1;
    }

    pCopy += DEF_SF_RW_LENGTH;
  }

  if(remain) {
    ret = uvc_get_xu_flash(devh, (Addr + loop * DEF_SF_RW_LENGTH), remain, pCopy);

    if(UVC_SUCCESS != ret) {
      return -1;
    }
  }

  return 0;

}




//Write Flash data


void SonixCtrls::SF_CMDread_Status() {
  // shawn 2009/02/02 modify +++++
  unsigned char data[1] = {0};
  long ldata;

  for(int i = 0; i < 10000; i++) {
    ldata = 0x0;
    XU_Asic_Write(0x1091, ldata);
    ldata = 0x05;
    XU_Asic_Write(0x1082, ldata);
    ldata = 0x01;
    XU_Asic_Write(0x1081, ldata);
    bSF_WaitRdy();

    ldata = 0x0;
    XU_Asic_Write(0x1083, ldata);
    ldata = 0x02;
    XU_Asic_Write(0x1081, ldata);
    bSF_WaitRdy();
    XU_Asic_Read(0x1083, &data[0]);

    ldata = 0x01;

    if((data[0] & 0x01) != 0x01) {
      XU_Asic_Write(0x1091, ldata);
      break;
    }

    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // shawn 2009/02/02 modify -----
}

void SonixCtrls::DisableWriteProtect(unsigned char bWriteProtect) {
  int bIsNewWPVer = false;
  // unsigned char bWriteProtectGPIO = 0;
  // unsigned int WPAddr=0xFFFF;

  // bWriteProtectGPIO = (bWriteProtect>>4)&0x7;  // shawn 2008/10/15 modify to fix influence of bSF_BP

  if((bWriteProtect & 0x0C) == 0x08) {
    bIsNewWPVer = true;
  }

  //printf("is new ver %d \n", bIsNewWPVer);
  if(bIsNewWPVer) {
    if((bWriteProtect & 0x03) == 0x02) {
      bWriteProtect = 1;
    } else if((bWriteProtect & 0x03) == 0x03) {
      bWriteProtect = 2;
    }
  } else {
    bWriteProtect = bWriteProtect & 0x03;
  }

  //printf("bWriteProtect %d \n", bWriteProtect);
  XU_Asic_Write(0x1080, 0x1);
  XU_Asic_Write(0x1091, 0x0);

  XU_Asic_Write(0x1082, 0x06);
  XU_Asic_Write(0x1081, 0x1);
  bSF_WaitRdy();
  //SF_WaitReady(iDevIndex);
  XU_Asic_Write(0x1091, 0x1);
  SF_CMDread_Status();

  XU_Asic_Write(0x1091, 0x0);
  XU_Asic_Write(0x1082, 0x1);
  XU_Asic_Write(0x1081, 0x1);
  //SF_WaitReady(iDevIndex);
  bSF_WaitRdy();
  XU_Asic_Write(0x1082, 0x0);
  XU_Asic_Write(0x1081, 0x1);
  bSF_WaitRdy();
  //SF_WaitReady(iDevIndex);
  XU_Asic_Write(0x1091, 0x1);
  SF_CMDread_Status();

  XU_Asic_Write(0x1080, 0x0);
}


void SonixCtrls::DisableAddr(unsigned short Addr,  unsigned char Addr_bit) {
  unsigned char bufs[1] = {0};
  unsigned char bufd[1] = {0};
  XU_Asic_Read(Addr, bufs);

  switch(Addr_bit) {
  case 0:
    bufd[0] = bufs[0] & 0xfe;
    break;

  case 1:
    bufd[0] = bufs[0] & 0xfd;
    break;

  case 2:
    bufd[0] = bufs[0] & 0xfb;
    break;

  case 3:
    bufd[0] = bufs[0] & 0xf7;
    break;

  case 4:
    bufd[0] = bufs[0] & 0xef;
    break;

  case 5:
    bufd[0] = bufs[0] & 0xdf;
    break;

  case 6:
    bufd[0] = bufs[0] & 0xbf;
    break;

  case 7:
    bufd[0] = bufs[0] & 0x7f;
    break;

  default:
    break;
  }

  XU_Asic_Write(Addr, bufd[0]);
}



void SonixCtrls::EnableAddr(unsigned short Addr,  unsigned char Addr_bit) {
  unsigned char bufs[1] = {0};
  unsigned char bufd[1] = {0};
  XU_Asic_Read(Addr, bufs);

  switch(Addr_bit) {
  case 0:
    bufd[0] = bufs[0] | 0x01;
    break;

  case 1:
    bufd[0] = bufs[0] | 0x02;
    break;

  case 2:
    bufd[0] = bufs[0] | 0x04;
    break;

  case 3:
    bufd[0] = bufs[0] | 0x08;
    break;

  case 4:
    bufd[0] = bufs[0] | 0x10;
    break;

  case 5:
    bufd[0] = bufs[0] | 0x20;
    break;

  case 6:
    bufd[0] = bufs[0] | 0x40;
    break;

  case 7:
    bufd[0] = bufs[0] | 0x80;
    break;

  default:
    break;
  }

  XU_Asic_Write(Addr, bufd[0]);
}

int SonixCtrls::PageErase(long lStartAddr) {
  char ldata;

  ldata = 0x1;
  XU_Asic_Write(0x1080, ldata);
  //SF_Set_WEL_Bit
  ldata = 0x0;
  XU_Asic_Write(0x1091, ldata);
  ldata = 0x06;
  XU_Asic_Write(0x1082, ldata); //WEN
  ldata =  0x01;
  XU_Asic_Write(0x1081, ldata);
  ldata = 0x1;
  XU_Asic_Write(0x1091, ldata);
  bSF_WaitRdy();
  SF_CMDread_Status();

  //sector erase
  ldata = 0x0;
  XU_Asic_Write(0x1091, ldata);
  ldata = 0x81;
  XU_Asic_Write(0x1082, ldata); // for chip page erase
  //SetRegData(0x1082,0x20);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  //  SF_CMDread_Status();

  ldata = lStartAddr >> 16;
  XU_Asic_Write(0x1082, ldata);
  //SetRegData(0x1082,0x00);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  ldata = (lStartAddr & 0xffff) >> 8;
  XU_Asic_Write(0x1082, ldata);
  //SetRegData(0x1082,SectorNum);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  ldata = (unsigned char)lStartAddr;//ldata = 0x00;
  XU_Asic_Write(0x1082, ldata);
  //SetRegData(0x1082,0x00);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  ldata = 0x1;
  XU_Asic_Write(0x1091, ldata);
  //SF_CMDread_Status
  bSF_WaitRdy();//SF_CMDread_Status(iDevIndex);
  SF_CMDread_Status();

  ldata = 0x0;
  int bRet = XU_Asic_Write(0x1080, ldata);

  return (bRet < 0) ? 0 : 1;
}

int SonixCtrls::SectorErase(long lStartAddr) {
  char ldata;

  ldata = 0x1;
  XU_Asic_Write(0x1080, ldata);
  //SF_Set_WEL_Bit
  ldata = 0x0;
  XU_Asic_Write(0x1091, ldata);
  ldata = 0x06;
  XU_Asic_Write(0x1082, ldata); //WEN
  ldata =  0x01;
  XU_Asic_Write(0x1081, ldata);
  ldata = 0x1;
  XU_Asic_Write(0x1091, ldata);
  bSF_WaitRdy();
  SF_CMDread_Status();

  //sector erase
  ldata = 0x0;
  XU_Asic_Write(0x1091, ldata);
  ldata = 0x20;
  XU_Asic_Write(0x1082, ldata); // for chip sector erase
  //SetRegData(0x1082,0x20);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  //  SF_CMDread_Status();

  ldata = lStartAddr >> 16;
  XU_Asic_Write(0x1082, ldata);
  //SetRegData(0x1082,0x00);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  ldata = (lStartAddr & 0xffff) >> 8;
  XU_Asic_Write(0x1082, ldata);
  //SetRegData(0x1082,SectorNum);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  ldata = (unsigned char)lStartAddr;//ldata = 0x00;
  XU_Asic_Write(0x1082, ldata);
  //SetRegData(0x1082,0x00);
  ldata = 0x01;
  XU_Asic_Write(0x1081, ldata);
  bSF_WaitRdy();
  ldata = 0x1;
  XU_Asic_Write(0x1091, ldata);
  //SF_CMDread_Status
  bSF_WaitRdy();//SF_CMDread_Status(iDevIndex);
  SF_CMDread_Status();

  ldata = 0x0;
  int bRet = XU_Asic_Write(0x1080, ldata);

  return (bRet < 0) ? 0 : 1;
}

//laddr need to fit 0x0XXX00h
int SonixCtrls::XU_SF_Write(long laddr, unsigned char uData[], int iLen) {
  if(0 == iLen) {
    return -1;
  }

  DisableWriteProtect(0);

  //printf("Disable Write Protect end\n");
  if(iLen > 256) {
    SectorErase(laddr);
  } else {
    PageErase(laddr);
  }

  SF_CMDread_Status();

  //printf("Sector Erase end\n");

  /////////////////
  EnableAddr(0x1080, 0);        // Flash Mode:En

  //WREN cycle
  DisableAddr(0x1091, 0);       // CS:0
  long ldata = 6;
  XU_Asic_Write(0x1082, ldata); //WREN
  EnableAddr(0x1081, 0);
  bSF_WaitRdy();
  EnableAddr(0x1091, 0);        // CS:1

  int j = 0;

  do {
    DisableAddr(0x1091, 0);   // CS:0
    ldata = 2;      //PP  [0xaf:SST 0x2:other]
    XU_Asic_Write(0x1082, ldata); //Page Program
#ifdef USE_CYCLOPS_FLASH
    EnableAddr(0x1081, 0);
    bSF_WaitRdy();
#endif // USE_CYCLOPS_FLASH

    ldata = laddr >> 16;    //addr1
    XU_Asic_Write(0x1082, ldata);
#ifdef USE_CYCLOPS_FLASH
    EnableAddr(0x1081, 0);
    bSF_WaitRdy();
#endif // USE_CYCLOPS_FLASH

    ldata = (laddr & 0xffff) >> 8;
    XU_Asic_Write(0x1082, ldata); //addr2
#ifdef USE_CYCLOPS_FLASH
    EnableAddr(0x1081, 0);
    bSF_WaitRdy();
#endif // USE_CYCLOPS_FLASH

    ldata = (unsigned char)laddr;
    XU_Asic_Write(0x1082, ldata); //addr3
#ifdef USE_CYCLOPS_FLASH
    EnableAddr(0x1081, 0);
    bSF_WaitRdy();
#endif  // USE_CYCLOPS_FLASH

    for(; j < iLen;) {
      //printf("j %d iLen %d\n", j, iLen);
      //      printf("%02X ",uData[j]);
      XU_Asic_Write(0x1082, uData[j]);  //Data
#ifdef USE_CYCLOPS_FLASH
      EnableAddr(0x1081, 0);
      bSF_WaitRdy();
#endif
      laddr++;
      j++;

      if(0 == laddr % 256) {
        EnableAddr(0x1091, 0);        // CS:1
        break;
      }
    }

    if(j == iLen) {
      printf("write flash done\n");
      EnableAddr(0x1091, 0);
      break;
    } else {
      //WREN cycle
      DisableAddr(0x1091, 0);     // CS:0
      long tldata = 6;
      XU_Asic_Write(0x1082, tldata);  //WREN
#ifdef USE_CYCLOPS_FLASH
      EnableAddr(0x1081, 0);
      bSF_WaitRdy();
#endif
      EnableAddr(0x1091, 0);        // CS:1
    }
    printf("j %d iLen %d\n", j, iLen);  // Add by yuliang 20200716
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //usleep(10*000);  // Add by yuliang 20200716
  } while(1);

  DisableAddr(0x1080, 0);
  bSF_WaitRdy();
  return 0;
}

int SonixCtrls::ReadAsicBit(long Addr,  unsigned char Addr_bit) {
  unsigned char bufs[1] = {0};;
  unsigned char biData = 0;
  //XU_Asic_Read(Addr,bufs);
  XU_Asic_Read(Addr, bufs);

  switch(Addr_bit) {
  case 0:
    biData = bufs[0] & 0x01;
    break;

  case 1:
    biData = bufs[0] & 0x02;
    break;

  case 2:
    biData = bufs[0] & 0x04;
    break;

  case 3:
    biData = bufs[0] & 0x08;
    break;

  case 4:
    biData = bufs[0] & 0x10;
    break;

  case 5:
    biData = bufs[0] & 0x20;
    break;

  case 6:
    biData = bufs[0] & 0x40;
    break;

  case 7:
    biData = bufs[0] & 0x80;
    break;

  default:
    break;
  }

  return biData;
}

void SonixCtrls::bSF_WaitRdy() {
  // long ldata=0;
  for(int i = 0; i < 50; i++) {     //pooling ready; 500us timeout
    if(ReadAsicBit(0x1084, 0)) {            //aISP_RDY
      return;
    }

    //std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return;
}

//CameraSN
CameraSN::CameraSN(void* fd)
  : m_fd(fd) {
}

bool CameraSN::GetSN(std::string* strSn) {
  if(0  == strSn) {
    return false;
  }

  strSn->clear();
  unsigned char cSn[SN_LEN + 1];
  memset(cSn, 0, sizeof(cSn));

  SonixCtrls camXu(m_fd);
  int nRes = camXu.XU_SF_Read(0x1f000, cSn, SN_LEN);

  if(0 > nRes) {
    return false;
  }

  *strSn = (char*)cSn;

  return true;
}


//CameraLED
CaptureLED::CaptureLED(void* fd)
  : m_fd(fd)
  , regLED(0x00) {
  memset(m_dValue, 0, sizeof(m_dValue));
}

CaptureLED::~CaptureLED() {
  if(NULL == m_fd) {
    return;
  }

  SonixCtrls cam(m_fd);
  cam.XU_I2C_WriteEx(0x30, 0x04, 1, 0x00, 1);
  regLED = 0x00;
}

void CaptureLED::Init() {
  if(NULL == m_fd) {
    return;
  }

  regLED = 0x01;
  SonixCtrls cam(m_fd);
  cam.XU_I2C_WriteEx(0x30, 0x00, 1, 0x78, 1);
  cam.XU_I2C_WriteEx(0x30, 0x01, 1, 0x80, 1); //linear Up/Down
  cam.XU_I2C_WriteEx(0x30, 0x02, 1, 0x00, 1);
  cam.XU_I2C_WriteEx(0x30, 0x03, 1, 0x00, 1);
  cam.XU_I2C_WriteEx(0x30, 0x05, 1, 0x00, 1);
  cam.XU_I2C_WriteEx(0x30, 0x0A, 1, 0x01, 1); //Alway on
  cam.XU_I2C_WriteEx(0x30, 0x06, 1, 0x47, 1);  //13mA
  cam.XU_I2C_WriteEx(0x30, 0x07, 1, 0x00, 1);
  cam.XU_I2C_WriteEx(0x30, 0x08, 1, 0x00, 1);
  cam.XU_I2C_WriteEx(0x30, 0x04, 1, regLED, 1);

  m_dValue[0] = 0.3;
}

//nChannel 0~2
//dValue 0.0~1.0 //0~3mA
bool CaptureLED::SetValue(const int nChannel, const double dValue) {
  if((0 > nChannel) || (2 < nChannel)) {
    return false;
  }

  if(NULL == m_fd) {
    return false;
  }


  double dSetValue = dValue < 0 ? 0 : dValue;
  //  unsigned char cValue = (unsigned char)((0x4F+1)*(dSetValue > 1?1:dSetValue));   //10mA //Modify by Murphy 2018-09-10
  unsigned char cValue = (unsigned char)((0x9F + 1) * (dSetValue > 1 ? 1 :
                                         dSetValue));   //20mA //Add by Murphy 2018-09-10

  SonixCtrls cam(m_fd);
  m_dValue[nChannel] = dValue;

  //B version
  unsigned char Date = 0x00;
  cam.XU_Asic_Read(0x1006, &Date);

  if(dValue == 0) {
    cam.XU_Asic_Write(0x1006, Date & 0xfe);
  } else {
    cam.XU_Asic_Write(0x1006, Date | 0x01);
  }

  //D version

  //Close
  if(0 == cValue) {
    regLED = regLED & (~(1 << nChannel * 2));
    return (cam.XU_I2C_WriteEx(0x30, 0x04, 1, regLED, 1) < 0) ? false : true;
  } else {
    regLED = regLED | (1 << nChannel * 2);
    int nRes = cam.XU_I2C_WriteEx(0x30, 0x06 + nChannel, 1, cValue - 1, 1);

    nRes = cam.XU_I2C_WriteEx(0x30, 0x04, 1, regLED, 1);
    return (nRes < 0) ? false : true;
  }
}

double CaptureLED::GetValue(const int nChannel) const {
  return m_dValue[nChannel];
}

CaptureFlash::CaptureFlash(void* fd)
  : m_fd(fd) {

}

int CaptureFlash::Write(const std::string& str) {
  SonixCtrls capCtrls(m_fd);
  return capCtrls.XU_SF_Write(0x20000, (unsigned char*)str.c_str(),
                              str.length() + 1);
}

int CaptureFlash::Read(std::string& str, int len) {
  const int nPage = len / 256;
  const int remain = len % 256;

  char* data = new char[(nPage + 1) << 8];

  SonixCtrls capCtrls(m_fd);
  int err;

  memset(data, '\0', (nPage + 1) << 8);

  for(int i = 0; i < nPage + 1; ++i) {
    unsigned char uData[256];
    memset(uData, '\0', 256);

    if(nPage == i) {
      if(0 != remain) {
        err = capCtrls.XU_SF_Read(0x20000 + (i << 8), uData, remain);
      }
    } else {
      err = capCtrls.XU_SF_Read(0x20000 + (i << 8), uData, 256);
    }

    if(0 != err) {
      str.clear();
      return -1;
    }

    //Empty page
    if('\0' == uData[0] || 0xff == uData[0]) {
      (data + (i << 8))[0] = '\0';
      str = data;
      delete[] data;
      return 0;
    }

    memcpy(data + (i << 8), uData, 256);

    //Read to end
    if('\0' == uData[255] || 0xff == uData[255]) {
      (data + (i << 8))[255] = '\0';
      break;
    }
  }

  str = data;
  str.shrink_to_fit();
  delete[] data;
  return 0;
}
