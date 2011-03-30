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


#ifndef _S3C_KEYPAD_H_
#define _S3C_KEYPAD_H_

static void __iomem *key_base;

#define GPIO_nPOWER             S3C64XX_GPN(5)
#define GPIO_nPOWER_AF          (2)
#define GPIO_LEVEL_LOW          0

#define __CONFIG_EARKEY__				1

#define KEY_PRESSED 1
#define KEY_RELEASED 0

#ifdef PHONE_B7610
#define KEYPAD_COLUMNS  7
#define KEYPAD_ROWS     7
#define MAX_KEYPAD_NR   KEYPAD_COLUMNS * KEYPAD_ROWS
#else
#define KEYPAD_COLUMNS	3	
#define KEYPAD_ROWS	3
#define MAX_KEYPAD_NR   KEYPAD_COLUMNS * KEYPAD_ROWS
#endif

int keypad_keycode[] = {
#ifdef PHONE_B7610
                KEY_Q,    KEY_A,     KEY_LEFTSHIFT, KEY_LEFTALT,  KEY_POWER, KEY_MENU, KEY_K,          KEY_W,          KEY_S,     KEY_Z,
                KEY_BACK, KEY_SLEEP, KEY_PHONE,     KEY_L,        KEY_E,     KEY_D,    KEY_X,          KEY_RIGHTSHIFT, KEY_ESC,   KEY_BACK,
                KEY_M,    KEY_R,     KEY_F,         KEY_C,        KEY_COMMA, KEY_I,    KEY_VOLUMEDOWN, KEY_UP,         KEY_T,     KEY_G,
                KEY_V,    KEY_SPACE, KEY_O,         KEY_VOLUMEUP, KEY_DOWN,  KEY_Y,    KEY_H,          KEY_B,          KEY_DOT,   KEY_HOME,
                KEY_P,    KEY_ENTER, KEY_U,         KEY_J,        KEY_N,     KEY_LEFT, KEY_HOME,       KEY_BACKSPACE,  KEY_RIGHT,
#else
       KEY_POWER, KEY_SEARCH, KEY_CONFIG,
       KEY_SEND,  KEY_CONFIG, KEY_VOLUMEUP,
       102,       KEY_BACK,   KEY_VOLUMEDOWN, //key_MENU
#endif
	};

/*
int keypad_keycode[] = {
		1, 2, KEY_1, KEY_Q, KEY_A, 6, 7, KEY_LEFT,
		9, 10, KEY_2, KEY_W, KEY_S, KEY_Z, KEY_RIGHT, 16,
		17, 18, KEY_3, KEY_E, KEY_D, KEY_X, 23, KEY_UP,
		25, 26, KEY_4, KEY_R, KEY_F, KEY_C, 31, 32,
		33, KEY_O, KEY_5, KEY_T, KEY_G, KEY_V, KEY_DOWN, KEY_BACKSPACE,
		KEY_P, KEY_0, KEY_6, KEY_Y, KEY_H, KEY_SPACE, 47, 48,
		KEY_M, KEY_L, KEY_7, KEY_U, KEY_J, KEY_N, 55, KEY_ENTER,
		KEY_LEFTSHIFT, KEY_9, KEY_8, KEY_I, KEY_K, KEY_B, 63, KEY_COMMA
	};
*/

#define KEYPAD_ROW_GPIOCON      S3C64XX_GPKCON
#define KEYPAD_ROW_GPIOPUD      S3C64XX_GPKPUD
#define KEYPAD_COL_GPIOCON      S3C64XX_GPLCON
#define KEYPAD_COL_GPIOPUD      S3C64XX_GPLPUD

#define KEYPAD_DELAY		(50)

#define	KEYIFCOL_CLEAR		(readl(key_base+S3C_KEYIFCOL) & ~0xffff)
#define	KEYIFCON_CLEAR		(readl(key_base+S3C_KEYIFCON) & ~0x1f)
#define KEYIFFC_DIV		(readl(key_base+S3C_KEYIFFC) | 0x1)



struct s3c_keypad {
	struct input_dev *dev;
	int nr_rows;	
	int no_cols;
	int total_keys; 
	int keycodes[MAX_KEYPAD_NR];
};

extern void s3c_setup_keypad_cfg_gpio(int rows, int columns);

#endif				/* _S3C_KEYIF_H_ */
