/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * lixiang <lixiang@allwinnertech.com>
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
#include <asm/arch/rsb.h>
#include <asm/arch/platform.h>

struct sunxi_rsb_slave_set
{
	u8 *m_slave_name;
	u32 m_slave_addr;
	u32 m_rtaddr;
	u32 chip_id;
};

#define  SUNXI_RSB_SLAVE_MAX      13

DECLARE_GLOBAL_DATA_PTR;

static s32 sunxi_rsb_config_secos(u32 slave_id, u32 rsb_addr);
static s32 sunxi_rsb_read_secos(u32 slave_id, u32 daddr, u8 *data, u32 len);
static s32 sunxi_rsb_write_secos(u32 slave_id, u32 daddr, u8 *data, u32 len);
static s32 sunxi_rsb_config_null(u32 slave_id, u32 rsb_addr);
static s32 sunxi_rsb_io_null(u32 slave_id, u32 daddr, u8 *data, u32 len);

static struct rsb_info rsbc = {1, 1, 1};
static struct sunxi_rsb_slave_set rsb_slave[SUNXI_RSB_SLAVE_MAX]={{NULL, 1, 1, 1}};
static int    sunxi_rsb_rtsaddr[16] = {0x2d, 0x3a, 0x4e, 0x59, 0x63, 0x74, 0x8b, 0x9c, 0xa6, 0xb1, 0xc5, 0xd2, 0xe8, 0xff};

static void rsb_cfg_io(void)
{
#if (defined(CONFIG_A73_FPGA) || defined(CONFIG_A67_FPGA) || defined(CONFIG_A50_FPGA) || (defined(CONFIG_A39_FPGA)))
//	gpio_set_cfg(GPIO_H(14), 2, 3);
//	gpio_set_pull(GPIO_H(14), 2, 1);
//	gpio_set_drv(GPIO_H(14), 2, 2);
	//PH14,PH15 cfg 3
	rsb_reg_writel(rsb_reg_readl(SUNXI_PIO_BASE+0x100)& ~(0xff<<24),SUNXI_PIO_BASE+0x100);
	rsb_reg_writel(rsb_reg_readl(SUNXI_PIO_BASE+0x100)|(0x33<<24),SUNXI_PIO_BASE+0x100);
	//PH14,PH15 pull up 1
	rsb_reg_writel(rsb_reg_readl(SUNXI_PIO_BASE+0x118)& ~(0xf<<28),SUNXI_PIO_BASE+0x118);
	rsb_reg_writel(rsb_reg_readl(SUNXI_PIO_BASE+0x118)|(0x5<<28),SUNXI_PIO_BASE+0x118);
	//PH14,PH15 drv 2
	rsb_reg_writel(rsb_reg_readl(SUNXI_PIO_BASE+0x110)& ~(0xf<<28),SUNXI_PIO_BASE+0x110);
	rsb_reg_writel(rsb_reg_readl(SUNXI_PIO_BASE+0x110)|(0xa<<28),SUNXI_PIO_BASE+0x110);
#elif (defined(CONFIG_ARCH_SUN8IW3P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1) )
//	r_gpio_set_cfg(R_GPIO_L(0), 2, 2);
//	r_gpio_set_pull(R_GPIO_L(0), 2, 1);
//	r_gpio_set_drv(R_GPIO_L(0), 2, 2);
	//PL0,PL1 cfg 2
	rsb_reg_writel(rsb_reg_readl(0x01f02c00)& ~0xff,0x01f02c00);
	rsb_reg_writel(rsb_reg_readl(0x01f02c00)|0x22,0x01f02c00);
	//PL0,PL1 pull up 1
	rsb_reg_writel(rsb_reg_readl(0x01f02c00+0x1c)& ~0xf,0x01f02c00+0x1c);
	rsb_reg_writel(rsb_reg_readl(0x01f02c00+0x1c)|0x5,0x01f02c00+0x1c);
	//PL0,PL1 drv 2
	rsb_reg_writel(rsb_reg_readl(0x01f02c00+0x14)& ~0xf,0x01f02c00+0x14);
	rsb_reg_writel(rsb_reg_readl(0x01f02c00+0x14)|0xa,0x01f02c00+0x14);
#elif defined(CONFIG_ARCH_SUN9IW1P1)
	//PN0,PN1 cfg 3
	rsb_reg_writel(rsb_reg_readl(0x08002c00+0x48)& ~0xff,0x08002c00+0x48);
	rsb_reg_writel(rsb_reg_readl(0x08002c00+0x48)|0x33,0x08002c00+0x48);
	//PN0,PN1 pull up 1
	rsb_reg_writel(rsb_reg_readl(0x08002c00+0x64)& ~0xf,0x08002c00+0x64);
	rsb_reg_writel(rsb_reg_readl(0x08002c00+0x64)|0x5,0x08002c00+0x64);
	//PN0,PN1 drv 2
	rsb_reg_writel(rsb_reg_readl(0x08002c00+0x5c)& ~0xf,0x08002c00+0x5c);
	rsb_reg_writel(rsb_reg_readl(0x08002c00+0x5c)|0xa,0x08002c00+0x5c);

#else

#endif
}


static void rsb_module_reset(void)
{
//	r_prcm_module_reset(R_RSB_CKID);
	rsb_reg_writel(rsb_reg_readl(SUNXI_RPRCM_BASE + 0xb0)& ~(0x1U << 3),SUNXI_RPRCM_BASE + 0xb0);
	rsb_reg_writel(rsb_reg_readl(SUNXI_RPRCM_BASE + 0xb0)|(0x1U << 3),SUNXI_RPRCM_BASE + 0xb0);
}


static void rsb_clock_enable(void)
{
//	r_prcm_clock_enable(R_RSB_CKID);
	rsb_reg_writel(rsb_reg_readl(SUNXI_RPRCM_BASE + 0x28)|(0x1U << 3),SUNXI_RPRCM_BASE + 0x28);
}

static void rsb_set_clk(u32 sck)
{
	u32 src_clk = 0;
	u32 div = 0;
	u32 cd_odly = 0;
	u32 rval = 0;

	src_clk = 24000000;

	div = src_clk/sck/2;
	if(0==div){
		div = 1;
		rsb_printk("Source clock is too low\n");
	}else if(div>256){
		div = 256;
		rsb_printk("Source clock is too high\n");
	}
	div--;
	cd_odly = div >> 1;
	//cd_odly = 1;
	if(!cd_odly)
		cd_odly = 1;
	rval = div | (cd_odly << 8);
	rsb_reg_writel(rval, RSB_REG_CCR);
}

/*	RSB function	*/
#ifdef RSB_USE_INT
static void rsb_irq_handler(void)
{
	u32 istat = rsb_reg_readl(RSB_REG_STAT);

	if(istat & RSB_LBSY_INT){
		rsbc.rsb_load_busy = 1;
	}

	if (istat & RSB_TERR_INT) {
		rsbc.rsb_flag = (istat >> 8) & 0xffff;
	}

	if (istat & RSB_TOVER_INT) {
		rsbc.rsb_busy = 0;
	}

	rsb_reg_writel(istat, RSB_REG_STAT);
}
#endif

static void rsb_init(void)
{
	rsbc.rsb_flag = 0;
	rsbc.rsb_busy = 0;
	rsbc.rsb_load_busy	= 0;

	rsb_cfg_io();
	rsb_module_reset();
	rsb_clock_enable();

	rsb_reg_writel(RSB_SOFT_RST, RSB_REG_CTRL);
	rsb_set_clk(RSB_SCK);
#ifdef RSB_USE_INT
	rsb_reg_writel(RSB_GLB_INTEN, RSB_REG_CTRL);
	rsb_reg_writel(RSB_TOVER_INT|RSB_TERR_INT|RSB_LBSY_INT, RSB_REG_INTE);
	irq_request(GIC_SRC_RRSB, rsb_irq_handler);
	irq_enable(GIC_SRC_RRSB);
#endif
}

//
static s32 rsb_send_initseq(u32 slave_addr, u32 reg, u32 data)
{

	while(rsb_reg_readl(RSB_REG_STAT)&(RSB_LBSY_INT|RSB_TERR_INT|RSB_TOVER_INT))
	{
		rsb_printk("status err\n");
	}

	rsbc.rsb_busy = 1;
	rsbc.rsb_flag = 0;
	rsbc.rsb_load_busy = 0;
	rsb_reg_writel(RSB_PMU_INIT|(slave_addr << 1)				\
					|(reg << PMU_MOD_REG_ADDR_SHIFT)			\
					|(data << PMU_INIT_DAT_SHIFT), 				\
					RSB_REG_PMCR);
	while(rsb_reg_readl(RSB_REG_PMCR) & RSB_PMU_INIT){
	}


	while(rsbc.rsb_busy){
#ifndef RSB_USE_INT
		//istat will be optimize?
		u32 istat = rsb_reg_readl(RSB_REG_STAT);

		if(istat & RSB_LBSY_INT){
			rsbc.rsb_load_busy = 1;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TERR_INT) {
			rsbc.rsb_flag = (istat >> 8) & 0xffff;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TOVER_INT) {
			rsbc.rsb_busy = 0;
			rsb_reg_writel(istat, RSB_REG_STAT);
		}
#endif
	}

	if(rsbc.rsb_load_busy){
		rsb_printk("Load busy\n");
		return RET_FAIL;
	}

	if (rsbc.rsb_flag) {
		rsb_printk(	"rsb write failed, flag 0x%x:%s%s%s%s%s !!\n",
					rsbc.rsb_flag,
					rsbc.rsb_flag & ERR_TRANS_1ST_BYTE	? " 1STE "  : "",
					rsbc.rsb_flag & ERR_TRANS_2ND_BYTE	? " 2NDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_3RD_BYTE	? " 3RDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_4TH_BYTE	? " 4THE "  : "",
					rsbc.rsb_flag & ERR_TRANS_RT_NO_ACK	? " NOACK "	: ""
					);
		return -rsbc.rsb_flag;
	}

	return 0;
}


static s32 set_run_time_addr(u32 saddr,u32 rtsaddr)
{

	while(rsb_reg_readl(RSB_REG_STAT)&(RSB_LBSY_INT|RSB_TERR_INT|RSB_TOVER_INT))
	{
		rsb_printk("status err\n");
	}

	rsbc.rsb_busy = 1;
	rsbc.rsb_flag = 0;
	rsbc.rsb_load_busy = 0;
	rsb_reg_writel((saddr<<RSB_SADDR_SHIFT)						\
					|(rtsaddr<<RSB_RTSADDR_SHIFT),				\
					RSB_REG_SADDR);
	rsb_reg_writel(RSB_CMD_SET_RTSADDR,RSB_REG_CMD);
	rsb_reg_writel(rsb_reg_readl(RSB_REG_CTRL)|RSB_START_TRANS, RSB_REG_CTRL);

	while(rsbc.rsb_busy){
#ifndef RSB_USE_INT
		//istat will be optimize?
		u32 istat = rsb_reg_readl(RSB_REG_STAT);

		if(istat & RSB_LBSY_INT){
			rsbc.rsb_load_busy = 1;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TERR_INT) {
			rsbc.rsb_flag = (istat >> 8) & 0xffff;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TOVER_INT) {
			rsbc.rsb_busy = 0;
			rsb_reg_writel(istat, RSB_REG_STAT);
		}
#endif
	}

	if(rsbc.rsb_load_busy){
		rsb_printk("Load busy\n");
		return RET_FAIL;
	}

	if (rsbc.rsb_flag) {
		rsb_printk(	"rsb set run time address failed, flag 0x%x:%s%s%s%s%s !!\n",
					rsbc.rsb_flag,
					rsbc.rsb_flag & ERR_TRANS_1ST_BYTE	? " 1STE "  : "",
					rsbc.rsb_flag & ERR_TRANS_2ND_BYTE	? " 2NDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_3RD_BYTE	? " 3RDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_4TH_BYTE	? " 4THE "  : "",
					rsbc.rsb_flag & ERR_TRANS_RT_NO_ACK	? " NOACK "	: ""
					);
		return -rsbc.rsb_flag;
	}
	return 0;
}


//s32 rsb_write(u32 rtsaddr,struct rsb_ad *ad, u32 len)
static s32 rsb_write(u32 rtsaddr,u32 daddr, u8 *data,u32 len)
{
	u32 cmd = 0;
	u32 dat = 0;
	s32 i	= 0;
	if (len > 4 || len==0||len==3) {
		rsb_printk("error length %d\n", len);
		return -1;
	}
	if(NULL==data){
		rsb_printk("data should not be NULL\n");
		return -1;
	}

	while(rsb_reg_readl(RSB_REG_STAT)&(RSB_LBSY_INT|RSB_TERR_INT|RSB_TOVER_INT))
	{
		rsb_printk("status err\n");
	}

	rsbc.rsb_flag = 0;
	rsbc.rsb_busy = 1;
	rsbc.rsb_load_busy	= 0;

	rsb_reg_writel(rtsaddr<<RSB_RTSADDR_SHIFT,RSB_REG_SADDR);
	rsb_reg_writel(daddr, RSB_REG_DADDR0);

	for(i=0;i<len;i++){
		dat |= data[i]<<(i*8);
	}

	rsb_reg_writel(dat, RSB_REG_DATA0);
	//rsb_reg_writel(*((u32*)data), RSB_REG_DATA0);
	//rsb_reg_writew(*((u16*)data), RSB_REG_DATA0);
//	rsb_reg_writel((len-1)|RSB_WRITE_FLAG, RSB_REG_DLEN);

	switch(len)	{
	case 1:
		cmd = RSB_CMD_BYTE_WRITE;
		break;
	case 2:
		cmd = RSB_CMD_HWORD_WRITE;
		break;
	case 4:
		cmd = RSB_CMD_WORD_WRITE;
		break;
	default:
		break;
	}
	rsb_reg_writel(cmd,RSB_REG_CMD);

	rsb_reg_writel(rsb_reg_readl(RSB_REG_CTRL)|RSB_START_TRANS, RSB_REG_CTRL);
	while(rsbc.rsb_busy){
#ifndef RSB_USE_INT
		//istat will be optimize?
		u32 istat = rsb_reg_readl(RSB_REG_STAT);

		if(istat & RSB_LBSY_INT){
			rsbc.rsb_load_busy = 1;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TERR_INT) {
			rsbc.rsb_flag = (istat >> 8) & 0xffff;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TOVER_INT) {
			rsbc.rsb_busy = 0;
			rsb_reg_writel(istat, RSB_REG_STAT);
		}
#endif
	}

	if(rsbc.rsb_load_busy){
		rsb_printk("Load busy\n");
		return RET_FAIL;
	}

	if (rsbc.rsb_flag) {
		rsb_printk(	"rsb write failed, flag 0x%x:%s%s%s%s%s !!\n",
					rsbc.rsb_flag,
					rsbc.rsb_flag & ERR_TRANS_1ST_BYTE	? " 1STE "  : "",
					rsbc.rsb_flag & ERR_TRANS_2ND_BYTE	? " 2NDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_3RD_BYTE	? " 3RDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_4TH_BYTE	? " 4THE "  : "",
					rsbc.rsb_flag & ERR_TRANS_RT_NO_ACK	? " NOACK "	: ""
					);
		return -rsbc.rsb_flag;
	}

	return 0;
}


//s32 rsb_read(u32 rtsaddr,struct rsb_ad *ad, u32 len)
static s32 rsb_read(u32 rtsaddr,u32 daddr, u8 *data, u32 len)
{
	u32 cmd = 0;
	u32 dat = 0;
	s32 i	= 0;
	if (len > 4 || len==0||len==3) {
		rsb_printk("error length %d\n", len);
		return -1;
	}
	if(NULL==data){
		rsb_printk("data should not be NULL\n");
		return -1;
	}

	while(rsb_reg_readl(RSB_REG_STAT)&(RSB_LBSY_INT|RSB_TERR_INT|RSB_TOVER_INT))
	{
		rsb_printk("status err\n");
	}

	rsbc.rsb_flag = 0;
	rsbc.rsb_busy = 1;
	rsbc.rsb_load_busy	= 0;

	rsb_reg_writel(rtsaddr<<RSB_RTSADDR_SHIFT,RSB_REG_SADDR);
	rsb_reg_writel(daddr, RSB_REG_DADDR0);
//	rsb_reg_writel((len-1)|RSB_READ_FLAG, RSB_REG_DLEN);

	switch(len){
	case 1:
		cmd = RSB_CMD_BYTE_READ;
		break;
	case 2:
		cmd = RSB_CMD_HWORD_READ;
		break;
	case 4:
		cmd = RSB_CMD_WORD_READ;
		break;
	default:
		break;
	}
	rsb_reg_writel(cmd,RSB_REG_CMD);

	rsb_reg_writel(rsb_reg_readl(RSB_REG_CTRL)|RSB_START_TRANS, RSB_REG_CTRL);
	while(rsbc.rsb_busy){
#ifndef RSB_USE_INT
		//istat will be optimize?
		u32 istat = rsb_reg_readl(RSB_REG_STAT);

		if(istat & RSB_LBSY_INT){
			rsbc.rsb_load_busy = 1;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TERR_INT) {
			rsbc.rsb_flag = (istat >> 8) & 0xffff;
			rsb_reg_writel(istat, RSB_REG_STAT);
			break;
		}

		if (istat & RSB_TOVER_INT) {
			rsbc.rsb_busy = 0;
			rsb_reg_writel(istat, RSB_REG_STAT);
		}
#endif
	}

	if(rsbc.rsb_load_busy){
		rsb_printk("Load busy\n");
		return RET_FAIL;
	}

	if (rsbc.rsb_flag) {
		rsb_printk(	"rsb read failed, flag 0x%x:%s%s%s%s%s !!\n",
					rsbc.rsb_flag,
					rsbc.rsb_flag & ERR_TRANS_1ST_BYTE	? " 1STE "  : "",
					rsbc.rsb_flag & ERR_TRANS_2ND_BYTE	? " 2NDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_3RD_BYTE	? " 3RDE "  : "",
					rsbc.rsb_flag & ERR_TRANS_4TH_BYTE	? " 4THE "  : "",
					rsbc.rsb_flag & ERR_TRANS_RT_NO_ACK	? " NOACK "	: ""
					);
		return -rsbc.rsb_flag;
	}

	//*((u32*)data) = rsb_reg_readl(RSB_REG_DATA0);
	//*((u16*)data) = rsb_reg_readw(RSB_REG_DATA0);

	dat = rsb_reg_readl(RSB_REG_DATA0);
	for(i=0;i<len;i++){
		data[i]=(dat>>(i*8))&0xff;
	}


	return 0;
}


#define CD_HIGH    0x3
#define CD_LOW     0x1
#define CK_HIGH    (0x3<<2)
#define CK_LOW     (0x1<<2)


//static void rsb_set_ck_level(u32 level)
//{
//	u32 rval = rsb_reg_readb(RSB_REG_LCR);
//
//	rval &= ~(CK_HIGH);
//	if (level)
//		rsb_reg_writeb(rval | CK_HIGH, RSB_REG_LCR);
//	else
//		rsb_reg_writeb(rval | CK_LOW, RSB_REG_LCR);
//}

//static void rsb_set_cd_level(u32 level)
//{
//	u32 rval = rsb_reg_readb(RSB_REG_LCR);
//
//	rval &= ~(CD_HIGH);
//	if (level)
//		rsb_reg_writeb(rval | CD_HIGH, RSB_REG_LCR);
//	else
//		rsb_reg_writeb(rval | CD_LOW, RSB_REG_LCR);
//}

//static u32 rsb_get_ck_level(void)
//{
//	return 0x1 & (rsb_reg_readb(RSB_REG_LCR) >> 5);
//}
//
//static u32 rsb_get_cd_level(void)
//{
//	return 0x1 & (rsb_reg_readb(RSB_REG_LCR) >> 4);
//}


s32 (* sunxi_rsb_config_pt)(u32 slave_id, u32 rsb_addr) = sunxi_rsb_config_null;
s32 (* sunxi_rsb_read_pt)(u32 slave_id, u32 daddr, u8 *data, u32 len) = sunxi_rsb_io_null;
s32 (* sunxi_rsb_write_pt)(u32 slave_id, u32 daddr, u8 *data, u32 len) = sunxi_rsb_io_null;


static s32 sunxi_rsb_config_null(u32 slave_id, u32 rsb_addr)
{
	return -1;
}

static s32 sunxi_rsb_io_null(u32 slave_id, u32 daddr, u8 *data, u32 len)
{
	return -1;
}

s32 sunxi_rsb_init(u32 slave_id)
{
	if(gd->securemode != SUNXI_SECURE_MODE_NO_SECUREOS)
	{
    	memset(rsb_slave, 0, SUNXI_RSB_SLAVE_MAX * sizeof(struct sunxi_rsb_slave_set));

		printf("normal or secure os\n");
		rsb_init();
		rsb_send_initseq(0x00, 0x3e, 0x7c);

		sunxi_rsb_config_pt = sunxi_rsb_config_secos;
		sunxi_rsb_read_pt  = sunxi_rsb_read_secos;
		sunxi_rsb_write_pt = sunxi_rsb_write_secos;
	}
	else
	{
		printf("without secure os\n");
	}

	return 0;
}

s32 sunxi_rsb_exit(u32 slave_id)
{
	return 0;
}

static s32 sunxi_rsb_config_secos(u32 slave_id, u32 rsb_addr)
{
	u32 rtaddr 		= 0;
    int i;

    for(i=0;i<SUNXI_RSB_SLAVE_MAX;i++)
    {
        if(!rsb_slave[i].m_slave_addr)
        {
            rsb_slave[i].m_slave_addr = rsb_addr;
            rsb_slave[i].m_rtaddr     = sunxi_rsb_rtsaddr[i];
            rsb_slave[i].chip_id      = slave_id;

            rtaddr     = sunxi_rsb_rtsaddr[i];

            return set_run_time_addr(rsb_addr, rtaddr);
        }
    }

    return -1;
}

static s32 sunxi_rsb_read_secos(u32 slave_id, u32 daddr, u8 *data, u32 len)
{
	u32 rtaddr 	= 0;
	u32 tmp_slave_id;
    int i;

	for(i=0;;)
    {
        tmp_slave_id = rsb_slave[i].chip_id;
        if(tmp_slave_id == slave_id)
        {
            break;
        }
        else if(!tmp_slave_id)
        {
            rsb_printk("sunxi_rsb_read err: bad id\n");
		    return -1;
        }
        i++;
    }

	rtaddr = rsb_slave[i].m_rtaddr;

	return rsb_read(rtaddr,daddr, data,len);
}


static s32 sunxi_rsb_write_secos(u32 slave_id, u32 daddr, u8 *data, u32 len)
{
	u32 rtaddr 	= 0;
	u32 tmp_slave_id;
    int i;

	for(i=0;;)
    {
        tmp_slave_id = rsb_slave[i].chip_id;
        if(tmp_slave_id == slave_id)
        {
            break;
        }
        else if(!tmp_slave_id)
        {
            rsb_printk("sunxi_rsb_write err: bad id\n");
		    return -1;
        }
        i++;
    }

	rtaddr = rsb_slave[i].m_rtaddr;

	return rsb_write(rtaddr,daddr, data,len);
}

int sunxi_rsb_config(u32 slave_id, u32 rsb_addr)
{
	return sunxi_rsb_config_pt(slave_id, rsb_addr);
}

int sunxi_rsb_read(u32 slave_id, u32 daddr, u8 *data, u32 len)
{
	return sunxi_rsb_read_pt(slave_id, daddr, data, len);
}

int sunxi_rsb_write(u32 slave_id, u32 daddr, u8 *data, u32 len)
{
	return sunxi_rsb_write_pt(slave_id, daddr, data, len);
}

