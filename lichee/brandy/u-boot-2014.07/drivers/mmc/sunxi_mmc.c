/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * MMC driver for allwinner sunxi platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/platform.h>
#include <asm/arch/clock.h>
#include <asm/arch/ccmu.h>
#include <asm/arch/mmc.h>
#include <asm/arch/timer.h>
#include <malloc.h>
#include <mmc.h>
#include <sys_config.h>

#include "mmc_def.h"

#define FPGA_PLATFORM

#undef readl
#define  readl(a)   *(volatile uint *)(ulong)(a)

#undef writel
#define  writel(v, c) *(volatile uint *)(ulong)(c) = (v)

DECLARE_GLOBAL_DATA_PTR;
//#define SUNXI_MMCDBG
//#undef SUNXI_MMCDBG
//#define MMCINFO(fmt...)	printf("[mmc]: "fmt)
#ifndef CONFIG_ARCH_SUN7I
#define MMC_REG_FIFO_OS		(0x200)
#else
#define MMC_REG_FIFO_OS		(0x100)
#endif

#ifdef SUNXI_MMCDBG
//#define MMCDBG(fmt...)	printf("[mmc]: "fmt)

static void dumphex32(char* name, char* base, int len)
{
	__u32 i;

	MMCMSG("dump %s registers:", name);
	for (i=0; i<len; i+=4) {
		if (!(i&0xf))
			MMCMSG("\n0x%p : ", base + i);
		MMCMSG("0x%08x ", readl((ulong)base + i));
	}
	MMCMSG("\n");
}

/*
static void dumpmmcreg(struct sunxi_mmc *reg)
{
	printf("dump mmc registers:\n");
	printf("gctrl     0x%08x\n", reg->gctrl     );
	printf("clkcr     0x%08x\n", reg->clkcr     );
	printf("timeout   0x%08x\n", reg->timeout   );
	printf("width     0x%08x\n", reg->width     );
	printf("blksz     0x%08x\n", reg->blksz     );
	printf("bytecnt   0x%08x\n", reg->bytecnt   );
	printf("cmd       0x%08x\n", reg->cmd       );
	printf("arg       0x%08x\n", reg->arg       );
	printf("resp0     0x%08x\n", reg->resp0     );
	printf("resp1     0x%08x\n", reg->resp1     );
	printf("resp2     0x%08x\n", reg->resp2     );
	printf("resp3     0x%08x\n", reg->resp3     );
	printf("imask     0x%08x\n", reg->imask     );
	printf("mint      0x%08x\n", reg->mint      );
	printf("rint      0x%08x\n", reg->rint      );
	printf("status    0x%08x\n", reg->status    );
	printf("ftrglevel 0x%08x\n", reg->ftrglevel );
	printf("funcsel   0x%08x\n", reg->funcsel   );
	printf("dmac      0x%08x\n", reg->dmac      );
	printf("dlba      0x%08x\n", reg->dlba      );
	printf("idst      0x%08x\n", reg->idst      );
	printf("idie      0x%08x\n", reg->idie      );
	printf("cbcr      0x%08x\n", reg->cbcr      );
	printf("bbcr      0x%08x\n", reg->bbcr      );
}
*/
#else
//#define MMCDBG(fmt...)
#define dumpmmcreg(fmt...)
#define  dumphex32(fmt...)
#endif /* SUNXI_MMCDBG */

#define BIT(x)				(1<<(x))
/* Struct for Intrrrupt Information */
#define SDXC_RespErr		BIT(1) //0x2
#define SDXC_CmdDone		BIT(2) //0x4
#define SDXC_DataOver		BIT(3) //0x8
#define SDXC_TxDataReq		BIT(4) //0x10
#define SDXC_RxDataReq		BIT(5) //0x20
#define SDXC_RespCRCErr		BIT(6) //0x40
#define SDXC_DataCRCErr		BIT(7) //0x80
#define SDXC_RespTimeout	BIT(8) //0x100
#define SDXC_ACKRcv			BIT(8)	//0x100
#define SDXC_DataTimeout	BIT(9)	//0x200
#define SDXC_BootStart		BIT(9)	//0x200
#define SDXC_DataStarve		BIT(10) //0x400
#define SDXC_VolChgDone		BIT(10) //0x400
#define SDXC_FIFORunErr		BIT(11) //0x800
#define SDXC_HardWLocked	BIT(12)	//0x1000
#define SDXC_StartBitErr		BIT(13)	//0x2000
#define SDXC_AutoCMDDone	BIT(14)	//0x4000
#define SDXC_EndBitErr		BIT(15)	//0x8000
#define SDXC_SDIOInt		BIT(16)	//0x10000
#define SDXC_CardInsert		BIT(30) //0x40000000
#define SDXC_CardRemove	BIT(31) //0x80000000
#define SDXC_IntErrBit		(SDXC_RespErr | SDXC_RespCRCErr | SDXC_DataCRCErr \
				| SDXC_RespTimeout | SDXC_DataTimeout | SDXC_FIFORunErr \
				| SDXC_HardWLocked | SDXC_StartBitErr | SDXC_EndBitErr)  //0xbfc2

/* IDMA status bit field */
#define SDXC_IDMACTransmitInt	BIT(0)
#define SDXC_IDMACReceiveInt	BIT(1)
#define SDXC_IDMACFatalBusErr	BIT(2)
#define SDXC_IDMACDesInvalid	BIT(4)
#define SDXC_IDMACCardErrSum	BIT(5)
#define SDXC_IDMACNormalIntSum	BIT(8)
#define SDXC_IDMACAbnormalIntSum BIT(9)
#define SDXC_IDMACHostAbtInTx	BIT(10)
#define SDXC_IDMACHostAbtInRx	BIT(10)
#define SDXC_IDMACIdle		(0U << 13)
#define SDXC_IDMACSuspend	(1U << 13)
#define SDXC_IDMACDESCRd	(2U << 13)
#define SDXC_IDMACDESCCheck	(3U << 13)
#define SDXC_IDMACRdReqWait	(4U << 13)
#define SDXC_IDMACWrReqWait	(5U << 13)
#define SDXC_IDMACRd		(6U << 13)
#define SDXC_IDMACWr		(7U << 13)
#define SDXC_IDMACDESCClose	(8U << 13)

/* delay control */
#define SDXC_StartCal        (1<<15)
#define SDXC_CalDone         (1<<14)
#define SDXC_CalDly          (0x3F<<8)
#define SDXC_EnableDly       (1<<7)
#define SDXC_CfgDly          (0x3F<<0)


#define MMC_CLK_400K					0
#define MMC_CLK_25M						1
#define MMC_CLK_50M						2
#define MMC_CLK_50MDDR				3
#define MMC_CLK_50MDDR_8BIT		4
#define MMC_CLK_100M					5
#define MMC_CLK_200M					6
#define MMC_CLK_MOD_NUM				7


#define SUNXI_MMC_TIMING_MODE_1 1U
#define SUNXI_MMC_TIMING_MODE_3 3U
#define SUNXI_MMC_TIMING_MODE_4 4U



/* smhc0&1 */
struct sunxi_mmc_clk_dly_mode1 {
	u32 freq;
	u32 oclk_dly;
	u32 sclk_dly;
};

struct sunxi_mmc_clk_dly_mode3 {
	u32 dly_cal_done;
	u32 sdly_unit_ps;
	u32 oclk_dly;
	u32 sclk_dly;
};

/* smhc2 */
struct sunxi_mmc_clk_dly_mode4 {
	u32 dly_cal_done;
	u32 sdly_unit_ps;
	u32 ddly_unit_ps;
	u32 oclk_dly;
	u32 sclk_dly;
	u32 ds_dly;
};

struct sunxi_mmc_des {
	u32			:1,
		dic		:1, /* disable interrupt on completion */
		last_des	:1, /* 1-this data buffer is the last buffer */
		first_des	:1, /* 1-data buffer is the first buffer,
						   0-data buffer contained in the next descriptor is 1st buffer */
		des_chain	:1, /* 1-the 2nd address in the descriptor is the next descriptor address */
		end_of_ring	:1, /* 1-last descriptor flag when using dual data buffer in descriptor */
				:24,
		card_err_sum	:1, /* transfer error flag */
		own		:1; /* des owner:1-idma owns it, 0-host owns it */

#define SDXC_DES_NUM_SHIFT 15
#define SDXC_DES_BUFFER_MAX_LEN	(1 << SDXC_DES_NUM_SHIFT)
	u32	data_buf1_sz	:16,
		data_buf2_sz	:16;

	u32	buf_addr_ptr1;
	u32	buf_addr_ptr2;
};

struct sunxi_mmc_host {
	u32 mmc_no;

	u32 hclkbase;
	u32 hclkrst;
	u32 mclkbase;
	u32 database;

	u32 fatal_err;
	u32	raw_int_bak;
	u32 mod_clk;
	struct sunxi_mmc *reg;
	struct sunxi_mmc_des* pdes;

	/*sample delay and output deley setting*/
	u32 timing_mode;
	struct sunxi_mmc_clk_dly_mode1 mmc_clk_dly_mode1[MMC_CLK_MOD_NUM];
	struct sunxi_mmc_clk_dly_mode3 mmc_clk_dly_mode3;
	struct sunxi_mmc_clk_dly_mode4 mmc_clk_dly_mode4;

	struct mmc *mmc;
	struct mmc_config cfg;
};

/* support 4 mmc hosts */
//struct mmc mmc_dev[4];
struct sunxi_mmc_host mmc_host[4];

void mmc_dump_errinfo(struct sunxi_mmc_host* smc_host, struct mmc_cmd *cmd)
{
	MMCINFO("smc %d err, cmd %d, %s%s%s%s%s%s%s%s%s%s\n",
		smc_host->mmc_no, cmd? cmd->cmdidx: -1,
		smc_host->raw_int_bak & SDXC_RespErr     ? " RE"     : "",
		smc_host->raw_int_bak & SDXC_RespCRCErr  ? " RCE"    : "",
		smc_host->raw_int_bak & SDXC_DataCRCErr  ? " DCE"    : "",
		smc_host->raw_int_bak & SDXC_RespTimeout ? " RTO"    : "",
		smc_host->raw_int_bak & SDXC_DataTimeout ? " DTO"    : "",
		smc_host->raw_int_bak & SDXC_DataStarve  ? " DS"     : "",
		smc_host->raw_int_bak & SDXC_FIFORunErr  ? " FE"     : "",
		smc_host->raw_int_bak & SDXC_HardWLocked ? " HL"     : "",
		smc_host->raw_int_bak & SDXC_StartBitErr ? " SBE"    : "",
		smc_host->raw_int_bak & SDXC_EndBitErr   ? " EBE"    : "",
		smc_host->raw_int_bak ==0  ? " STO"    : ""
		);
}

static int mmc_resource_init(int sdc_no)
{
	struct sunxi_mmc_host* mmchost = &mmc_host[sdc_no];
	MMCDBG("init mmc %d resource\n", sdc_no);
	switch (sdc_no) {
		case 0:
			mmchost->reg = (struct sunxi_mmc *)SUNXI_SMHC0_BASE;
			mmchost->database = SUNXI_SMHC0_BASE + MMC_REG_FIFO_OS;
			mmchost->mclkbase = CCMU_SDMMC0_CLK_REG;
			break;
		case 1:
			mmchost->reg = (struct sunxi_mmc *)SUNXI_SMHC1_BASE;
			mmchost->database = SUNXI_SMHC1_BASE + MMC_REG_FIFO_OS;
			mmchost->mclkbase = CCMU_SDMMC1_CLK_REG;
			break;
		case 2:
			mmchost->reg = (struct sunxi_mmc *)SUNXI_SMHC2_BASE;
			mmchost->database = SUNXI_SMHC2_BASE + MMC_REG_FIFO_OS;
			mmchost->mclkbase = CCMU_SDMMC2_CLK_REG;
			break;
		default:
			MMCINFO("Wrong mmc number %d\n", sdc_no);
			break;
	}

	mmchost->hclkbase = CCMU_BUS_CLK_GATING_REG0;
	mmchost->hclkrst  = CCMU_BUS_SOFT_RST_REG0;

	mmchost->mmc_no = sdc_no;

	return 0;
}

static void get_fex_para(int sdc_no)
{
	return ;
}

static int mmc_clk_io_onoff(int sdc_no, int onoff)
{
	int rval;
	struct sunxi_mmc_host* mmchost = &mmc_host[sdc_no];

	/* config ahb clock */
	if(onoff)
	{
		rval = readl(mmchost->hclkrst);
		rval |= (1 << (8 + sdc_no));
		writel(rval, mmchost->hclkrst);
		rval = readl(mmchost->hclkbase);
		rval |= (1 << (8 + sdc_no));
		writel(rval, mmchost->hclkbase);
	}
	else
	{
		rval = readl(mmchost->hclkbase);
		rval &= ~(1 << (8 + sdc_no));
		writel(rval, mmchost->hclkbase);

		rval = readl(mmchost->hclkrst);
		rval &= ~ (1 << (8 + sdc_no));
		writel(rval, mmchost->hclkrst);
	}

	/* config mod clock */
	writel(0x80000000, mmchost->mclkbase);
	mmchost->mod_clk = 24000000;
	//dumphex32("ccmu", (char*)SUNXI_CCM_BASE, 0x100);
	//dumphex32("gpio", (char*)SUNXI_PIO_BASE, 0x100);
	//dumphex32("mmc", (char*)mmchost->reg, 0x100);

	return 0;
}

static int mmc_update_clk(struct sunxi_mmc_host* mmchost)
{
	unsigned int cmd;
	unsigned timeout = 1000;

	cmd = (1U << 31) | (1 << 21) | (1 << 13);
  	writel(cmd, &mmchost->reg->cmd);
	while((readl(&mmchost->reg->cmd)&0x80000000) && --timeout){
		__msdelay(1);
	}
	if (!timeout){
		MMCINFO("mmc %d,update clk failed\n",mmchost->mmc_no);
		dumphex32("mmc", (char*)mmchost->reg, 0x100);
		return -1;
	}

	writel(readl(&mmchost->reg->rint), &mmchost->reg->rint);
	return 0;
}

#ifndef FPGA_PLATFORM
static int mmc_set_mclk(struct sunxi_mmc_host* mmchost, u32 clk_hz)
{
	unsigned n, m, div, src, sclk_hz = 0;
	unsigned rval;

	if (clk_hz <= 40000) {
		src = 0;
		sclk_hz = 24000000;
	} else {
		src = 1;
		sclk_hz = sunxi_clock_get_pll6()*1000000;
	}

	div = (2 * sclk_hz + clk_hz) / (2 * clk_hz);
	div = (div==0) ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		MMCINFO("%s: source clock is too high!!!\n", __FUNCTION__);
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	//rval = (1U << 31) | (src << 24) | (n << 16) | (m - 1);
	rval = (src << 24) | (n << 16) | (m - 1);
	writel(rval, mmchost->mclkbase);

	return 0;
}

static unsigned mmc_get_mclk(struct sunxi_mmc_host* mmchost)
{
	unsigned n, m, src, sclk_hz = 0;
	unsigned rval = readl(mmchost->mclkbase);

	m = rval & 0xf;
	n = (rval>>16) & 0x3;
	src = (rval>>24) & 0x3;

	if (src == 0)
		sclk_hz = 24000000;
	else if (src == 1)
		sclk_hz = sunxi_clock_get_pll6()*1000000;
	else if (src == 2) {
		/*todo*/
	} else {
		MMCINFO("%s: wrong clock source %d\n", src);
	}

	return (sclk_hz / (1<<n) / (m+1) );
}

#endif /*FPGA_PLATFORM*/


static int mmc_config_clock_modex(struct sunxi_mmc_host* mmchost, unsigned clk)
{
	unsigned rval = 0;
	struct mmc *mmc = mmchost->mmc;
	unsigned mode = mmchost->timing_mode;

#ifndef FPGA_PLATFORM
	/* disable mclk */
	writel(0x0, mmchost->mclkbase);
	MMCDBG("mmc %d mclkbase 0x%x\n", mmchost->mmc_no, readl(mmchost->mclkbase));

	/* disable timing mode 1 */
	if (mode == SUNXI_MMC_TIMING_MODE_1) {
		rval = readl(&mmchost->reg->ntsr);
		rval |= (1<<31);
		writel(rval, &mmchost->reg->ntsr);
		MMCDBG("mmc %d rntsr 0x%x\n", mmchost->mmc_no, rval);
	} else
		writel(0x0, &mmchost->reg->ntsr);

	/* configure clock */
	if (mode == SUNXI_MMC_TIMING_MODE_1) {
		if (mmc->io_mode == MMC_MODE_DDR_52MHz)
			mmchost->mod_clk = clk * 4;
		else
			mmchost->mod_clk = clk * 2;
	} else if (mode == SUNXI_MMC_TIMING_MODE_3) {
		if (mmc->io_mode == MMC_MODE_DDR_52MHz)
			mmchost->mod_clk = clk * 4;
		else
			mmchost->mod_clk = clk * 2;
	} else if (mode == SUNXI_MMC_TIMING_MODE_4) {
		if ((mmc->io_mode == MMC_MODE_DDR_52MHz)
			&& (mmc->bus_width == 8))
			mmchost->mod_clk = clk * 4; /* 4xclk: DDR8(HS) */
		else
			mmchost->mod_clk = clk * 2; /* 2xclk: SDR 1/4/8; DDR4(HS); DDR8(HS400)  */
	}

	mmc_set_mclk(mmchost, mmchost->mod_clk);
	if (mmc->io_mode == MMC_MODE_DDR_52MHz)
		mmc->clock = mmc_get_mclk(mmchost) / 4;
	else
		mmc->clock = mmc_get_mclk(mmchost) / 2;
	MMCDBG("get round card clk %d, mod_clk %d\n", mmc->clock, mmchost->mod_clk);

	/* re-enable mclk */
	writel(readl(mmchost->mclkbase)|(1<<31),mmchost->mclkbase);
	MMCDBG("mmc %d mclkbase 0x%x\n", mmchost->mmc_no, readl(mmchost->mclkbase));


	/*
	 * CLKCREG[7:0]: divider
	 * CLKCREG[16]:  on/off
	 * CLKCREG[17]:  power save
	 */
	rval = readl(&mmchost->reg->clkcr);
	rval &= ~(0xFF);
	if (mode == SUNXI_MMC_TIMING_MODE_1) {
		if (mmc->io_mode == MMC_MODE_DDR_52MHz)
			rval |= 0x1;
	} else if (mode == SUNXI_MMC_TIMING_MODE_3) {
		if (mmc->io_mode == MMC_MODE_DDR_52MHz)
			rval |= 0x1;
	} else if (mode == SUNXI_MMC_TIMING_MODE_4) {
		if ((mmc->io_mode == MMC_MODE_DDR_52MHz)
			&& (mmc->bus_width == 8))
			rval |= 0x1;
	}
	writel(rval, &mmchost->reg->clkcr);

	if (mmc_update_clk(mmchost))
		return -1;


	/* configure delay for current frequency */
	/* ??? */

#else
	unsigned div, sclk= 24000000;
	unsigned clk_2x = 0;

	if (mode == SUNXI_MMC_TIMING_MODE_1)
	{
		div = (2 * sclk + clk) / (2 * clk);
		rval = readl(&mmchost->reg->clkcr) & (~0xff);
		if (mmc->io_mode == MMC_MODE_DDR_52MHz)
			rval |= 0x1;
		else
			rval |= div >> 1;
		writel(rval, &mmchost->reg->clkcr);

		rval = readl(&mmchost->reg->ntsr);
		rval |= (1<<31);
		writel(rval, &mmchost->reg->ntsr);
		MMCDBG("mmc %d ntsr 0x%x, ckcr 0x%x\n", mmchost->mmc_no,
			readl(&mmchost->reg->ntsr), readl(&mmchost->reg->clkcr));
	}

	if ((mode == SUNXI_MMC_TIMING_MODE_3) || (mode == SUNXI_MMC_TIMING_MODE_4))
	{
		if (mode == SUNXI_MMC_TIMING_MODE_3) {
			if (mmc->io_mode == MMC_MODE_DDR_52MHz)
				clk_2x = clk << 2; //4xclk
			else
				clk_2x = clk << 1; //2xclk
		} else if (mode == SUNXI_MMC_TIMING_MODE_4) {
			if (mmc->io_mode == MMC_MODE_DDR_52MHz && mmc->bus_width == 8)
				clk_2x = clk << 2; //4xclk: DDR8(HS)
			else
				clk_2x = clk << 1; //2xclk: SDR 1/4/8; DDR4(HS); DDR8(HS400)
		}

		div = (2 * sclk + clk_2x) / (2 * clk_2x);
		rval = readl(&mmchost->reg->clkcr) & (~0xff);
		if (mmc->io_mode == MMC_MODE_DDR_52MHz)
			rval |= 0x1;
		else
			rval |= div >> 1;
		writel(rval, &mmchost->reg->clkcr);
	}

#endif

	//dumphex32("ccmu", (char*)SUNXI_CCM_BASE, 0x100);
	//dumphex32("gpio", (char*)SUNXI_PIO_BASE, 0x100);
	//dumphex32("mmc", (char*)mmchost->reg, 0x100);
	return 0;
}

static int mmc_config_clock(struct sunxi_mmc_host* mmchost, unsigned clk)
{
	unsigned rval = 0;

	/* disable card clock */
	rval = readl(&mmchost->reg->clkcr);
	rval &= ~(1 << 16);
	writel(rval, &mmchost->reg->clkcr);
	if(mmc_update_clk(mmchost))
		return -1;

	if ((mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_1)
		|| (mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_3)
		|| (mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_4) )

		mmc_config_clock_modex(mmchost, clk);

	else {
		MMCINFO("mmc %d wrong timing mode: 0x%x\n",
			mmchost->mmc_no, mmchost->timing_mode);
		return -1;
	}

	/* Re-enable card clock */
	rval = readl(&mmchost->reg->clkcr);
	rval |= (3 << 16);
	writel(rval, &mmchost->reg->clkcr);
	if(mmc_update_clk(mmchost)){
		MMCINFO("mmc %d re-enable clock failed\n",mmchost->mmc_no);
		return -1;
	}

	return 0;
}

static int mmc_calibrate_delay_unit(struct sunxi_mmc_host* mmchost)
{
	unsigned rval = 0;
	unsigned result = 0;

	/* close card clock */
	rval = readl(&mmchost->reg->clkcr);
	rval &= ~(1 << 16);
	writel(rval, &mmchost->reg->clkcr);
	if(mmc_update_clk(mmchost))
		return -1;

	/* set card clock to 100MHz */
	if ((mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_1)
		|| (mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_3)
		|| (mmchost->timing_mode == SUNXI_MMC_TIMING_MODE_4) )

		mmc_config_clock_modex(mmchost, 100000000);
	else {
		MMCINFO("mmc %d wrong timing mode: 0x%x\n",
			mmchost->mmc_no, mmchost->timing_mode);
		return -1;
	}

	/* start carlibrate delay unit */
	rval = readl(&mmchost->reg->samp_dl);
	rval |= SDXC_StartCal;
	writel(rval, &mmchost->reg->samp_dl);
	while (readl(&mmchost->reg->samp_dl) & SDXC_CalDone);

	if (mmchost->mmc_no == 2) {
		rval = readl(&mmchost->reg->ds_dl);
		rval |= SDXC_StartCal;
		writel(rval, &mmchost->reg->ds_dl);
		while (readl(&mmchost->reg->ds_dl) & SDXC_CalDone);
	}

	/* update result */
	rval = readl(&mmchost->reg->samp_dl);
	result = (rval & SDXC_CalDly) >> 8;
	/* 10ns= 10*1000 ps */
	mmchost->mmc_clk_dly_mode3.sdly_unit_ps = 10000 / result;
	mmchost->mmc_clk_dly_mode3.dly_cal_done = 1;

	mmchost->mmc_clk_dly_mode4.sdly_unit_ps = 10000 / result;

	if (mmchost->mmc_no == 2) {
		rval = readl(&mmchost->reg->ds_dl);
		result = (rval & SDXC_CalDly) >> 8;
		mmchost->mmc_clk_dly_mode4.ddly_unit_ps = 10000 / result;
		mmchost->mmc_clk_dly_mode4.dly_cal_done = 1;
	}

	return 0;
}


static void mmc_set_ios(struct mmc *mmc)
{
	struct sunxi_mmc_host* mmchost = (struct sunxi_mmc_host *)mmc->priv;
	unsigned rval = 0;

	MMCDBG("mmc %d ios: bus: %d, clock: %d\n", mmchost->mmc_no,mmc->bus_width, mmc->clock);

	/* change clock */
	if (mmc->clock && mmc_config_clock((struct sunxi_mmc_host*)mmc->priv, mmc->clock)) {
		MMCINFO("[mmc]: mmc %d update clock failed\n",mmchost->mmc_no);
		mmchost->fatal_err = 1;
		return;
	}

	/* Change bus width */
	if (mmc->bus_width == 8)
		writel(2, &mmchost->reg->width);
	else if (mmc->bus_width == 4)
		writel(1, &mmchost->reg->width);
	else
		writel(0, &mmchost->reg->width);

	/* set ddr mode */
	if (mmc->io_mode == MMC_MODE_DDR_52MHz) {
		rval = readl(&mmchost->reg->gctrl);
		rval |= 1 << 10;
		writel(rval, &mmchost->reg->gctrl);
		MMCDBG("set %d rgctrl 0x%x to enable ddr mode\n",mmchost->mmc_no,readl(&mmchost->reg->gctrl));
	}

	MMCDBG("host bus width register 0x%x\n",readl(&mmchost->reg->width));
}

static int mmc_core_init(struct mmc *mmc)
{
	struct sunxi_mmc_host* mmchost = (struct sunxi_mmc_host *)mmc->priv;

	/* Reset controller */
	writel(0x40000007, &mmchost->reg->gctrl);
	while(readl(&mmchost->reg->gctrl)&0x7);
	/* release eMMC reset signal */
	writel(1, &mmchost->reg->hwrst);
	writel(0, &mmchost->reg->hwrst);
	udelay(1000);
	writel(1, &mmchost->reg->hwrst);

	mmc_calibrate_delay_unit(mmchost);

	return 0;
}

static int mmc_trans_data_by_cpu(struct mmc *mmc, struct mmc_data *data)
{
	struct sunxi_mmc_host* mmchost = (struct sunxi_mmc_host *)mmc->priv;
	unsigned i;
	unsigned byte_cnt = data->blocksize * data->blocks;
	unsigned *buff;
	unsigned timeout = 1000;

	if (data->flags & MMC_DATA_READ) {
		buff = (unsigned int *)data->dest;
		for (i=0; i<(byte_cnt>>2); i++) {
			while(--timeout && (readl(&mmchost->reg->status)&(1 << 2))){
				__msdelay(1);
			}
			if (timeout <= 0)
				goto out;
			buff[i] = readl(mmchost->database);
			timeout = 1000;
		}
	} else {
		buff = (unsigned int *)data->src;
		for (i=0; i<(byte_cnt>>2); i++) {
			while(--timeout && (readl(&mmchost->reg->status)&(1 << 3))){
				__msdelay(1);
			}
			if (timeout <= 0)
				goto out;
			writel(buff[i], mmchost->database);
			timeout = 1000;
		}
	}

out:
	if (timeout <= 0){
		MMCINFO("transfer by cpu failed\n");
		return -1;
	}

	return 0;
}

static int mmc_trans_data_by_dma(struct mmc *mmc, struct mmc_data *data)
{
	struct sunxi_mmc_host* mmchost = (struct sunxi_mmc_host *)mmc->priv;
	struct sunxi_mmc_des *pdes = mmchost->pdes;
	unsigned byte_cnt = data->blocksize * data->blocks;
	unsigned char *buff;
	unsigned des_idx = 0;
	unsigned buff_frag_num = 0;
	unsigned remain;
	unsigned i, rval;

	buff = data->flags & MMC_DATA_READ ?
			(unsigned char *)data->dest : (unsigned char *)data->src;
	buff_frag_num = byte_cnt >> SDXC_DES_NUM_SHIFT;
	remain = byte_cnt & (SDXC_DES_BUFFER_MAX_LEN-1);
	if (remain)
		buff_frag_num ++;
	else
		remain = SDXC_DES_BUFFER_MAX_LEN;

	flush_cache((unsigned long)buff, (unsigned long)byte_cnt);
	for (i=0; i < buff_frag_num; i++, des_idx++) {
		memset((void*)&pdes[des_idx], 0, sizeof(struct sunxi_mmc_des));
		pdes[des_idx].des_chain = 1;
		pdes[des_idx].own = 1;
		pdes[des_idx].dic = 1;
		if (buff_frag_num > 1 && i != buff_frag_num-1)
			pdes[des_idx].data_buf1_sz = SDXC_DES_BUFFER_MAX_LEN;
		else
			pdes[des_idx].data_buf1_sz = remain;

		pdes[des_idx].buf_addr_ptr1 = (ulong)buff + i * SDXC_DES_BUFFER_MAX_LEN;
		if (i==0)
			pdes[des_idx].first_des = 1;

		if (i == buff_frag_num-1) {
			pdes[des_idx].dic = 0;
			pdes[des_idx].last_des = 1;
			pdes[des_idx].end_of_ring = 1;
			pdes[des_idx].buf_addr_ptr2 = 0;
		} else {
			pdes[des_idx].buf_addr_ptr2 = (ulong)&pdes[des_idx+1];
		}
//		MMCDBG("frag %d, remain %d, des[%d](%08x): "
//			"[0] = %08x, [1] = %08x, [2] = %08x, [3] = %08x\n",
//			i, remain, des_idx, (u32)&pdes[des_idx],
//			(u32)((u32*)&pdes[des_idx])[0], (u32)((u32*)&pdes[des_idx])[1],
//			(u32)((u32*)&pdes[des_idx])[2], (u32)((u32*)&pdes[des_idx])[3]);
	}
	flush_cache((unsigned long)pdes, sizeof(struct sunxi_mmc_des) * (des_idx+1));

	/*
	 * GCTRLREG
	 * GCTRL[2]	: DMA reset
	 * GCTRL[5]	: DMA enable
	 *
	 * IDMACREG
	 * IDMAC[0]	: IDMA soft reset
	 * IDMAC[1]	: IDMA fix burst flag
	 * IDMAC[7]	: IDMA on
	 *
	 * IDIECREG
	 * IDIE[0]	: IDMA transmit interrupt flag
	 * IDIE[1]	: IDMA receive interrupt flag
	 */
	rval = readl(&mmchost->reg->gctrl);
	writel(rval|(1 << 5)|(1 << 2), &mmchost->reg->gctrl);	/* dma enable */
	writel((1 << 0), &mmchost->reg->dmac); /* idma reset */
	writel((1 << 1) | (1 << 7), &mmchost->reg->dmac); /* idma on */
	rval = readl(&mmchost->reg->idie) & (~3);
	if (data->flags & MMC_DATA_WRITE)
		rval |= (1 << 0);
	else
		rval |= (1 << 1);
	writel(rval, &mmchost->reg->idie);
	writel((unsigned long)pdes, &mmchost->reg->dlba);
	writel((2U<<28)|(7<<16)|8, &mmchost->reg->ftrglevel);

	return 0;
}

static int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd,
				struct mmc_data *data)
{
	struct sunxi_mmc_host* mmchost = (struct sunxi_mmc_host *)mmc->priv;
	unsigned int cmdval = 0x80000000;
	signed int timeout = 0;
	int error = 0;
	unsigned int status = 0;
	unsigned int usedma = 0;
	unsigned int bytecnt = 0;

	if (mmchost->fatal_err) {
		MMCINFO("mmc %d Found fatal err,so no send cmd\n",mmchost->mmc_no);
		return -1;
	}

	if (cmd->resp_type & MMC_RSP_BUSY)
		MMCDBG("mmc %d mmc cmd %d check rsp busy\n", mmchost->mmc_no,cmd->cmdidx);
	if ((cmd->cmdidx == 12)&&!(cmd->flags&MMC_CMD_MANUAL)){
		MMCDBG("note we don't send stop cmd,only check busy here\n");
		timeout = 500*1000;
		do {
			status = readl(&mmchost->reg->status);
			if (!timeout--) {
				error = -1;
				MMCINFO("mmc %d cmd12 busy timeout\n",mmchost->mmc_no);
				goto out;
			}
			__usdelay(1);
		} while (status & (1 << 9));
		return 0;
	}
	/*
	 * CMDREG
	 * CMD[5:0]	: Command index
	 * CMD[6]	: Has response
	 * CMD[7]	: Long response
	 * CMD[8]	: Check response CRC
	 * CMD[9]	: Has data
	 * CMD[10]	: Write
	 * CMD[11]	: Steam mode
	 * CMD[12]	: Auto stop
	 * CMD[13]	: Wait previous over
	 * CMD[14]	: About cmd
	 * CMD[15]	: Send initialization
	 * CMD[21]	: Update clock
	 * CMD[31]	: Load cmd
	 */
	if (!cmd->cmdidx)
		cmdval |= (1 << 15);
	if (cmd->resp_type & MMC_RSP_PRESENT)
		cmdval |= (1 << 6);
	if (cmd->resp_type & MMC_RSP_136)
		cmdval |= (1 << 7);
	if (cmd->resp_type & MMC_RSP_CRC)
		cmdval |= (1 << 8);
	if (data) {
		if ((ulong)data->dest & 0x3) {
			MMCINFO("mmc %d dest is not 4 byte align\n",mmchost->mmc_no);
			error = -1;
			goto out;
		}

		cmdval |= (1 << 9) | (1 << 13);
		if (data->flags & MMC_DATA_WRITE)
			cmdval |= (1 << 10);
		if (data->blocks > 1&&!(cmd->flags&MMC_CMD_MANUAL))
			cmdval |= (1 << 12);
		writel(data->blocksize, &mmchost->reg->blksz);
		writel(data->blocks * data->blocksize, &mmchost->reg->bytecnt);
	} else {
		if ((cmd->cmdidx == 12)&&(cmd->flags&MMC_CMD_MANUAL)) {
			cmdval |= 1<<14;//stop current data transferin progress.
			cmdval &= ~(1 << 13);//Send command at once, even if previous data transfer has notcompleted
		}
	}

	MMCDBG("mmc %d, cmd %d(0x%08x), arg 0x%08x\n",
		mmchost->mmc_no, cmd->cmdidx, cmdval|cmd->cmdidx, cmd->cmdarg);
	writel(cmd->cmdarg, &mmchost->reg->arg);
	if (!data)
		writel(cmdval|cmd->cmdidx, &mmchost->reg->cmd);

	/*
	 * transfer data and check status
	 * STATREG[2] : FIFO empty
	 * STATREG[3] : FIFO full
	 */
	if (data) {
		int ret = 0;

		bytecnt = data->blocksize * data->blocks;
		MMCDBG("mmc %d trans data %d bytes\n",mmchost->mmc_no, bytecnt);
#ifdef CONFIG_MMC_SUNXI_USE_DMA
		if (bytecnt > 64) {
#else
		if (0) {
#endif
			usedma = 1;
			writel(readl(&mmchost->reg->gctrl)&(~0x80000000), &mmchost->reg->gctrl);
			ret = mmc_trans_data_by_dma(mmc, data);
			writel(cmdval|cmd->cmdidx, &mmchost->reg->cmd);
		} else {
			writel(readl(&mmchost->reg->gctrl)|0x80000000, &mmchost->reg->gctrl);
			writel(cmdval|cmd->cmdidx, &mmchost->reg->cmd);
			ret = mmc_trans_data_by_cpu(mmc, data);
		}
		if (ret) {
			MMCINFO("mmc %d Transfer failed\n",mmchost->mmc_no);
			error = readl(&mmchost->reg->rint) & 0xbfc2;
			if(!error)
				error = 0xffffffff;
			goto out;
		}
	}

	timeout = 1000;
	do {
		status = readl(&mmchost->reg->rint);
		if (!timeout-- || (status & 0xbfc2)) {
			error = status & 0xbfc2;
			if(!error)
				error = 0xffffffff;//represet software timeout
			MMCINFO("mmc %d cmd %d timeout, err %x\n",mmchost->mmc_no, cmd->cmdidx, error);
			goto out;
		}
		__usdelay(1);
	} while (!(status&0x4));

	if (data) {
		unsigned done = 0;
		timeout = usedma ? (50*bytecnt/25) : 0xffffff;//0.04us(25M)*2(4bit width)*25()
		if(timeout < 0xffffff){
			timeout = 0xffffff;
		}
		MMCDBG("mmc %d cacl timeout %x\n",mmchost->mmc_no, timeout);
		do {
			status = readl(&mmchost->reg->rint);
			if (!timeout-- || (status & 0xbfc2)) {
				error = status & 0xbfc2;
				if(!error)
					error = 0xffffffff;//represet software timeout
				MMCINFO("mmc %d data timeout %x\n",mmchost->mmc_no, error);
				goto out;
			}
			if ((data->blocks > 1)&&!(cmd->flags&MMC_CMD_MANUAL))//not wait auto stop when MMC_CMD_MANUAL is set
				done = status & (1 << 14);
			else
				done = status & (1 << 3);
			__usdelay(1);
		} while (!done);
	}

	if (cmd->resp_type & MMC_RSP_BUSY) {
		if ((cmd->cmdidx == MMC_CMD_ERASE)
			|| ((cmd->cmdidx == MMC_CMD_SWITCH)
				&&(((cmd->cmdarg>>16)&0xFF) == EXT_CSD_SANITIZE_START)))
			timeout = 0x1fffffff;
		else
			timeout = 500*1000;

		do {
			status = readl(&mmchost->reg->status);
			if (!timeout--) {
				error = -1;
				MMCINFO("mmc %d busy timeout\n",mmchost->mmc_no);
				goto out;
			}
			__usdelay(1);
		} while (status & (1 << 9));

		if ((cmd->cmdidx == MMC_CMD_ERASE)
			|| ((cmd->cmdidx == MMC_CMD_SWITCH)
				&&(((cmd->cmdarg>>16)&0xFF) == EXT_CSD_SANITIZE_START)))
			MMCINFO("%s: cmd %d wait rsp busy 0x%x us \n",__FUNCTION__,
				cmd->cmdidx, 0x1fffffff-timeout);
	}

	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = readl(&mmchost->reg->resp3);
		cmd->response[1] = readl(&mmchost->reg->resp2);
		cmd->response[2] = readl(&mmchost->reg->resp1);
		cmd->response[3] = readl(&mmchost->reg->resp0);
		MMCDBG("mmc %d mmc resp 0x%08x 0x%08x 0x%08x 0x%08x\n",
			mmchost->mmc_no,
			cmd->response[3], cmd->response[2],
			cmd->response[1], cmd->response[0]);
	} else {
		cmd->response[0] = readl(&mmchost->reg->resp0);
		MMCDBG("mmc %d mmc resp 0x%08x\n",mmchost->mmc_no, cmd->response[0]);
	}
out:
	if(error){
		mmchost->raw_int_bak = readl(&mmchost->reg->rint )& 0xbfc2;
		mmc_dump_errinfo(mmchost,cmd);
	}
	if (data && usedma) {
		/* IDMASTAREG
		 * IDST[0] : idma tx int
		 * IDST[1] : idma rx int
		 * IDST[2] : idma fatal bus error
		 * IDST[4] : idma descriptor invalid
		 * IDST[5] : idma error summary
		 * IDST[8] : idma normal interrupt sumary
		 * IDST[9] : idma abnormal interrupt sumary
		 */
		status = readl(&mmchost->reg->idst);
		writel(status, &mmchost->reg->idst);
		writel(0, &mmchost->reg->idie);
		writel(0, &mmchost->reg->dmac);
		writel(readl(&mmchost->reg->gctrl)&(~(1 << 5)), &mmchost->reg->gctrl);
	}
	if (error) {
		if(data && (data->flags&MMC_DATA_READ)&&(bytecnt==512)){
			writel(readl(&mmchost->reg->gctrl)|0x80000000, &mmchost->reg->gctrl);
			writel(0xdeb, &mmchost->reg->dbgc);
			timeout = 1000;
			int tmp;
			MMCINFO("Read remain data\n");
			while(readl(&mmchost->reg->bbcr)<512){
				tmp = readl(mmchost->database);
				 tmp=tmp;
				MMCDBG("Read data %x,bbcr %x\n",tmp,readl(&mmchost->reg->bbcr));
				__usdelay(1);
				if(!(timeout--)){
					MMCINFO("Read remain data timeout\n");
					break;
				}
			}
		}

		writel(0x7, &mmchost->reg->gctrl);
		while(readl(&mmchost->reg->gctrl)&0x7) { };

		mmc_update_clk(mmchost);
		MMCINFO("mmc %d mmc cmd %d err 0x%08x\n",mmchost->mmc_no, cmd->cmdidx, error);
	}
	writel(0xffffffff, &mmchost->reg->rint);
	//writel(readl(&mmchost->reg->gctrl)|(1 << 1), &mmchost->reg->gctrl);

	if (error)
		return -1;
	else
		return 0;
}

static const struct mmc_ops sunxi_mmc_ops = {
	.send_cmd	= mmc_send_cmd,
	.set_ios	= mmc_set_ios,
	.init		= mmc_core_init,
};

int sunxi_mmc_init(int sdc_no)
{
	struct sunxi_mmc_host *host = NULL;

	//while(*(volatile unsigned int*)(0x4A000000) != 0xA);

	MMCINFO("mmc driver ver %s\n", DRIVER_VER);

	//memset(&mmc_host[sdc_no], 0, sizeof(struct sunxi_mmc_host));
	host = &mmc_host[sdc_no];

	host->cfg.name = "SUNXI SD/MMC";
	host->cfg.ops = &sunxi_mmc_ops;

	host->cfg.voltages = MMC_VDD_32_33 | MMC_VDD_33_34
		| MMC_VDD_27_28 | MMC_VDD_28_29 | MMC_VDD_29_30
		| MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_34_35
		| MMC_VDD_35_36;
	/* bus width will be change in mmc_clk_io_onoff according to sys_config.fex */
	host->cfg.host_caps = MMC_MODE_4BIT; // | MMC_MODE_8BIT;
	host->cfg.host_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS| MMC_MODE_HC; // | MMC_MODE_DDR_52MHz;
	host->cfg.b_max = CONFIG_SYS_MMC_MAX_BLK_COUNT;

	if (sdc_no == 0) {
		host->cfg.f_min = 400000;
		host->cfg.f_max = 50000000;
	} else if (sdc_no == 2) {
		host->cfg.f_min = 400000;
		host->cfg.f_max = 50000000;
	}

	MMCDBG("mmc->host_caps %x\n", host->cfg.host_caps);
	host->pdes = malloc(64 * 1024);
	if (host->pdes == NULL) {
		MMCINFO("%s: get mem for descriptor failed\n", __FUNCTION__);
		return -1;
	}

	get_fex_para(sdc_no);
	mmc_resource_init(sdc_no);
	mmc_clk_io_onoff(sdc_no, 1);

	if ((sdc_no == 0) || (sdc_no ==1))
		host->timing_mode = SUNXI_MMC_TIMING_MODE_1; //SUNXI_MMC_TIMING_MODE_3
	else if (sdc_no == 2)
		host->timing_mode = SUNXI_MMC_TIMING_MODE_4;

	host->mmc = mmc_create(&host->cfg, host);
	if (host->mmc == NULL) {
		MMCINFO("%s: register mmc %d failed\n", __FUNCTION__, sdc_no);
		return -1;
	} else
		MMCINFO("%s: register mmc %d ok\n", __FUNCTION__, __LINE__);

	return 0;
}

int sunxi_mmc_exit(int sdc_no)
{
	mmc_clk_io_onoff(sdc_no, 0);
	//mmc_unregister(sdc_no);

	memset(&mmc_host[sdc_no], 0, sizeof(struct sunxi_mmc_host));
	MMCDBG("sunxi mmc%d exit\n", sdc_no);
	return 0;
}
