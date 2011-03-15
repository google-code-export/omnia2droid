/* arch/arm/mach-s3c6400/include/mach/gpio.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C6400 - GPIO lib support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_GPIO_H
#define __ASM_ARCH_GPIO_H __FILE__

#define gpio_get_value	__gpio_get_value
#define gpio_set_value	__gpio_set_value
#define gpio_cansleep	__gpio_cansleep

extern int gpio_to_irq(unsigned gpio);
extern int irq_to_gpio(unsigned irq);

/* GPIO bank sizes */
#define S3C64XX_GPIO_A_NR	(8)
#define S3C64XX_GPIO_B_NR	(7)
#define S3C64XX_GPIO_C_NR	(8)
#define S3C64XX_GPIO_D_NR	(5)
#define S3C64XX_GPIO_E_NR	(5)
#define S3C64XX_GPIO_F_NR	(16)
#define S3C64XX_GPIO_G_NR	(7)
#define S3C64XX_GPIO_H_NR	(10)
#define S3C64XX_GPIO_I_NR	(16)
#define S3C64XX_GPIO_J_NR	(12)
#define S3C64XX_GPIO_K_NR	(16)
#define S3C64XX_GPIO_L_NR	(15)
#define S3C64XX_GPIO_M_NR	(6)
#define S3C64XX_GPIO_N_NR	(16)
#define S3C64XX_GPIO_O_NR	(16)
#define S3C64XX_GPIO_P_NR	(15)
#define S3C64XX_GPIO_Q_NR	(9)

/* GPIO bank numbes */

/* CONFIG_S3C_GPIO_SPACE allows the user to select extra
 * space for debugging purposes so that any accidental
 * change from one gpio bank to another can be caught.
*/

#define S3C64XX_GPIO_NEXT(__gpio) \
	((__gpio##_START) + (__gpio##_NR) + CONFIG_S3C_GPIO_SPACE + 1)

enum s3c_gpio_number {
	S3C64XX_GPIO_A_START = 0,
	S3C64XX_GPIO_B_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_A),
	S3C64XX_GPIO_C_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_B),
	S3C64XX_GPIO_D_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_C),
	S3C64XX_GPIO_E_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_D),
	S3C64XX_GPIO_F_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_E),
	S3C64XX_GPIO_G_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_F),
	S3C64XX_GPIO_H_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_G),
	S3C64XX_GPIO_I_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_H),
	S3C64XX_GPIO_J_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_I),
	S3C64XX_GPIO_K_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_J),
	S3C64XX_GPIO_L_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_K),
	S3C64XX_GPIO_M_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_L),
	S3C64XX_GPIO_N_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_M),
	S3C64XX_GPIO_O_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_N),
	S3C64XX_GPIO_P_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_O),
	S3C64XX_GPIO_Q_START = S3C64XX_GPIO_NEXT(S3C64XX_GPIO_P),
};

/* S3C64XX GPIO number definitions. */

#define S3C64XX_GPA(_nr)	(S3C64XX_GPIO_A_START + (_nr))
#define S3C64XX_GPB(_nr)	(S3C64XX_GPIO_B_START + (_nr))
#define S3C64XX_GPC(_nr)	(S3C64XX_GPIO_C_START + (_nr))
#define S3C64XX_GPD(_nr)	(S3C64XX_GPIO_D_START + (_nr))
#define S3C64XX_GPE(_nr)	(S3C64XX_GPIO_E_START + (_nr))
#define S3C64XX_GPF(_nr)	(S3C64XX_GPIO_F_START + (_nr))
#define S3C64XX_GPG(_nr)	(S3C64XX_GPIO_G_START + (_nr))
#define S3C64XX_GPH(_nr)	(S3C64XX_GPIO_H_START + (_nr))
#define S3C64XX_GPI(_nr)	(S3C64XX_GPIO_I_START + (_nr))
#define S3C64XX_GPJ(_nr)	(S3C64XX_GPIO_J_START + (_nr))
#define S3C64XX_GPK(_nr)	(S3C64XX_GPIO_K_START + (_nr))
#define S3C64XX_GPL(_nr)	(S3C64XX_GPIO_L_START + (_nr))
#define S3C64XX_GPM(_nr)	(S3C64XX_GPIO_M_START + (_nr))
#define S3C64XX_GPN(_nr)	(S3C64XX_GPIO_N_START + (_nr))
#define S3C64XX_GPO(_nr)	(S3C64XX_GPIO_O_START + (_nr))
#define S3C64XX_GPP(_nr)	(S3C64XX_GPIO_P_START + (_nr))
#define S3C64XX_GPQ(_nr)	(S3C64XX_GPIO_Q_START + (_nr))

/* the end of the S3C64XX specific gpios */
#define S3C64XX_GPIO_END	(S3C64XX_GPQ(S3C64XX_GPIO_Q_NR) + 1)
#define S3C_GPIO_END		S3C64XX_GPIO_END

#define S3C64XX_GPIO_OFF_PART_BASE		S3C64XX_GPA(0)
#define S3C64XX_GPIO_ALIVE_PART_BASE		S3C64XX_GPK(0)
#define S3C64XX_GPIO_MEM_PART_BASE		S3C64XX_GPO(0)

/* define the number of gpios we need to the one after the GPQ() range */
#define ARCH_NR_GPIOS	(S3C64XX_GPQ(S3C64XX_GPIO_Q_NR) + 1)

#include <asm-generic/gpio.h>

#define S3C_GPIO_LAVEL(x)	((x >= S3C64XX_GPIO_Q_START) ? "GPQ" : \
								((x >= S3C64XX_GPIO_P_START) ? "GPP" : \
								((x >= S3C64XX_GPIO_O_START) ? "GPO" : \
								((x >= S3C64XX_GPIO_N_START) ? "GPN" : \
								((x >= S3C64XX_GPIO_M_START) ? "GPM" : \
								((x >= S3C64XX_GPIO_L_START) ? "GPL" : \
								((x >= S3C64XX_GPIO_K_START) ? "GPK" : \
								((x >= S3C64XX_GPIO_J_START) ? "GPJ" : \
								((x >= S3C64XX_GPIO_I_START) ? "GPI" : \
								((x >= S3C64XX_GPIO_H_START) ? "GPH" : \
								((x >= S3C64XX_GPIO_G_START) ? "GPG" : \
								((x >= S3C64XX_GPIO_F_START) ? "GPF" : \
								((x >= S3C64XX_GPIO_E_START) ? "GPE" : \
								((x >= S3C64XX_GPIO_D_START) ? "GPD" : \
								((x >= S3C64XX_GPIO_C_START) ? "GPC" : \
								((x >= S3C64XX_GPIO_B_START) ? "GPB" : \
								((x >= S3C64XX_GPIO_A_START) ? "GPA" : NULL))))))))))))))))) \

#include <plat/gpio-bank-a.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-d.h>
#include <plat/gpio-bank-e.h>
#include <plat/gpio-bank-f.h>
#include <plat/gpio-bank-g.h>
#include <plat/gpio-bank-h.h>
#include <plat/gpio-bank-i.h>
#include <plat/gpio-bank-j.h>
#include <plat/gpio-bank-k.h>
#include <plat/gpio-bank-l.h>
#include <plat/gpio-bank-m.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-bank-o.h>
#include <plat/gpio-bank-p.h>
#include <plat/gpio-bank-q.h>


#endif /* __ASM_ARCH_GPIO_H */