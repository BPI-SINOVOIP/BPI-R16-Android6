/*
**********************************************************************************************************************
*
*						           the Embedded Secure Bootloader System
*
*
*						       Copyright(C), 2006-2014, Allwinnertech Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      :
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/

#ifndef __EFUSE_H__
#define __EFUSE_H__

#include "asm/arch/cpu.h"

#define SID_PRCTL				(SUNXI_SID_BASE + 0x40)
#define SID_PRKEY				(SUNXI_SID_BASE + 0x50)
#define SID_RDKEY				(SUNXI_SID_BASE + 0x60)
#define SID_PMER				(SUNXI_SID_BASE + 0x60)
#define SJTAG_AT0				(SUNXI_SID_BASE + 0x80)
#define SJTAG_AT1				(SUNXI_SID_BASE + 0x84)
#define SJTAG_S					(SUNXI_SID_BASE + 0x88)
#define SID_RF(n)               (SUNXI_SID_BASE + (n) * 4 + 0xa0)

#define SID_EFUSE               (SUNXI_SID_BASE + 0x200)


#define EFUSE_CHIPD             (0x00)
#define EFUSE_OEM_PROGRAM       (0x10)
#define EFUSE_NV1               (0x14)
#define EFUSE_NV2               (0x18)
#define EFUSE_THERMAL_SENSOR    (0x34)

#define EFUSE_HUK               (0x44)
#define EFUSE_ROTPK             (0x64)
#define EFUSE_SSK               (0x84)
#define EFUSE_RSSK              (0x94)

#define EFUSE_HDCP_HASH         (0xB4)
#define EFUSE_EK_HASH           (0xC4)
#define EFUSE_RESERVED          (0xD4)

#define EFUSE_LCJS              (0xF4)
#define EFUSE_DEBUG             (0xF8)
#define EFUSE_CHIPCONFIG        (0xFC)


extern void sid_set_security_mode(void);
extern void sid_read(void);

#endif    /*  #ifndef __EFUSE_H__  */
