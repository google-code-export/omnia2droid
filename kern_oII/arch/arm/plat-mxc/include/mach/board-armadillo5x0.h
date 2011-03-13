/*
 * Copyright 2009 Alberto Panizzo <maramaopercheseimorto@gmail.com>.
 * All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_BOARD_ARMADILLO5X0_H__
#define __ASM_ARCH_MXC_BOARD_ARMADILLO5X0_H__

#include <mach/hardware.h>

/* mandatory for CONFIG_DEBUG_LL */

#define MXC_LL_UART_PADDR	UART1_BASE_ADDR
#define MXC_LL_UART_VADDR	AIPS1_IO_ADDRESS(UART1_BASE_ADDR)

#endif
