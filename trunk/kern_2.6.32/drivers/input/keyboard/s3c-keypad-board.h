/* linux/drivers/input/keyboard/s3c-keypad.h 
 *
 * Driver header for Samsung SoC keypad.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _S3C_KEYPAD_BOARD_H_
#define _S3C_KEYPAD_BOARD_H_

#if defined(CONFIG_MACH_MAX)
#define KEYPAD_COLUMNS  (3)     /* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_2 */
#define KEYPAD_ROWS     (3)     /* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_2 */
#else
#define KEYPAD_COLUMNS  (7)     /* GPIO_KEYSENSE_0 ~ GPIO_KEYSENSE_6 */
#define KEYPAD_ROWS     (7)     /* GPIO_KEYSCAN_0 ~ GPIO_KEYSCAN_6 */
#endif
#define MAX_KEYPAD_NR   (KEYPAD_COLUMNS * KEYPAD_ROWS)
#define MAX_KEYMASK_NR  32

#define KEYCODE_DUMPKEY 247
#define KEYCODE_SENDEND 248
#define KEYCODE_ENDCALL 249
#define KEYCODE_FOCUS   250
#define KEYCODE_HOLDKEY 251
//#define KEYCODE_CAMERA  252

#define CONFIG_INSTINCTQ_REV	0x0000
#define GPIO_POWER_N		GPIO_nPOWER
#define GPIO_POWER_N_AF		GPIO_nPOWER_AF

u32 g_board_num = CONFIG_INSTINCTQ_REV;


#ifndef CONFIG_MACH_MAX
struct s3c_keypad_slide slide_instinctq = {IRQ_EINT(11), GPIO_HALL_SW, GPIO_HALL_SW_AF, 1};
#endif

struct s3c_keypad_special_key special_key_instinctq_00_01[] = {
	{0x00000000, 0x00100000,  KEYCODE_FOCUS},
	{0x20000020, 0x00000020,  KEYCODE_DUMPKEY},
};

struct s3c_keypad_special_key special_key_instinctq_01a_40[] = {
	{0x00020000,   0x00000000,    KEYCODE_FOCUS},
	{0x00050400,   0x00000000,    KEYCODE_DUMPKEY},
};
struct s3c_keypad_gpio_key gpio_key_instinctq_01[] = {
	{IRQ_EINT(5),  GPIO_POWER_N     , GPIO_POWER_N_AF     , KEYCODE_ENDCALL, 1},
};

#if defined(CONFIG_MACH_MAX)
struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, NULL,  &special_key_instinctq_01a_40[0], 2,   &gpio_key_instinctq_01[0] , 1, 0},	
};
#elif defined(CONFIG_MACH_VITAL)
struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, &slide_instinctq,  &special_key_instinctq_01a_40[0], 4,   &gpio_key_instinctq_01[0] , 1, 0},	
};
#else
struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, &slide_instinctq,  &special_key_instinctq_00_01[0], 4,   &gpio_key_instinctq_01[0] , 1, 0},
	
};
#endif

#endif				/* _S3C_KEYIF_H_ */
