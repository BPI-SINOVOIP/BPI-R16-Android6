/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
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
#include <asm/arch/ccmu.h>
#include <asm/arch/gic.h>

DECLARE_GLOBAL_DATA_PTR;

struct _irq_handler
{
	void                *m_data;
	void (*m_func)( void * data);
};

struct _irq_handler sunxi_int_handlers[GIC_IRQ_NUM];
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void default_isr(void *data)
{
	printf("default_isr():  called from IRQ %d\n", (uint)data);
	while(1);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int irq_enable(int irq_no)
{
	uint reg_val;
	uint offset;

	if (irq_no >= GIC_IRQ_NUM)
	{
		printf("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	offset   = irq_no >> 5; // ��32
	reg_val  = readl(GIC_SET_EN(offset));
	reg_val |= 1 << (irq_no & 0x1f);
	writel(reg_val, GIC_SET_EN(offset));

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int irq_disable(int irq_no)
{
	uint reg_val;
	uint offset;

	if (irq_no >= GIC_IRQ_NUM)
	{
		printf("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", irq_no, GIC_IRQ_NUM);
		return -1;
	}

	offset   = irq_no >> 5; // ��32
	reg_val = (1 << (irq_no & 0x1f));
	writel(reg_val, GIC_CLR_EN(offset));

	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void gic_sgi_handler(uint irq_no)
{
	printf("SGI irq %d coming... \n", irq_no);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void gic_ppi_handler(uint irq_no)
{
	printf("PPI irq %d coming... \n", irq_no);
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void gic_spi_handler(uint irq_no)
{
	if (sunxi_int_handlers[irq_no].m_func != default_isr)
	{
		sunxi_int_handlers[irq_no].m_func(sunxi_int_handlers[irq_no].m_data);
	}
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void gic_clear_pending(uint irq_no)
{
	uint reg_val;
	uint offset;

	offset = irq_no >> 5; // ��32
	reg_val = readl(GIC_PEND_CLR(offset));
	reg_val |= (1 << (irq_no & 0x1f));
	writel(reg_val, GIC_PEND_CLR(offset));

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void irq_install_handler (int irq, interrupt_handler_t handle_irq, void *data)
{
	disable_interrupts();
	if (irq >= GIC_IRQ_NUM || !handle_irq)
	{
		enable_interrupts();
		return;
	}

	sunxi_int_handlers[irq].m_data = data;
	sunxi_int_handlers[irq].m_func = handle_irq;

	enable_interrupts();
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
void irq_free_handler(int irq)
{
	disable_interrupts();
	if (irq >= GIC_IRQ_NUM)
	{
		enable_interrupts();
		return;
	}

	sunxi_int_handlers[irq].m_data = NULL;
	sunxi_int_handlers[irq].m_func = default_isr;

	enable_interrupts();
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
#ifdef CONFIG_USE_IRQ
void do_irq (struct pt_regs *pt_regs)
{
	u32 idnum;
	volatile u32 secmode;

	secmode = gd->securemode;
	if(secmode)
		idnum = readl(GIC_AIAR_REG);
	else
		idnum = readl(GIC_INT_ACK_REG);

	if ((idnum == 1022) || (idnum == 1023))
	{
		printf("spurious irq !!\n");
		return;
	}
	if (idnum >= GIC_IRQ_NUM) {
		printf("irq NO.(%d) > GIC_IRQ_NUM(%d) !!\n", idnum, GIC_IRQ_NUM-32);
		return;
	}
	if (idnum < 16)
		gic_sgi_handler(idnum);
	else if (idnum < 32)
		gic_ppi_handler(idnum);
	else
		gic_spi_handler(idnum);

	if(secmode)
		writel(idnum, GIC_AEOI_REG);
	else
	{
		writel(idnum, GIC_END_INT_REG);
		writel(idnum, GIC_DEACT_INT_REG);
	}
	//writel(idnum, GIC_DEACT_INT_REG);
	gic_clear_pending(idnum);

	return;
}
#endif
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
extern int get_cluster_id(void);
static void gic_distributor_init(void)
{
	__u32 cpumask = 0x01010101;
    __u32 gic_irqs;
	__u32 i;

	writel(0, GIC_DIST_CON);
    if(get_cluster_id() == 1)
        cpumask = 0x10101010;
	/* check GIC hardware configutation */
	gic_irqs = ((readl(GIC_CON_TYPE) & 0x1f) + 1) * 32;
	if (gic_irqs > 1020)
	{
		gic_irqs = 1020;
	}
	if (gic_irqs < GIC_IRQ_NUM)
	{
		printf("GIC parameter config error, only support %d"
				" irqs < %d(spec define)!!\n", gic_irqs, GIC_IRQ_NUM);
		return ;
	}
	if(gd->securemode)
	{
		/*  Set ALL interrupts as group1(non-secure) interrupts */
		for (i=1; i<GIC_IRQ_NUM; i+=16)
		{
		    writel(0xffffffff, GIC_CON_IGRP(i>>4));
		}
	}
	/* set trigger type to be level-triggered, active low */
	for (i=0; i<GIC_IRQ_NUM; i+=16)
	{
		writel(0, GIC_IRQ_MOD_CFG(i>>4));
	}
	/* set priority */
	for (i=GIC_SRC_SPI(0); i<GIC_IRQ_NUM; i+=4)
	{
		writel(0xa0a0a0a0, GIC_SPI_PRIO((i-32)>>2));
	}
	/* set processor target */
	for (i=32; i<GIC_IRQ_NUM; i+=4)
	{
		writel(cpumask, GIC_SPI_PROC_TARG((i-32)>>2));
	}
	/* disable all interrupts */
	for (i=32; i<GIC_IRQ_NUM; i+=32)
	{
		writel(0xffffffff, GIC_CLR_EN(i>>5));
	}
	/* clear all interrupt active state */
	for (i=32; i<GIC_IRQ_NUM; i+=32)
	{
		writel(0xffffffff, GIC_ACT_CLR(i>>5));
	}
	if(gd->securemode)
		writel(3, GIC_DIST_CON);
	else
		writel(1, GIC_DIST_CON);

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
static void gic_cpuif_init(void)
{
	uint i;

	writel(0, GIC_CPU_IF_CTRL);
	/*
	 * Deal with the banked PPI and SGI interrupts - disable all
	 * PPI interrupts, ensure all SGI interrupts are enabled.
	*/
	writel(0xffff0000, GIC_CLR_EN(0));
	writel(0x0000ffff, GIC_SET_EN(0));
	/* Set priority on PPI and SGI interrupts */
	for (i=0; i<16; i+=4)
	{
		writel(0xa0a0a0a0, GIC_SGI_PRIO(i>>2));
	}
	for (i=16; i<32; i+=4)
	{
		writel(0xa0a0a0a0, GIC_PPI_PRIO((i-16)>>2));
	}

	writel(0xf0, GIC_INT_PRIO_MASK);

	if(gd->securemode)
		writel(0xb, GIC_CPU_IF_CTRL);
	else
		writel(1, GIC_CPU_IF_CTRL);

	return ;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int arch_interrupt_init (void)
{
	int i;

	for (i=0; i<GIC_IRQ_NUM; i++)
	{
		sunxi_int_handlers[i].m_data = default_isr;
	}
	if((uboot_spare_head.boot_data.secureos_exist == SUNXI_SECURE_MODE_NO_SECUREOS) ||
		(uboot_spare_head.boot_data.secureos_exist == SUNXI_NORMAL_MODE))
	{
		printf("gic: normal or no secure os mode\n");
		gic_distributor_init();
		gic_cpuif_init();
	}
	return 0;
}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/
int arch_interrupt_exit(void)
{
	gic_distributor_init();
	gic_cpuif_init();

	return 0;
}

