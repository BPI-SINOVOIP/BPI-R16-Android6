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


#ifndef __SW_SYS_CONFIG_H
#define __SW_SYS_CONFIG_H


#define   SCRIPT_PARSER_OK                   (0)
#define   SCRIPT_PARSER_EMPTY_BUFFER         (-1)
#define   SCRIPT_PARSER_KEYNAME_NULL         (-2)
#define   SCRIPT_PARSER_DATA_VALUE_NULL      (-3)
#define   SCRIPT_PARSER_KEY_NOT_FIND         (-4)
#define   SCRIPT_PARSER_BUFFER_NOT_ENOUGH    (-5)

typedef enum
{
	SCIRPT_PARSER_VALUE_TYPE_INVALID = 0,
	SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD,
	SCIRPT_PARSER_VALUE_TYPE_STRING,
	SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD,
	SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD
} script_parser_value_type_t;

typedef struct
{
	char  gpio_name[32];
	int port;
	int port_num;
	int mul_sel;
	int pull;
	int drv_level;
	int data;
} script_gpio_set_t;

typedef struct
{
	unsigned  main_key_count;
	unsigned  length;
	unsigned  version[2];
} script_head_t;

typedef struct
{
	char main_name[32];
	int  lenth;
	int  offset;
} script_main_key_t;

typedef struct
{
	char sub_name[32];
	int  offset;
	int  pattern;
} script_sub_key_t;


#define   EGPIO_FAIL             (-1)
#define   EGPIO_SUCCESS          (0)

typedef enum
{
	PIN_PULL_DEFAULT 	= 	0xFF,
	PIN_PULL_DISABLE 	=	0x00,
	PIN_PULL_UP			  =	0x01,
	PIN_PULL_DOWN	  	=	0x02,
	PIN_PULL_RESERVED	=	0x03
} pin_pull_level_t;

typedef	enum
{
	PIN_MULTI_DRIVING_DEFAULT	=	0xFF,
	PIN_MULTI_DRIVING_0			=	0x00,
	PIN_MULTI_DRIVING_1			=	0x01,
	PIN_MULTI_DRIVING_2			=	0x02,
	PIN_MULTI_DRIVING_3			=	0x03
} pin_drive_level_t;

typedef enum
{
	PIN_DATA_LOW,
	PIN_DATA_HIGH,
	PIN_DATA_DEFAULT = 0XFF
} pin_data_t;

#define	PIN_PHY_GROUP_A			0x00
#define	PIN_PHY_GROUP_B			0x01
#define	PIN_PHY_GROUP_C			0x02
#define	PIN_PHY_GROUP_D			0x03
#define	PIN_PHY_GROUP_E			0x04
#define	PIN_PHY_GROUP_F			0x05
#define	PIN_PHY_GROUP_G			0x06
#define	PIN_PHY_GROUP_H			0x07
#define	PIN_PHY_GROUP_I			0x08
#define	PIN_PHY_GROUP_J			0x09

typedef struct
{
    char  gpio_name[32];
    int port;
    int port_num;
    int mul_sel;
    int pull;
    int drv_level;
    int data;
} user_gpio_set_t;

/* functions for early boot */
extern int sw_cfg_get_int(const char *script_buf, const char *main_key, const char *sub_key);
extern char *sw_cfg_get_str(const char *script_buf, const char *main_key, const char *sub_key, char *buf);

/* script operations */
extern int script_parser_init(char *script_buf);
#ifdef CONFIG_SMALL_MEMSIZE
extern void save_config(void);
extern void reload_config(void);
#endif
extern int script_parser_exit(void);
extern unsigned script_get_length(void);
extern uint script_parser_fetch_subkey_start(char *main_name);
extern int script_parser_fetch_subkey_next(uint hd, char *sub_name, int value[], int *index);
extern int script_parser_fetch(char *main_name, char *sub_name, int value[], int count);
extern int script_parser_fetch_ex(char *main_name, char *sub_name, int value[],
               script_parser_value_type_t *type, int count);
extern int script_parser_patch(char *main_name, char *sub_name, void *str, int str_size);
extern int script_parser_subkey_count(char *main_name);
extern int script_parser_mainkey_count(void);
extern int script_parser_mainkey_get_gpio_count(char *main_name);
extern int script_parser_mainkey_get_gpio_cfg(char *main_name, void *gpio_cfg, int gpio_count);

extern uint script_parser_subkey( script_main_key_t* main_name,char *subkey_name , uint *pattern);

extern int script_parser_patch_all(char *main_name, void *str, uint data_count);

/* gpio operations */
extern int gpio_init(void);
extern int gpio_exit(void);
extern int gpio_request_simple(char *main_name, const char *sub_name);
extern unsigned gpio_request(user_gpio_set_t *gpio_list, unsigned group_count_max);
extern unsigned gpio_request_ex(char *main_name, const char *sub_name);
int gpio_request_early(void  *user_gpio_list, __u32 group_count_max, __s32 set_gpio);
extern int gpio_release(unsigned p_handler, int if_release_to_default_status);
extern int gpio_get_all_pin_status(unsigned p_handler, user_gpio_set_t *gpio_status, unsigned gpio_count_max, unsigned if_get_from_hardware);
extern int gpio_get_one_pin_status(unsigned p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, unsigned if_get_from_hardware);
extern int gpio_set_one_pin_status(unsigned p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, unsigned if_set_to_current_input_status);
extern int gpio_set_one_pin_io_status(unsigned p_handler, unsigned if_set_to_output_status, const char *gpio_name);
extern int gpio_set_one_pin_pull(unsigned p_handler, unsigned set_pull_status, const char *gpio_name);
extern int gpio_set_one_pin_driver_level(unsigned p_handler, unsigned set_driver_level, const char *gpio_name);
extern int gpio_read_one_pin_value(unsigned p_handler, const char *gpio_name);
extern int gpio_write_one_pin_value(unsigned p_handler, unsigned value_to_gpio, const char *gpio_name);

extern void upper(char *str);
extern void lower(char *str);

#endif
