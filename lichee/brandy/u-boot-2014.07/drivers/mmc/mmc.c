/*
 * Copyright 2008, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the Linux code
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <errno.h>
#include <mmc.h>
#include <part.h>
#include <malloc.h>
#include <linux/list.h>
#include <div64.h>
#include "mmc_private.h"
#include "mmc_def.h"


//static struct list_head mmc_devices;
static int cur_dev_num = -1;
int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd);
int mmc_decode_ext_csd(struct mmc *mmc,struct mmc_ext_csd *dec_ext_csd, char *ext_csd);
int mmc_do_switch(struct mmc *mmc, u8 set, u8 index, u8 value, u32 timeout);

extern int mmc_init_blk_ops(struct mmc *mmc);
extern unsigned int mmc_mmc_update_timeout(struct mmc *mmc);

LIST_HEAD(mmc_devices);

int __weak board_mmc_getwp(struct mmc *mmc)
{
	return -1;
}

int mmc_getwp(struct mmc *mmc)
{
	int wp;

	wp = board_mmc_getwp(mmc);

	if (wp < 0) {
		if (mmc->cfg->ops->getwp)
			wp = mmc->cfg->ops->getwp(mmc);
		else
			wp = 0;
	}

	return wp;
}

int __board_mmc_getcd(struct mmc *mmc) {
	return -1;
}

int board_mmc_getcd(struct mmc *mmc)__attribute__((weak,
	alias("__board_mmc_getcd")));

int mmc_send_cmd(struct mmc *mmc, struct mmc_cmd *cmd, struct mmc_data *data)
{
	int ret;

#ifdef CONFIG_MMC_TRACE
	int i;
	u8 *ptr;

	MMCDBG("CMD_SEND:%d\n", cmd->cmdidx);
	MMCDBG("\t\tARG\t\t\t 0x%08X\n", cmd->cmdarg);
	MMCDBG("\t\tFLAG\t\t\t %d\n", cmd->flags);
	ret = mmc->cfg->ops->send_cmd(mmc, cmd, data);
	switch (cmd->resp_type) {
		case MMC_RSP_NONE:
			MMCDBG("\t\tMMC_RSP_NONE\n");
			break;
		case MMC_RSP_R1:
			MMCDBG("\t\tMMC_RSP_R1,5,6,7 \t 0x%08X \n",
				cmd->response[0]);
			break;
		case MMC_RSP_R1b:
			MMCDBG("\t\tMMC_RSP_R1b\t\t 0x%08X \n",
				cmd->response[0]);
			break;
		case MMC_RSP_R2:
			MMCDBG("\t\tMMC_RSP_R2\t\t 0x%08X \n",
				cmd->response[0]);
			MMCDBG("\t\t          \t\t 0x%08X \n",
				cmd->response[1]);
			MMCDBG("\t\t          \t\t 0x%08X \n",
				cmd->response[2]);
			MMCDBG("\t\t          \t\t 0x%08X \n",
				cmd->response[3]);
			MMCDBG("\n");
			MMCDBG("\t\t\t\t\tDUMPING DATA\n");
			for (i = 0; i < 4; i++) {
				int j;
				MMCDBG("\t\t\t\t\t%03d - ", i*4);
				ptr = (u8 *)&cmd->response[i];
				ptr += 3;
				for (j = 0; j < 4; j++)
					MMCDBG("%02X ", *ptr--);
				MMCDBG("\n");
			}
			break;
		case MMC_RSP_R3:
			MMCDBG("\t\tMMC_RSP_R3,4\t\t 0x%08X \n",
				cmd->response[0]);
			break;
		default:
			MMCDBG("\t\tERROR MMC rsp not supported\n");
			break;
	}
#else
	ret = mmc->cfg->ops->send_cmd(mmc, cmd, data);
#endif
	return ret;
}

int mmc_send_status(struct mmc *mmc, int timeout)
{
	struct mmc_cmd cmd;
	int err, retries = 5;
#ifdef CONFIG_MMC_TRACE
	int status;
#endif

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	if (!mmc_host_is_spi(mmc))
		cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	do {
		err = mmc_send_cmd(mmc, &cmd, NULL);
		if (!err) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;
			else if (cmd.response[0] & MMC_STATUS_MASK) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
				MMCINFO("Status Error: 0x%08X\n",
					cmd.response[0]);
#endif
				return COMM_ERR;
			}
		} else if (--retries < 0)
			return err;

		udelay(1000);

	} while (timeout--);

#ifdef CONFIG_MMC_TRACE
	status = (cmd.response[0] & MMC_STATUS_CURR_STATE) >> 9;
	printf("CURR STATE:%d\n", status);
#endif
	if (timeout <= 0) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
		MMCINFO("Timeout waiting card ready\n");
#endif
		return TIMEOUT;
	}
	if (cmd.response[0] & MMC_STATUS_SWITCH_ERROR) {
		MMCINFO("Fail to switch to expected mode by SWITCH cmd\n");
		return SWITCH_ERR;
	}
	if (cmd.response[0] & MMC_STATUS_ADDR_OUT_OF_RANGE) {
		MMCINFO("Address out of range !!\n");
		return -1;
	}

	return 0;
}

int mmc_set_blocklen(struct mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	/*ddr mode not send blocklenth*/
	if(mmc->io_mode == MMC_MODE_DDR_52MHz ){
		return 0;
	}

/*
	if (mmc->card_caps & MMC_MODE_DDR_52MHz)
		return 0;
*/

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;
	cmd.flags = 0;

	return mmc_send_cmd(mmc, &cmd, NULL);
}


int mmc_send_manual_stop(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int ret = 0;
	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.flags = MMC_CMD_MANUAL; //let bsp send cmd12
	ret = mmc_send_cmd(mmc, &cmd, NULL);
	if (ret) {
		MMCINFO("mmc fail to send manual stop cmd\n");
		return ret;
	}
	return 0;
}

struct mmc *find_mmc_device(int dev_num)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		if (m->block_dev.dev == dev_num)
			return m;
	}

#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
	MMCINFO("MMC Device %d not found\n", dev_num);
#endif

	return NULL;
}

static int mmc_read_blocks(struct mmc *mmc, void *dst, lbaint_t start,
			   lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout = 1000;

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start * mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;
	cmd.flags = 0;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize = mmc->read_bl_len;
	data.flags = MMC_DATA_READ;

	if (mmc_send_cmd(mmc, &cmd, &data)) {
		MMCINFO(" read block failed\n");
		return 0;
	}
	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.flags = 0;
		if (mmc_send_cmd(mmc, &cmd, NULL)) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
			MMCINFO("mmc fail to send stop cmd\n");
#endif
			return 0;
		}

		/* Waiting for the ready status */
		mmc_send_status(mmc, timeout);
	}

	return blkcnt;
}

ulong mmc_bread(int dev_num, lbaint_t start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;
	struct mmc *mmc = find_mmc_device(dev_num);

	if (blkcnt == 0) {
		MMCINFO("blkcnt should not be 0\n");
		return 0;
	}
	if (!mmc) {
		MMCINFO("Can not find mmc dev\n");
		return 0;
	}

	if ((start + blkcnt) > mmc->block_dev.lba) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
		MMCINFO("MMC: block number 0x" LBAF " exceeds max(0x" LBAF ")\n",
			start + blkcnt, mmc->block_dev.lba);
#endif
		return 0;
	}

	if (mmc_set_blocklen(mmc, mmc->read_bl_len)) {
		MMCINFO("Set block len failed\n");
		return 0;
	}

	do {
		cur = (blocks_todo > mmc->cfg->b_max) ?
			mmc->cfg->b_max : blocks_todo;
		if(mmc_read_blocks(mmc, dst, start, cur) != cur) {
			MMCINFO("block read failed\n");
			return 0;
		}
		blocks_todo -= cur;
		start += cur;
		dst += cur * mmc->read_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}

static int mmc_go_idle(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	udelay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err) {
		MMCINFO("go idle failed\n");
		return err;
	}

	udelay(2000);

	return 0;
}

static int sd_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	do {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("send app cmd failed\n");
			return err;
		}

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc_host_is_spi(mmc) ? 0 :
			(mmc->cfg->voltages & 0xff8000);

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("send cmd41 failed\n");
			return err;
		}

		udelay(1000);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout < 0) {
		MMCINFO("wait card init failed\n");
		return UNUSABLE_ERR;
	}

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("spi read ocr failed\n");
			return err;
		}
	}

	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

/* We pass in the cmd since otherwise the init seems to fail */
static int mmc_send_op_cond_iter(struct mmc *mmc, struct mmc_cmd *cmd,
		int use_arg)
{
	int err;

	cmd->cmdidx = MMC_CMD_SEND_OP_COND;
	cmd->resp_type = MMC_RSP_R3;
 	cmd->cmdarg = 0x40ff8000;//foresee
 	cmd->flags = 0;
	if (use_arg && !mmc_host_is_spi(mmc)) {
		cmd->cmdarg =
			(mmc->cfg->voltages &
			(mmc->op_cond_response & OCR_VOLTAGE_MASK)) |
			(mmc->op_cond_response & OCR_ACCESS_MODE);

		if (mmc->cfg->host_caps & MMC_MODE_HC)
			cmd->cmdarg |= OCR_HCS;
	}
	err = mmc_send_cmd(mmc, cmd, NULL);
	if (err) {
		MMCINFO("read op condition failed\n");
		return err;
	}
	mmc->op_cond_response = cmd->response[0];
	return 0;
}

int mmc_send_op_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err, i;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

 	/* Asking to the card its capabilities */
	mmc->op_cond_pending = 1;
	for (i = 0; i < 2; i++) {
		err = mmc_send_op_cond_iter(mmc, &cmd, i != 0);
		if (err) {
			MMCINFO("mmc send op cond failed\n");
			return err;
		}

		/* exit if not busy (flag seems to be inverted) */
		if (mmc->op_cond_response & OCR_BUSY)
			return 0;
	}
	return IN_PROGRESS;
}

int mmc_complete_op_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int timeout = 1000;
	uint start;
	int err;

	mmc->op_cond_pending = 0;
	start = get_timer(0);
	do {
		err = mmc_send_op_cond_iter(mmc, &cmd, 1);
		if (err) {
			MMCINFO("mmc send op cond failed\n");
			return err;
		}
		if (get_timer(start) > timeout) {
			MMCINFO("wait for mmc init failed\n");
			return UNUSABLE_ERR;
		}
		udelay(100);
	} while (!(mmc->op_cond_response & OCR_BUSY));

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("spi read ocr failed\n");
			return err;
		}
	}

	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 1;

	return 0;
}


int mmc_send_ext_csd(struct mmc *mmc, char *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	data.dest = (char *)ext_csd;
	data.blocks = 1;
	data.blocksize = MMC_MAX_BLOCK_LEN;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);
	if(err)
		MMCINFO("mmc send ext csd failed\n");

	return err;
}

/* decode ext_csd */
int mmc_decode_ext_csd(struct mmc *mmc,
	struct mmc_ext_csd *dec_ext_csd, char *ext_csd)
{
	int err = 0;

	if ((!ext_csd) || !(dec_ext_csd))
		return 0;

	/* Version is coded in the CSD_STRUCTURE byte in the EXT_CSD register */
	dec_ext_csd->raw_ext_csd_structure = ext_csd[EXT_CSD_STRUCTURE];


	dec_ext_csd->rev = ext_csd[EXT_CSD_REV];
	if (dec_ext_csd->rev > 7) {
		MMCINFO("unrecognised EXT_CSD revision %d\n", dec_ext_csd->rev);
		err = -1;
		goto out;
	}

	dec_ext_csd->raw_sectors[0] = ext_csd[EXT_CSD_SEC_CNT + 0];
	dec_ext_csd->raw_sectors[1] = ext_csd[EXT_CSD_SEC_CNT + 1];
	dec_ext_csd->raw_sectors[2] = ext_csd[EXT_CSD_SEC_CNT + 2];
	dec_ext_csd->raw_sectors[3] = ext_csd[EXT_CSD_SEC_CNT + 3];
	if (dec_ext_csd->rev >= 2) {
		dec_ext_csd->sectors =
			ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
			ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
			ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
			ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
	}

	dec_ext_csd->raw_card_type = ext_csd[EXT_CSD_CARD_TYPE];

	dec_ext_csd->raw_erase_timeout_mult =
		ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];
	dec_ext_csd->raw_hc_erase_grp_size =
		ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
	if (dec_ext_csd->rev >= 3) {
		dec_ext_csd->erase_group_def =
			ext_csd[EXT_CSD_ERASE_GROUP_DEF];
		dec_ext_csd->hc_erase_timeout = 300 *
			ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];
		dec_ext_csd->hc_erase_size =
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] << 10;
	}

	dec_ext_csd->raw_hc_erase_gap_size =
		ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
	dec_ext_csd->raw_sec_trim_mult =
		ext_csd[EXT_CSD_SEC_TRIM_MULT];
	dec_ext_csd->raw_sec_erase_mult =
		ext_csd[EXT_CSD_SEC_ERASE_MULT];
	dec_ext_csd->raw_sec_feature_support =
		ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
	dec_ext_csd->raw_trim_mult =
		ext_csd[EXT_CSD_TRIM_MULT];

	if (dec_ext_csd->rev >= 4) {
		dec_ext_csd->sec_trim_mult =
			ext_csd[EXT_CSD_SEC_TRIM_MULT];
		dec_ext_csd->sec_erase_mult =
			ext_csd[EXT_CSD_SEC_ERASE_MULT];
		dec_ext_csd->sec_feature_support =
			ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
		dec_ext_csd->trim_timeout = 300 *
			ext_csd[EXT_CSD_TRIM_MULT];
	}

	dec_ext_csd->raw_erased_mem_count = ext_csd[EXT_CSD_ERASED_MEM_CONT];

	/* eMMC v4.5 or later */
	if (dec_ext_csd->rev >= 6) {
		dec_ext_csd->generic_cmd6_time = 10 *
			ext_csd[EXT_CSD_GENERIC_CMD6_TIME];
		dec_ext_csd->power_off_longtime = 10 *
			ext_csd[EXT_CSD_POWER_OFF_LONG_TIME];

	} else {
		dec_ext_csd->data_sector_size = 512;
	}

out:
	return err;
}

int mmc_do_switch(struct mmc *mmc, u8 set, u8 index, u8 value, u32 timeout)
{
	struct mmc_cmd cmd;
	int ret;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
				 (index << 16) |
				 (value << 8);
	cmd.flags = 0;

	ret = mmc_send_cmd(mmc, &cmd, NULL);
	if(ret){
		MMCINFO("mmc switch failed\n");
	}

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	return ret;
}

int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value)
{
	int timeout = 1000;
	return mmc_do_switch(mmc, set, index, value, timeout);
}

static int mmc_change_freq(struct mmc *mmc)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd, MMC_MAX_BLOCK_LEN);
	char cardtype;
	int err;
	int retry = 5;

	mmc->card_caps = 0;

	if (mmc_host_is_spi(mmc))
		return 0;

	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4)
		return 0;

	/* --??-- here we assume eMMC support 8 bit */
	mmc->card_caps |= MMC_MODE_4BIT|MMC_MODE_8BIT;

	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err) {
		MMCINFO("mmc get ext csd failed\n");
		return err;
	}

	cardtype = ext_csd[EXT_CSD_CARD_TYPE] & 0xf;

	/* --??-- retry for Toshiba emmc;for the first time Toshiba emmc change to HS
	 it will return response crc err,so retry */
	do{
		err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
		if(!err){
			break;
		}
		MMCINFO("retry mmc switch(cmd6)\n");
	} while(retry--);
	if (err) {
		MMCINFO("mmc change to hs failed\n");
		return err; //return err == SWITCH_ERR ? 0 : err;
	}

	/* Now check to see that it worked */
	err = mmc_send_ext_csd(mmc, ext_csd);

	if (err) {
		MMCINFO("send ext csd faild\n");
		return err;
	}

	/* No high-speed support */
	if (!ext_csd[EXT_CSD_HS_TIMING]) {
		MMCDBG("don't support hign speed mode\n");
		return 0;
	}

	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & EXT_CSD_CARD_TYPE_52) {
		if (cardtype & EXT_CSD_CARD_TYPE_DDR_52) {
			MMCDBG("get ddr OK!\n");
			mmc->card_caps |= MMC_MODE_DDR_52MHz;
		}
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
	} else {
		mmc->card_caps |= MMC_MODE_HS;
	}

	return 0;
}

static int mmc_set_capacity(struct mmc *mmc, int part_num)
{
	switch (part_num) {
	case 0:
		mmc->capacity = mmc->capacity_user;
		break;
	case 1:
	case 2:
		mmc->capacity = mmc->capacity_boot;
		break;
	case 3:
		mmc->capacity = mmc->capacity_rpmb;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		mmc->capacity = mmc->capacity_gp[part_num - 4];
		break;
	default:
		return -1;
	}

	mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);

	return 0;
}

int mmc_select_hwpart(int dev_num, int hwpart)
{
	struct mmc *mmc = find_mmc_device(dev_num);
	int ret;

	if (!mmc)
		return -ENODEV;

	if (mmc->part_num == hwpart)
		return 0;

	if (mmc->part_config == MMCPART_NOAVAILABLE) {
		printf("Card doesn't support part_switch\n");
		return -EMEDIUMTYPE;
	}

	ret = mmc_switch_part(dev_num, hwpart);
	if (ret)
		return ret;

	mmc->part_num = hwpart;

	return 0;
}


int mmc_switch_part(int dev_num, unsigned int part_num)
{
	char ext_csd[512]={0};
	unsigned char part_config = 0;
	struct mmc *mmc = find_mmc_device(dev_num);
	int ret;

	MMCDBG("Try to switch part \n");
	if (!mmc) {
		MMCINFO("can not find mmc device\n");
		return -1;
	}

	ret = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
			 (mmc->part_config & ~PART_ACCESS_MASK)
			 | (part_num & PART_ACCESS_MASK));
	if (ret) {
		MMCINFO("mmc switch part failed\n");
		return -1; //ret;
	}

	ret = mmc_send_ext_csd(mmc, ext_csd);
	if(ret) {
		MMCINFO("send ext csd failed\n");
		return -1;
	}
	MMCDBG("part conf:0x%x\n",ext_csd[EXT_CSD_PART_CONF]);
	if (part_config!=ext_csd[EXT_CSD_PART_CONF]) {
		MMCINFO("switch boot part failed,now bus con is 0x%x\n",ext_csd[EXT_CSD_PART_CONF]);
		return -1;
	}
	mmc->part_config = part_config;

	MMCDBG("switch part succeed\n");

	return mmc_set_capacity(mmc, part_num);
}

int mmc_getcd(struct mmc *mmc)
{
	int cd;

	cd = board_mmc_getcd(mmc);

	if (cd < 0) {
		if (mmc->cfg->ops->getcd)
			cd = mmc->cfg->ops->getcd(mmc);
		else
			cd = 1;
	}

	return cd;
}

static int sd_switch(struct mmc *mmc, int mode, int group, u8 value, u8 *resp)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = (mode << 31) | 0xffffff;
	cmd.cmdarg &= ~(0xf << (group * 4));
	cmd.cmdarg |= value << (group * 4);
	cmd.flags = 0;

	data.dest = (char *)resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	return mmc_send_cmd(mmc, &cmd, &data);
}


static int sd_change_freq(struct mmc *mmc)
{
	int err;
	struct mmc_cmd cmd;
	ALLOC_CACHE_ALIGN_BUFFER(uint, scr, 2);
	ALLOC_CACHE_ALIGN_BUFFER(uint, switch_status, 16);
	struct mmc_data data;
	int timeout;

	mmc->card_caps = 0;

	if (mmc_host_is_spi(mmc))
		return 0;

	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err) {
		MMCINFO("send app cmd failed\n");
		return err;
	}

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	timeout = 3;

retry_scr:
	data.dest = (char *)scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	if (err) {
		if (timeout--)
			goto retry_scr;

		MMCINFO("send scr failed\n");
		return err;
	}

	mmc->scr[0] = __be32_to_cpu(scr[0]);
	mmc->scr[1] = __be32_to_cpu(scr[1]);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			if ((mmc->scr[0] >> 15) & 0x1)
				mmc->version = SD_VERSION_3;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}

	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;
	while (timeout--) {
		err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
				(u8 *)switch_status);

		if (err) {
			MMCINFO("check high speed status faild\n");
			return err;
		}

		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}

	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	/*
	 * If the host doesn't support SD_HIGHSPEED, do not switch card to
	 * HIGHSPEED mode even if the card support SD_HIGHSPPED.
	 * This can avoid furthur problem when the card runs in different
	 * mode between the host.
	 */
	if (!((mmc->cfg->host_caps & MMC_MODE_HS_52MHz) &&
		(mmc->cfg->host_caps & MMC_MODE_HS)))
		return 0;

	err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (u8 *)switch_status);

	if (err) {
		MMCINFO("switch to high speed failed\n");
		return err;
	}

	if ((__be32_to_cpu(switch_status[4]) & 0x0f000000) == 0x01000000)
		mmc->card_caps |= MMC_MODE_HS;

	return 0;
}

/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};

static void mmc_set_ios(struct mmc *mmc)
{
	if (mmc->cfg->ops->set_ios)
		mmc->cfg->ops->set_ios(mmc);
}

void mmc_set_clock(struct mmc *mmc, uint clock)
{
	if (clock > mmc->cfg->f_max)
		clock = mmc->cfg->f_max;

	if (clock < mmc->cfg->f_min)
		clock = mmc->cfg->f_min;

	mmc->clock = clock;

	mmc_set_ios(mmc);
}

static void mmc_set_bus_width(struct mmc *mmc, uint width)
{
	mmc->bus_width = width;

	mmc_set_ios(mmc);
}

static int mmc_startup(struct mmc *mmc)
{
	int err, i;
	uint mult, freq;
	u64 cmult, csize, capacity;
	struct mmc_cmd cmd;
	ALLOC_CACHE_ALIGN_BUFFER(char, ext_csd, MMC_MAX_BLOCK_LEN);
	//ALLOC_CACHE_ALIGN_BUFFER(u8, test_csd, MMC_MAX_BLOCK_LEN);
	int timeout = 1000;
	int erase_gsz, erase_gmul;
	int def_erase_grp_size, hc_erase_gpr_size;
	int hc_erase_timeout;

#ifdef CONFIG_MMC_SPI_CRC_ON
	if (mmc_host_is_spi(mmc)) { /* enable CRC check for spi */
		cmd.cmdidx = MMC_CMD_SPI_CRC_ON_OFF;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 1;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("spi crc on off failed\n");
			return err;
		}
	}
#endif

	/* Put the Card in Identify Mode */
	cmd.cmdidx = mmc_host_is_spi(mmc) ? MMC_CMD_SEND_CID :
		MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err) {
		MMCINFO("Put the Card in Identify Mode failed\n");
		return err;
	}

	memcpy(mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;
		cmd.flags = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("send rca failed\n");
			return err;
		}

		if (IS_SD(mmc))
			mmc->rca = (cmd.response[0] >> 16) & 0xffff;
	}

	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	if (err) {
		MMCINFO("get csd failed\n");
		return err;
	}

	mmc->csd[0] = cmd.response[0];
	mmc->csd[1] = cmd.response[1];
	mmc->csd[2] = cmd.response[2];
	mmc->csd[3] = cmd.response[3];

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				break;
		}
	}

	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->tran_speed = freq * mult;

	mmc->dsr_imp = ((cmd.response[1] >> 12) & 0x1);
	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);

	if (IS_SD(mmc))
		mmc->write_bl_len = mmc->read_bl_len;
	else
		mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if (mmc->high_capacity) {
		csize = (mmc->csd[1] & 0x3f) << 16
			| (mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = (mmc->csd[1] & 0x3ff) << 2
			| (mmc->csd[2] & 0xc0000000) >> 30;
		cmult = (mmc->csd[2] & 0x00038000) >> 15;
	}

	mmc->capacity_user = (csize + 1) << (cmult + 2);
	mmc->capacity_user *= mmc->read_bl_len;
	mmc->capacity_boot = 0;
	mmc->capacity_rpmb = 0;
	for (i = 0; i < 4; i++)
		mmc->capacity_gp[i] = 0;

	if (mmc->read_bl_len > MMC_MAX_BLOCK_LEN)
		mmc->read_bl_len = MMC_MAX_BLOCK_LEN;

	if (mmc->write_bl_len > MMC_MAX_BLOCK_LEN)
		mmc->write_bl_len = MMC_MAX_BLOCK_LEN;

	if ((mmc->dsr_imp) && (0xffffffff != mmc->dsr)) {
		cmd.cmdidx = MMC_CMD_SET_DSR;
		cmd.cmdarg = (mmc->dsr & 0xffff) << 16;
		cmd.resp_type = MMC_RSP_NONE;
		if (mmc_send_cmd(mmc, &cmd, NULL))
			printf("MMC: SET_DSR failed\n");
	}

	/* Select the card, and put it into Transfer Mode */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1b;
		cmd.cmdarg = mmc->rca << 16;
		cmd.flags = 0;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			MMCINFO("Select the card failed\n");
			return err;
		}
	}

	/*
	 * For SD, its erase group is always one sector
	 */
	mmc->erase_grp_size = 1;
	mmc->part_config = MMCPART_NOAVAILABLE;
	if (!IS_SD(mmc) && (mmc->version >= MMC_VERSION_4)) {
		/* check  ext_csd version and capacity */
		err = mmc_send_ext_csd(mmc, ext_csd);
		if (!err && (ext_csd[EXT_CSD_REV] >= 2)) {
			/*
			 * According to the JEDEC Standard, the value of
			 * ext_csd's capacity is valid if the value is more
			 * than 2GB
			 */
			capacity = ext_csd[EXT_CSD_SEC_CNT] << 0
					| ext_csd[EXT_CSD_SEC_CNT + 1] << 8
					| ext_csd[EXT_CSD_SEC_CNT + 2] << 16
					| ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
			capacity *= MMC_MAX_BLOCK_LEN;
			if ((capacity >> 20) > 2 * 1024)
				mmc->capacity_user = capacity;
		}

		switch (ext_csd[EXT_CSD_REV]) {
		case 0:
			mmc->version = MMC_VERSION_4;
			break;
		case 1:
			mmc->version = MMC_VERSION_4_1;
			break;
		case 2:
			mmc->version = MMC_VERSION_4_2;
			break;
		case 3:
			mmc->version = MMC_VERSION_4_3;
			break;
		case 5:
			mmc->version = MMC_VERSION_4_41;
			break;
		case 6:
			mmc->version = MMC_VERSION_4_5;
			break;
		case 7:
			mmc->version = MMC_VERSION_5_0;
			break;
		default:
			MMCINFO("Invalid ext_csd revision %d\n", ext_csd[192]);
			break;
		}


		/*
		  * Get timeout value
		  */
		mmc_mmc_update_timeout(mmc);

		/*
		 * Check whether GROUP_DEF is set, if yes, read out
		 * group size from ext_csd directly, or calculate
		 * the group size from the csd value.
		 */
		erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
		erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
		def_erase_grp_size = (erase_gsz + 1) * (erase_gmul + 1);

		hc_erase_gpr_size = ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * MMC_MAX_BLOCK_LEN * 1024;
		hc_erase_gpr_size = hc_erase_gpr_size / mmc->write_bl_len;

		hc_erase_timeout = 300 * ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];

		if (ext_csd[EXT_CSD_ERASE_GROUP_DEF] && hc_erase_gpr_size && hc_erase_timeout)
			mmc->erase_grp_size = hc_erase_gpr_size;
		else
			mmc->erase_grp_size = def_erase_grp_size;

		/*
		 * Host needs to enable ERASE_GRP_DEF bit if device is
		 * partitioned. This bit will be lost every time after a reset
		 * or power off. This will affect erase size.
		 */
		if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) &&
			(ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE] & PART_ENH_ATTRIB)) {
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_ERASE_GROUP_DEF, 1);

			if (err)
				return err;

			mmc->erase_grp_size = hc_erase_gpr_size;
		}

		mmc->secure_feature = ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
		mmc->secure_removal_type = ext_csd[EXT_CSD_SECURE_REMOAL_TYPE];

		/* store the partition info of emmc */
		if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) ||
			ext_csd[EXT_CSD_BOOT_MULT]) {
			mmc->part_config = ext_csd[EXT_CSD_PART_CONF];

			mmc->capacity_boot = ext_csd[EXT_CSD_BOOT_MULT] << 17;

			mmc->capacity_rpmb = ext_csd[EXT_CSD_RPMB_MULT] << 17;

			if (mmc->capacity_boot) {
				mmc->boot_support = 1;
				mmc->boot_bus_cond = ext_csd[EXT_CSD_BOOT_BUS_WIDTH];
			} else {
				MMCDBG("not PART_SUPPORT ext_csd[226] = %d\n",ext_csd[226]);
			}

			for (i = 0; i < 4; i++) {
				int idx = EXT_CSD_GP_SIZE_MULT + i * 3;
				mmc->capacity_gp[i] = (ext_csd[idx + 2] << 16) +
					(ext_csd[idx + 1] << 8) + ext_csd[idx];
				mmc->capacity_gp[i] *=
					ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
				mmc->capacity_gp[i] *= ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
			}
		}
	}

	err = mmc_set_capacity(mmc, mmc->part_num);
	if (err) {
		MMCINFO("%s: set capcacity error\n", __FUNCTION__);
		return err;
	}

	/* ???? --WJQ */
	mmc_set_clock(mmc, 25000000);

	if (IS_SD(mmc))
		err = sd_change_freq(mmc);
	else
		err = mmc_change_freq(mmc);

	if (err) {
		MMCINFO("change speed mode failed\n");
		return err;
	}

	/* Restrict card's capabilities by what the host can do */
	mmc->card_caps &= mmc->cfg->host_caps;
	tick_printf("---%s %d %s: 0x%x 0x%x\n", __FILE__, __LINE__, __FUNCTION__, mmc->card_caps, mmc->cfg->host_caps);

	if (IS_SD(mmc)) {
		if (mmc->card_caps & MMC_MODE_4BIT) {
			cmd.cmdidx = MMC_CMD_APP_CMD;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = mmc->rca << 16;
			cmd.flags = 0;

			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err) {
				MMCINFO("send app cmd failed\n");
				return err;
			}

			cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
			cmd.resp_type = MMC_RSP_R1;
			cmd.cmdarg = 2;
			cmd.flags = 0;
			err = mmc_send_cmd(mmc, &cmd, NULL);
			if (err){
				MMCINFO("sd set bus width failed\n");
				return err;
			}

			mmc_set_bus_width(mmc, 4);
		}

		if (mmc->card_caps & MMC_MODE_HS)
			mmc->tran_speed = 50000000;
		else
			mmc->tran_speed = 25000000;

	} else {
		if (mmc->card_caps & MMC_MODE_8BIT) {
			if (mmc->card_caps & MMC_MODE_DDR_52MHz){
				MMCINFO("mmc 8bit bus ddr!!!!!! \n");
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
						EXT_CSD_BUS_WIDTH,
						EXT_CSD_BUS_DDR_8);
				if (err) {
					MMCINFO("mmc switch bus width to ddr8 failed\n");
					return err;
				}
				mmc_set_bus_width(mmc, 8);
			} else {
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
						EXT_CSD_BUS_WIDTH,
						EXT_CSD_BUS_WIDTH_8);

				if (err){
					MMCINFO("mmc switch bus width8 failed\n");
					return err;
				}
				mmc_set_bus_width(mmc, 8);
			}
		}else if (mmc->card_caps & MMC_MODE_4BIT) {
			if (mmc->card_caps & MMC_MODE_DDR_52MHz ) {

				MMCINFO("mmc 4 bit bus ddr!!!!!! \n");
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
						EXT_CSD_BUS_WIDTH,
						EXT_CSD_BUS_DDR_4);
				if (err) {
					MMCINFO("mmc switch bus width to ddr4 failed\n");
					return err;
				}
				mmc_set_bus_width(mmc, 4);
			} else {
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
						EXT_CSD_BUS_WIDTH,
						EXT_CSD_BUS_WIDTH_4);
				if (err){
					MMCINFO("mmc switch bus width failed\n");
					return err;
				}
				mmc_set_bus_width(mmc, 4);
			}
		}

		if (mmc->card_caps & MMC_MODE_DDR_52MHz) {
			mmc->io_mode = MMC_MODE_DDR_52MHz;
			MMCDBG("set mmc io_mode to ddr mode, %x\n",mmc->io_mode);
			mmc->tran_speed = 52000000;

		} else if (mmc->card_caps & MMC_MODE_HS) {
			if (mmc->card_caps & MMC_MODE_HS_52MHz)
				mmc->tran_speed = 52000000;
			else
				mmc->tran_speed = 26000000;
		} else {
			mmc->tran_speed = 26000000;
		}
	}

	mmc_set_clock(mmc, mmc->tran_speed);

	/* fill in device description */
	mmc->block_dev.lun = 0;
	mmc->block_dev.type = 0;
	mmc->block_dev.blksz = mmc->read_bl_len;
	mmc->block_dev.log2blksz = LOG2(mmc->block_dev.blksz);
	mmc->block_dev.lba = lldiv(mmc->capacity, mmc->read_bl_len);
	if (IS_SD(mmc)){
		sprintf(mmc->block_dev.vendor, "MID %06x PSN %08x",
			mmc->cid[0] >> 24, (mmc->cid[2] << 8) | (mmc->cid[3] >> 24));
		sprintf(mmc->block_dev.product, "PNM %c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
		sprintf(mmc->block_dev.revision, "PRV %d.%d", mmc->cid[2] >> 28,
			(mmc->cid[2] >> 24) & 0xf);
	} else {
		sprintf(mmc->block_dev.vendor, "MID %06x PSN %04x%04x",
			mmc->cid[0] >> 24, (mmc->cid[2] & 0xffff),
			(mmc->cid[3] >> 16) & 0xffff);
		sprintf(mmc->block_dev.product, "PNM %c%c%c%c%c%c", mmc->cid[0] & 0xff,
			(mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff,
			(mmc->cid[2] >> 24) & 0xff);
		sprintf(mmc->block_dev.revision, "PRV %d.%d", (mmc->cid[2] >> 20) & 0xf,
			(mmc->cid[2] >> 16) & 0xf);
	}

	MMCINFO("%s\n", mmc->block_dev.vendor);
	MMCINFO("%s -- 0x%02x-%02x-%02x-%02x-%02x-%02x\n", mmc->block_dev.product,
			mmc->cid[0] & 0xff, (mmc->cid[1] >> 24), (mmc->cid[1] >> 16) & 0xff,
			(mmc->cid[1] >> 8) & 0xff, mmc->cid[1] & 0xff);
	MMCINFO("%s\n", mmc->block_dev.revision);

	if (IS_SD(mmc)) {
		MMCINFO("MDT m-%d y-%d\n", ((mmc->cid[3] >> 8) & 0xF), (((mmc->cid[3] >> 12) & 0xFF) + 2000));
	} else {
		if (ext_csd[192] > 4) {
			MMCINFO("MDT m-%d y-%d\n", ((mmc->cid[3] >> 12) & 0xF),
				(((mmc->cid[3] >> 8) & 0xF) < 13) ? (((mmc->cid[3] >> 8) & 0xF) + 2013) : (((mmc->cid[3] >> 8) & 0xF) + 1997));
		} else {
			MMCINFO("MDT m-%d y-%d\n", ((mmc->cid[3] >> 12) & 0xF), (((mmc->cid[3] >> 8) & 0xF) + 1997));
		}
	}

	if(!IS_SD(mmc)){
		switch(mmc->version)
		{
			case MMC_VERSION_1_2:
				MMCINFO("MMC ver 1.2\n");
				break;
			case MMC_VERSION_1_4:
				MMCINFO("MMC ver 1.4\n");
				break;
			case MMC_VERSION_2_2:
				MMCINFO("MMC ver 2.2\n");
				break;
			case MMC_VERSION_3:
				MMCINFO("MMC ver 3.0\n");
				break;
			case MMC_VERSION_4:
				MMCINFO("MMC ver 4.0\n");
				break;
			case MMC_VERSION_4_1:
				MMCINFO("MMC ver 4.1\n");
				break;
			case MMC_VERSION_4_2:
				MMCINFO("MMC ver 4.2\n");
				break;
			case MMC_VERSION_4_3:
				MMCINFO("MMC ver 4.3\n");
				break;
			case MMC_VERSION_4_41:
				MMCINFO("MMC ver 4.41\n");
				break;
			case MMC_VERSION_4_5:
				MMCINFO("MMC ver 4.5\n");
				break;
			case MMC_VERSION_5_0:
				MMCINFO("MMC ver 5.0\n");
				break;
			default:
				MMCINFO("Unknow MMC ver\n");
				break;
		}
	}

	mmc->clock_after_init = mmc->clock; //back up clock after mmc init

	MMCINFO("clock          : %d Hz\n", mmc->clock);
	MMCINFO("bus_width      : %d bit\n", mmc->bus_width);
	MMCINFO("user capacity  : %ld MB\n", mmc->block_dev.lba>>11);
	MMCINFO("boot capacity  : %d KB\n", mmc->capacity_boot>>10);
	MMCINFO("************SD/MMC %d init OK!!!************\n", mmc->cfg->host_no);

	return 0;
}

static int mmc_send_if_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->cfg->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;
	cmd.flags = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err) {
		MMCINFO("mmc send if cond failed\n");
		return err;
	}

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}



/* not used any more */
int __deprecated mmc_register(struct mmc *mmc)
{
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
	printf("%s is deprecated! use mmc_create() instead.\n", __func__);
#endif
	return -1;
}

struct mmc *mmc_create(const struct mmc_config *cfg, void *priv)
{
	struct mmc *mmc;

	/* quick validation */
	if (cfg == NULL || cfg->ops == NULL || cfg->ops->send_cmd == NULL ||
			cfg->f_min == 0 || cfg->f_max == 0 || cfg->b_max == 0) {
		tick_printf("---error, %s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	mmc = calloc(1, sizeof(*mmc));
	if (mmc == NULL) {
		tick_printf("---error, %s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	mmc->cfg = cfg;
	mmc->priv = priv;

	/* the following chunk was mmc_register() */

	/* Setup dsr related values */
	mmc->dsr_imp = 0;
	mmc->dsr = 0xffffffff;

	/* Setup the universal parts of the block interface just once */
	mmc->block_dev.if_type = IF_TYPE_MMC;
	mmc->block_dev.dev = mmc->cfg->host_no; //cur_dev_num++;
	mmc->block_dev.removable = 1;

	//mmc_init_blk_ops();
	mmc->block_dev.block_read = mmc_bread;
	mmc->block_dev.block_write = mmc_bwrite;
	mmc->block_dev.block_erase = mmc_berase;

	/* setup initial part type */
	mmc->block_dev.part_type = mmc->cfg->part_type;

	INIT_LIST_HEAD(&mmc->link);

	list_add_tail(&mmc->link, &mmc_devices);

	return mmc;
}

void mmc_destroy(struct mmc *mmc)
{
	/* only freeing memory for now */
	free(mmc);
}

#ifdef CONFIG_PARTITIONS
block_dev_desc_t *mmc_get_dev(int dev)
{
	struct mmc *mmc = find_mmc_device(dev);
	if (!mmc || mmc_init(mmc))
		return NULL;

	return &mmc->block_dev;
}
#endif

int mmc_start_init(struct mmc *mmc)
{
	int err;

	/* we pretend there's no card when init is NULL */
	if (mmc_getcd(mmc) == 0 || mmc->cfg->ops->init == NULL) {
		mmc->has_init = 0;
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
		printf("MMC: no card present\n");
#endif
		return NO_CARD_ERR;
	}

	if (mmc->has_init) {
		MMCINFO("Has init\n");
		return 0;
	}

	/* made sure it's not NULL earlier */
	err = mmc->cfg->ops->init(mmc);

	if (err) {
		tick_printf("---%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
		MMCINFO("mmc->init error\n");
		return err;
	}

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err) {
		tick_printf("---%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
		MMCINFO("mmc go idle error\n");
		return err;
	}

	/* The internal partition reset to user partition(0) at every CMD0*/
	mmc->part_num = 0;

	MMCINFO("************Try SD card %d************\n", mmc->cfg->host_no);
	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);

	/* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc);

	tick_printf("---%s %d %s, err: %d\n", __FILE__, __LINE__, __FUNCTION__, err);
	MMCINFO("************Try MMC card %d************\n", mmc->cfg->host_no);
	/* If the command timed out, we check for an MMC card */
	if (err == -1) { //if (err == TIMEOUT) {
		err = mmc_send_op_cond(mmc);
		tick_printf("---%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
		if (err && err != IN_PROGRESS) {
#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)
			printf("Card did not respond to voltage select!\n");
#endif
			MMCINFO("************SD/MMC %d init error!!!************\n", mmc->cfg->host_no);
			return UNUSABLE_ERR;
		}
	}

	if (err == IN_PROGRESS)
		mmc->init_in_progress = 1;

	return err;
}

static int mmc_complete_init(struct mmc *mmc)
{
	int err = 0;

	if (mmc->op_cond_pending)
		err = mmc_complete_op_cond(mmc);

	if (!err)
		err = mmc_startup(mmc);
	if (err) {
		MMCINFO("************SD/MMC %d init error!!!************\n", mmc->cfg->host_no);
		mmc->has_init = 0;
	} else {
		mmc->has_init = 1;
		tick_printf("startup ok\n");
	}
	mmc->init_in_progress = 0;

	init_part(&mmc->block_dev);

	return err;
}

int mmc_init(struct mmc *mmc)
{
	int err = IN_PROGRESS;
	unsigned start;

	if (mmc->has_init) {
		tick_printf("---%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
		return 0;
	}

	start = get_timer(0);

	if (!mmc->init_in_progress)
		err = mmc_start_init(mmc);

	if (!err || err == IN_PROGRESS)
		err = mmc_complete_init(mmc);

	debug("%s: %d, time %lu\n", __func__, err, get_timer(start));
	tick_printf("---%s %d %s  err: %d\n", __FILE__, __LINE__, __FUNCTION__, err);
	return err;
}

int mmc_set_dsr(struct mmc *mmc, u16 val)
{
	mmc->dsr = val;
	return 0;
}

/*
 * CPU and board-specific MMC initializations.  Aliased function
 * signals caller to move on
 */
static int __def_mmc_init(bd_t *bis)
{
	return -1;
}

int cpu_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));
int board_mmc_init(bd_t *bis) __attribute__((weak, alias("__def_mmc_init")));

#if !defined(CONFIG_SPL_BUILD) || defined(CONFIG_SPL_LIBCOMMON_SUPPORT)

void print_mmc_devices(char separator)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		printf("%s: %d", m->cfg->name, m->block_dev.dev);

		if (entry->next != &mmc_devices)
			printf("%c ", separator);
	}

	printf("\n");
}

#else
void print_mmc_devices(char separator) { }
#endif

int get_mmc_num(void)
{
	return cur_dev_num;
}

void mmc_set_preinit(struct mmc *mmc, int preinit)
{
	mmc->preinit = preinit;
}

static void do_preinit(void)
{
	struct mmc *m;
	struct list_head *entry;

	list_for_each(entry, &mmc_devices) {
		m = list_entry(entry, struct mmc, link);

		if (m->preinit)
			mmc_start_init(m);
	}
}


int mmc_initialize(bd_t *bis)
{
	INIT_LIST_HEAD (&mmc_devices);
	cur_dev_num = 0;

	if (board_mmc_init(bis) < 0)
		cpu_mmc_init(bis);

#ifndef CONFIG_SPL_BUILD
	print_mmc_devices(',');
#endif

	do_preinit();
	return 0;
}

int mmc_exit(void)
{
	int err;
	int sdc_no = 2;
	struct mmc *mmc = find_mmc_device(sdc_no);

	if (mmc == NULL) {
		MMCINFO("mmc %s not find, so not exit\n", sdc_no);
		return -1;
	}

	MMCINFO("mmc exit start\n");

	err = mmc->cfg->ops->init(mmc);
	if (err){
		MMCINFO("mmc->init error\n");
		MMCINFO("mmc %d exit failed\n", mmc->cfg->host_no);
		return err;
	}
	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

	/* Reset the Card */
	err = mmc_go_idle(mmc);
	if (err){
		MMCINFO("mmc go idle error\n");
		MMCINFO("mmc %d exit failed\n", mmc->cfg->host_no);
		return err;
	}

	/* The internal partition reset to user partition(0) at every CMD0*/
	mmc->part_num = 0;

	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);

    /* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc);

	/* If the command timed out, we check for an MMC card */
	if (err == -1){
		err = mmc_send_op_cond(mmc);

		if (err) {
			MMCINFO("Card did not respond to voltage select!\n");
			MMCINFO("mmc %d exit failed\n", mmc->cfg->host_no);
			return UNUSABLE_ERR;
		}
	}

	MMCINFO("mmc %d exit ok\n", mmc->cfg->host_no);
	return err;
}


#ifdef CONFIG_SUPPORT_EMMC_BOOT
/*
 * This function changes the size of boot partition and the size of rpmb
 * partition present on EMMC devices.
 *
 * Input Parameters:
 * struct *mmc: pointer for the mmc device strcuture
 * bootsize: size of boot partition
 * rpmbsize: size of rpmb partition
 *
 * Returns 0 on success.
 */

int mmc_boot_partition_size_change(struct mmc *mmc, unsigned long bootsize,
				unsigned long rpmbsize)
{
	int err;
	struct mmc_cmd cmd;

	/* Only use this command for raw EMMC moviNAND. Enter backdoor mode */
	cmd.cmdidx = MMC_CMD_RES_MAN;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = MMC_CMD62_ARG1;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err) {
		debug("mmc_boot_partition_size_change: Error1 = %d\n", err);
		return err;
	}

	/* Boot partition changing mode */
	cmd.cmdidx = MMC_CMD_RES_MAN;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = MMC_CMD62_ARG2;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err) {
		debug("mmc_boot_partition_size_change: Error2 = %d\n", err);
		return err;
	}
	/* boot partition size is multiple of 128KB */
	bootsize = (bootsize * 1024) / 128;

	/* Arg: boot partition size */
	cmd.cmdidx = MMC_CMD_RES_MAN;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = bootsize;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err) {
		debug("mmc_boot_partition_size_change: Error3 = %d\n", err);
		return err;
	}
	/* RPMB partition size is multiple of 128KB */
	rpmbsize = (rpmbsize * 1024) / 128;
	/* Arg: RPMB partition size */
	cmd.cmdidx = MMC_CMD_RES_MAN;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = rpmbsize;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err) {
		debug("mmc_boot_partition_size_change: Error4 = %d\n", err);
		return err;
	}
	return 0;
}

/*
 * Modify EXT_CSD[177] which is BOOT_BUS_WIDTH
 * based on the passed in values for BOOT_BUS_WIDTH, RESET_BOOT_BUS_WIDTH
 * and BOOT_MODE.
 *
 * Returns 0 on success.
 */
int mmc_set_boot_bus_width(struct mmc *mmc, u8 width, u8 reset, u8 mode)
{
	int err;

	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BOOT_BUS_WIDTH,
			 EXT_CSD_BOOT_BUS_WIDTH_MODE(mode) |
			 EXT_CSD_BOOT_BUS_WIDTH_RESET(reset) |
			 EXT_CSD_BOOT_BUS_WIDTH_WIDTH(width));

	if (err)
		return err;
	return 0;
}

/*
 * Modify EXT_CSD[179] which is PARTITION_CONFIG (formerly BOOT_CONFIG)
 * based on the passed in values for BOOT_ACK, BOOT_PARTITION_ENABLE and
 * PARTITION_ACCESS.
 *
 * Returns 0 on success.
 */
int mmc_set_part_conf(struct mmc *mmc, u8 ack, u8 part_num, u8 access)
{
	int err;

	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONF,
			 EXT_CSD_BOOT_ACK(ack) |
			 EXT_CSD_BOOT_PART_NUM(part_num) |
			 EXT_CSD_PARTITION_ACCESS(access));

	if (err)
		return err;
	return 0;
}

/*
 * Modify EXT_CSD[162] which is RST_n_FUNCTION based on the given value
 * for enable.  Note that this is a write-once field for non-zero values.
 *
 * Returns 0 on success.
 */
int mmc_set_rst_n_function(struct mmc *mmc, u8 enable)
{
	return mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_RST_N_FUNCTION,
			  enable);
}
#endif

int mmc_write_info(int dev_num,void *buffer,u32 buffer_size)
{
	struct mmc *mmc = find_mmc_device(dev_num);
	int work_mode = uboot_spare_head.boot_data.work_mode;
	if(mmc==NULL){
		MMCINFO("Can not find mmc\n");
		return -1;
	}

	if(mmc->sample_mode ==AUTO_SAMPLE_MODE
		&& work_mode!= WORK_MODE_BOOT){

		if(sizeof(struct tuning_sdly)>buffer_size){
			MMCINFO("size of tuning_sdly over %d\n",buffer_size);
			return -1;
		}

		memcpy(buffer,(void *)&mmc->sdly_tuning,sizeof(sizeof(struct tuning_sdly)));
		MMCINFO("write mmc info ok\n");
		return 0;
	}
	return -1;

}

