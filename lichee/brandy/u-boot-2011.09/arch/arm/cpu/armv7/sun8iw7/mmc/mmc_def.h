/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Description: MMC driver for general mmc operations
 * Author: Aaron <leafy.myeh@allwinnertech.com>
 * Date: 2012-2-3 14:18:18
 */

#ifndef _MMC_DEF_H_
#define _MMC_DEF_H_

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/ccmu.h>

#define MAX_MMC_NUM			4
#define MMC_TRANS_BY_DMA
//#define MMC_DEBUG

#define MMC_REG_FIFO_OS		(0x200)

#define MMC_REG_BASE		SUNXI_MMC0_BASE
#define CCMU_HCLKGATE0_BASE CCM_AHB1_GATE0_CTRL
#define CCMU_HCLKRST0_BASE 	CCM_AHB1_RST_REG0
#define CCMU_MMC0_CLK_BASE 	CCM_SDC0_SCLK_CTRL
#define CCMU_MMC2_CLK_BASE 	CCM_SDC2_SCLK_CTRL


//#define CCMU_PLL5_CLK_BASE 	0x01c20020
//#define __be32_to_cpu(x)	((0x000000ff&((x)>>24)) | (0x0000ff00&((x)>>8)) |
//							 (0x00ff0000&((x)<< 8)) | (0xff000000&((x)<<24)))

#ifndef NULL
#define NULL (void*)0
#endif

#ifdef MMC_DEBUG
#define mmcinfo(fmt...)	printf("[mmc]: "fmt)
#define mmcdbg(fmt...)	printf("[mmc]: "fmt)
#define mmcmsg(fmt...)	printf(fmt)
#else
#define mmcinfo(fmt...)	printf("[mmc]: "fmt)
#define mmcdbg(fmt...)
#define mmcmsg(fmt...)
#endif

//#define readb(addr)		(*((volatile unsigned char  *)(addr)))
//#define readw(addr)		(*((volatile unsigned short *)(addr)))
//#define readl(addr)		(*((volatile unsigned long  *)(addr)))
//#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
//#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
//#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))


#define DMAC_DES_BASE_IN_SRAM		(0x20000 + 0xC000)
#define DMAC_DES_BASE_IN_SDRAM		(0x42000000)
#define DRAM_START_ADDR				(0x40000000)


#define DRIVER_VER  "2014-11-20 9:55:00"

#endif /* _MMC_H_ */
