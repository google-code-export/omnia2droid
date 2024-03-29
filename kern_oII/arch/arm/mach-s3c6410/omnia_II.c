/* linux/arch/arm/mach-s3c6410/mach-smdk6410.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/smsc911x.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/reboot.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>

#ifdef PHONE_B7610
#include <linux/spi/wl12xx.h>
#else
#include <linux/spi/libertas_spi.h>
#endif

#include <plat/spi-clocks.h>
#include <plat/s3c64xx-spi.h>

#include <video/platform_lcd.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/regs-fb.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/regs-rtc.h>
#include <plat/regs-sys.h>
#include <plat/regs-clock.h>
#include <plat/iic.h>
#include <plat/fimc.h>
#include <plat/fb.h>

#include <plat/s3c6410.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/adc.h>
#include <plat/reserved_mem.h>
#include <plat/regs-lcd.h>
#include <plat/pm.h>
#include <plat/media.h>
#include <plat/gpio-cfg.h>
#include <linux/android_pmem.h>
#include <linux/i2c/pmic.h>
#include <plat/regs-gpio.h>
#include <mach/param.h>

#include <linux/delay.h>
#include <linux/usb/android.h>

#include <mach/sec_headset.h>

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

#include "omnia_II_gpio_table.h"
void s3c_config_gpio_table(int array_size, int (*gpio_table)[6]);

#define UCON S3C_UCON_DEFAULT
#define ULCON S3C_LCON_CS8 | S3C_LCON_PNONE
#define UFCON S3C_UFCON_RXTRIG8 | S3C_UFCON_FIFOMODE

#ifndef CONFIG_HIGH_RES_TIMERS
extern struct sys_timer s3c64xx_timer;
#else
extern struct sys_timer sec_timer;
#endif /* CONFIG_HIGH_RES_TIMERS */

static struct s3c_uartcfg saturn_uartcfgs[] __initdata = {
	[0] = {	/* Phone */
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {	/* Bluetooth */
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[2] = {	/* Serial */
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
};

static struct i2c_gpio_platform_data i2c_pmic_platdata = {
	.sda_pin	= GPIO_PWR_I2C_SDA,
	.scl_pin	= GPIO_PWR_I2C_SCL,
	.udelay		= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1
};

static struct platform_device sec_device_i2c_pmic = {
	.name	= "i2c-gpio",
	.id		= 2,
	.dev.platform_data	= &i2c_pmic_platdata,
};

static struct i2c_gpio_platform_data i2c_common_platdata = {
	.sda_pin	= GPIO_FM_SDA,
	.scl_pin	= GPIO_FM_SCL,
	.udelay		= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 1
};

static struct platform_device sec_device_i2c_common = {
	.name	= "i2c-gpio",
	.id		= 5,
	.dev.platform_data	= &i2c_common_platdata,
};

static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs1[] __initdata = {
};

static void __init saturn_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline, struct meminfo *mi)
{
	mi->nr_banks = 2;
	mi->bank[0].start = UL(0x50000000);//PHYS_OFFSET;
	mi->bank[0].size = (128 * 1024 * 1024); //PHYS_UNRESERVED_SIZE;
	mi->bank[0].node = PHYS_TO_NID(0x50000000);// 0;
	mi->bank[1].start = UL(0x60000000);//PHYS_OFFSET;
	mi->bank[1].size = PHYS_UNRESERVED_SIZE;//80 * 1024 * 1024;//PHYS_UNRESERVED_SIZE;
	mi->bank[1].node = PHYS_TO_NID(0x60000000);///0;
}

struct platform_device sec_device_opt_prox = {
        .name   = "gp2a-opt",
        .id             = -1,
};

struct platform_device sec_device_opt_light = {
        .name   = "isl29023-opt",
        .id             = -1,
};

static struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id		= -1,
};

struct platform_device sec_device_ts = {
        .name   = "qt602240-ts",
        .id             = -1,
};


struct platform_device sec_device_battery = {
        .name   = "saturn-battery",
        .id             = -1,
};

struct platform_device sec_device_fuelgauge = {
        .name = "max17040_driver",
        .id = -1,
};

struct platform_device sec_device_backlight = {
	.name   = "ams320fs01-backlight",
	.id     = -1,
};

static struct sec_headset_port sec_headset_port[] = {
        {
		{ // HEADSET detect info
			.eint		= IRQ_EINT(10), 
			.gpio		= GPIO_DET_35,   
			.gpio_af	= GPIO_DET_35_AF  , 
			.low_active 	= 0
		}
		,{ // SEND/END info
#ifdef PHONE_B7610
			.eint		= IRQ_EINT(13), 
			.gpio		= GPIO_BOOT_EINT13, 
			.gpio_af	= GPIO_BOOT_EINT13_AF, 
			.low_active	= 1

//			.eint		= IRQ_EINT(11), 
//			.gpio		= GPIO_HALL_SW, 
//			.gpio_af	= GPIO_HALL_SW_AF, 
//			.low_active	= 1

#else
			.eint		= IRQ_EINT(11), 
			.gpio		= GPIO_EAR_SEND_END, 
			.gpio_af	= GPIO_EAR_SEND_END_AF, 
			.low_active	= 1
#endif
		}
        }
};
 
static struct sec_headset_platform_data sec_headset_data = {
        .port           = sec_headset_port,
        .nheadsets      = ARRAY_SIZE(sec_headset_port),
};

static struct platform_device sec_device_headset = {
        .name           = "sec_headset",
        .id             = -1,
        .dev            = {
                .platform_data  = &sec_headset_data,
        },
};

struct platform_device sec_device_rfkill = {
		.name = "bt_rfkill",
			.id = -1,
};

struct platform_device sec_device_btsleep = {
		.name = "bt_sleep",
			.id = -1,
};

#ifdef PHONE_B7610

// TI WL1271 (B7610) SPI Wlan setup
// check with linux/arch/arm/mach-omap2/board-rx51-peripherals.c simular omap spi setup

void set_cs_level( void __iomem *regs_base, unsigned line_id, int lvl) {
	gpio_set_value(GPIO_WLAN_SPI_nCS, lvl);
};

struct s3c64xx_spi_csinfo  wl12xx_chip_cs = {
	.fb_delay	= 0,
	.line		= 1,
	.set_level	= set_cs_level,
};

static struct wl12xx_platform_data wl12xx_pdata;

static struct spi_board_info s3c6410_spi_board_info[] = {
	{
                .modalias		= "wl12xx_spi",
                .mode			= SPI_MODE_0,
//              .max_speed_hz   	= 48000000,
                .max_speed_hz   	= 24000000,
                .bus_num		= 1,
		.irq			= IRQ_EINT(1),
                .chip_select		= 0,
                .controller_data	= &wl12xx_chip_cs,
                .platform_data		= &wl12xx_pdata,
        },

};

//static int wl12xx_setup(struct spi_device *spi)
static int wl12xx_setup(void)
{
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
//	gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_HIGH); 	
	gpio_set_value(GPIO_BT_nEN, GPIO_LEVEL_LOW); 	
	mdelay(600); // check value
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
	mdelay(20); // check value
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
	mdelay(100); // check value
	printk("wl12xx: enable wlan chip\n");
//	spi->bits_per_word = 16;
//	spi_setup done in wl12xx_probe()
//	spi_setup(spi);
	return 0;
}

static int wl12xx_teardown(struct spi_device *spi)
{
	if (gpio_get_value(GPIO_BT_nRST) == 0) {
//		gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_BT_nEN, GPIO_LEVEL_HIGH);
	}
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
	printk("wl12xx: disable wlan chip\n");
	return 0;
}

#define ARRAY_AND_SIZE(x)        (x), ARRAY_SIZE(x)
static void __init init_spi(void)
{
	s3c_gpio_cfgpin(GPIO_HOST_WAKE, S3C_GPIO_SFN(GPIO_HOST_WAKE_AF));
	s3c_gpio_setpull(GPIO_HOST_WAKE_AF, S3C_GPIO_PULL_UP); 
	set_irq_type(GPIO_HOST_WAKE, IRQ_TYPE_EDGE_FALLING); 

//	s3c_gpio_cfgpin(GPIO_BT_EN, S3C_GPIO_SFN(GPIO_BT_EN_AF));
	s3c_gpio_cfgpin(GPIO_BT_nEN, S3C_GPIO_SFN(GPIO_BT_nEN_AF));
	s3c_gpio_cfgpin(GPIO_BT_nRST, S3C_GPIO_SFN(GPIO_BT_nRST_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SFN(GPIO_WLAN_nRST_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SPI_nCS, S3C_GPIO_SFN(1)); // Output

	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
//	gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_LOW); 	
	gpio_set_value(GPIO_BT_nEN, GPIO_LEVEL_HIGH); 	
	
//	s3c64xx_spi_set_info(1, S3C64XX_SPI_SRCCLK_PCLK, 1);
	s3c64xx_spi_set_info(1, S3C64XX_SPI_SRCCLK_SPIBUS, 1);
	wl12xx_setup();
	spi_register_board_info(ARRAY_AND_SIZE(s3c6410_spi_board_info));
}

#else

// Libertas (I8000 and others) SPI Wlan setup

static int libertas_setup(struct spi_device *spi)
{
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
	gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_HIGH); 	
	mdelay(600);
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
	mdelay(20);
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
	mdelay(100);
	printk("Sanya: Power on wlan\n");
	spi->bits_per_word = 16;
	spi_setup(spi);
	return 0;
}

static int libertas_teardown(struct spi_device *spi)
{
	if (gpio_get_value(GPIO_BT_nRST) == 0) {
		gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_LOW);
	}
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
	return 0;
}

static struct libertas_spi_platform_data libertas_spi_pdata = {
        .use_dummy_writes       = 0,
	.setup = libertas_setup,
	.teardown = libertas_teardown,
};

void set_cs_level( void __iomem *regs_base, unsigned line_id, int lvl) {
	gpio_set_value(GPIO_WLAN_SPI_nCS, lvl);
};

struct s3c64xx_spi_csinfo  libertas_chip_cs = {
		.fb_delay	= 0,
		.line		= 1,
		.set_level	= set_cs_level,
};

static struct spi_board_info s3c6410_spi_board_info[] = {
	{
                .modalias		= "libertas_spi",
                .mode			= SPI_MODE_0,
//                .max_speed_hz   	= 48000000,
                .max_speed_hz   	= 24000000,
                .bus_num		= 1,
		.irq			= IRQ_EINT(1),
                .chip_select		= 0,
                .controller_data	= &libertas_chip_cs,
                .platform_data		= &libertas_spi_pdata,
        },

};

#define ARRAY_AND_SIZE(x)        (x), ARRAY_SIZE(x)
static void __init init_spi(void)
{
	s3c_gpio_cfgpin(GPIO_HOST_WAKE, S3C_GPIO_SFN(GPIO_HOST_WAKE_AF));
	s3c_gpio_setpull(GPIO_HOST_WAKE_AF, S3C_GPIO_PULL_UP); 
	set_irq_type(GPIO_HOST_WAKE, IRQ_TYPE_EDGE_FALLING); 

	s3c_gpio_cfgpin(GPIO_BT_EN, S3C_GPIO_SFN(GPIO_BT_EN_AF));
	s3c_gpio_cfgpin(GPIO_BT_nRST, S3C_GPIO_SFN(GPIO_BT_nRST_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SFN(GPIO_WLAN_nRST_AF));
	s3c_gpio_cfgpin(GPIO_WLAN_SPI_nCS, S3C_GPIO_SFN(1)); // Output

	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
	gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_LOW); 	
	
//	s3c64xx_spi_set_info(1, S3C64XX_SPI_SRCCLK_PCLK, 1);
	s3c64xx_spi_set_info(1, S3C64XX_SPI_SRCCLK_SPIBUS, 1);
	spi_register_board_info(ARRAY_AND_SIZE(s3c6410_spi_board_info));
}

#endif

static struct s3c6410_pmem_setting pmem_setting = {
        .pmem_start = RESERVED_PMEM_START,
        .pmem_size = RESERVED_PMEM,
        .pmem_gpu1_start = GPU1_RESERVED_PMEM_START,
        .pmem_gpu1_size = RESERVED_PMEM_GPU1,
        .pmem_render_start = RENDER_RESERVED_PMEM_START,
        .pmem_render_size = RESERVED_PMEM_RENDER,
        .pmem_stream_start = STREAM_RESERVED_PMEM_START,
        .pmem_stream_size = RESERVED_PMEM_STREAM,
        .pmem_preview_start = PREVIEW_RESERVED_PMEM_START,
        .pmem_preview_size = RESERVED_PMEM_PREVIEW,
        .pmem_picture_start = PICTURE_RESERVED_PMEM_START,
        .pmem_picture_size = RESERVED_PMEM_PICTURE,
        .pmem_jpeg_start = JPEG_RESERVED_PMEM_START,
        .pmem_jpeg_size = RESERVED_PMEM_JPEG,
        .pmem_skia_start = SKIA_RESERVED_PMEM_START,
        .pmem_skia_size = RESERVED_PMEM_SKIA,
};


extern int get_usb_cable_state(void);

#define AV                                      (0x1 << 14)
#define TTY                                     (0x1 << 13)
#define PPD                                     (0x1 << 12)
#define JIG_UART_OFF            (0x1 << 11)
#define JIG_UART_ON                     (0x1 << 10)
#define JIG_USB_OFF                     (0x1 << 9)
#define JIG_USB_ON                      (0x1 << 8)
#define USB_OTG                         (0x1 << 7)
#define DEDICATED_CHARGER       (0x1 << 6)
#define USB_CHARGER                     (0x1 << 5)
#define CAR_KIT                         (0x1 << 4)
#define UART                            (0x1 << 3)
#define USB                                     (0x1 << 2)
#define AUDIO_TYPE2                     (0x1 << 1)
#define AUDIO_TYPE1                     (0x1 << 0)

extern void arch_reset(char mode);

static void saturn_pm_power_off(void)
{
	int	mode = REBOOT_MODE_NONE;
	char reset_mode = 'r';
	int cnt = 0;

	if (!gpio_get_value(GPIO_TA_CONNECTED_N)) 
	{	/* Reboot Charging */
		mode = REBOOT_MODE_CHARGING;
		if (sec_set_param_value)
			sec_set_param_value(__REBOOT_MODE, &mode);
		/* Watchdog Reset */
		arch_reset(reset_mode);
	}
	else 
	{	/* Power Off or Reboot */
		if (sec_set_param_value)
			sec_set_param_value(__REBOOT_MODE, &mode);
		if (get_usb_cable_state() & (JIG_UART_ON | JIG_UART_OFF | JIG_USB_OFF | JIG_USB_ON)) {
			/* Watchdog Reset */
			arch_reset(reset_mode);
		}
		else 
		{
			/* POWER_N -> Input */
			gpio_direction_input(GPIO_POWER_N);
			gpio_direction_input(GPIO_PHONE_ACTIVE);

			if (!gpio_get_value(GPIO_POWER_N) || gpio_get_value(GPIO_PHONE_ACTIVE))
			{	
				/* Wait Power Button Release */
				while (gpio_get_value(GPIO_PHONE_ACTIVE)) 
				{
					if (cnt++ < 5) 
					{
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE is high(%d)\n", __func__, cnt);
						mdelay(1000);

					} 
					else 
					{
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE TIMED OUT!!!\n", __func__);
						
						// Display Yellow LED 	
						// (PHONE_RST: Output Low) -> (nPHONE_RST: Low) -> (MSM_PSHOLD: Low) -> CP PMIC Off
						gpio_direction_output(GPIO_PHONE_RST_N, GPIO_LEVEL_HIGH);
						s3c_gpio_setpull(GPIO_PHONE_RST_N, S3C_GPIO_PULL_NONE);
						gpio_set_value(GPIO_PHONE_RST_N, GPIO_LEVEL_LOW);
						break;
					}
				}	
				while (!gpio_get_value(GPIO_POWER_N));
			}

			if (!gpio_get_value(GPIO_TA_CONNECTED_N) || !gpio_get_value(GPIO_CHG_ING_N) ) 
			{
	        		mode = REBOOT_MODE_CHARGING;
				if (sec_set_param_value)
					sec_set_param_value(__REBOOT_MODE, &mode);
				/* Watchdog Reset */
				arch_reset(reset_mode);
			}			
			else
			{
				if (sec_set_param_value)
					sec_set_param_value(__REBOOT_MODE, &mode);				
				/* PS_HOLD -> Output Low */
				gpio_direction_output(GPIO_PDA_PS_HOLD, GPIO_LEVEL_HIGH);
				s3c_gpio_setpull(GPIO_PDA_PS_HOLD, S3C_GPIO_PULL_NONE);
				gpio_set_value(GPIO_PDA_PS_HOLD, GPIO_LEVEL_LOW);
			}	
		}
	}
	
	while (1);
}

static int uart_current_owner = 0;

static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if ( uart_current_owner )		
		return sprintf(buf, "%s[UART Switch] Current UART owner = PDA \n", buf);	
	else			
		return sprintf(buf, "%s[UART Switch] Current UART owner = MODEM \n", buf);
}

static ssize_t uart_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{	
	int switch_sel = 1;

	if (sec_get_param_value)
		sec_get_param_value(__SWITCH_SEL, &switch_sel);

	if (strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0)	{		
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);		
		uart_current_owner = 1;		
		switch_sel |= UART_SEL_MASK;
		printk("[UART Switch] Path : PDA\n");	
	}	

	if (strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {		
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);		
		uart_current_owner = 0;		
		switch_sel &= ~UART_SEL_MASK;
		printk("[UART Switch] Path : MODEM\n");	
	}	

	if (sec_set_param_value)
		sec_set_param_value(__SWITCH_SEL, &switch_sel);

	return size;
}

static DEVICE_ATTR(uart_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, uart_switch_show, uart_switch_store);

static void saturn_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

        if (gpio_is_valid(GPIO_UART_SEL)) {
                if (gpio_request(GPIO_UART_SEL, S3C_GPIO_LAVEL(GPIO_UART_SEL)))
                        printk(KERN_ERR "Failed to request GPIO_UART_SEL!\n");
                gpio_direction_output(GPIO_UART_SEL, gpio_get_value(GPIO_UART_SEL));
        }
        s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

        if (device_create_file(switch_dev, &dev_attr_uart_sel) < 0)
                pr_err("Failed to create device file(%s)!\n", dev_attr_uart_sel.attr.name);

};

static struct map_desc smdk6410_iodesc[] = {
	{
		.virtual	= (unsigned long)S3C_VA_LCD,
		.pfn		= __phys_to_pfn(S3C64XX_PA_LCD),
		.length	= S3C64XX_SZ_LCD,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C64XX_VA_HOSTIFB),
		.pfn		= __phys_to_pfn(S3C64XX_PA_HOSTIFB),
		.length	= S3C64XX_SZ_HOSTIFB,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C64XX_VA_HOSTIFA),
		.pfn		= __phys_to_pfn(S3C64XX_PA_HOSTIFA),
		.length	= S3C64XX_SZ_HOSTIFA,
		.type		= MT_DEVICE,
	},{
		.virtual	= (unsigned long)(S3C64XX_VA_OTG),
		.pfn		= __phys_to_pfn(S3C64XX_PA_OTG),
		.length	= S3C64XX_SZ_OTG,
		.type		= MT_DEVICE,
	},{
		.virtual	= (unsigned long)(S3C64XX_VA_OTGSFR),
		.pfn		= __phys_to_pfn(S3C64XX_PA_OTGSFR),
		.length	= S3C64XX_SZ_OTGSFR,
		.type		= MT_DEVICE,
	},
};

static struct platform_device *smdk6410_devices[] __initdata = {
#if defined(CONFIG_S3C_DMA_PL080_SOL)
        &s3c_device_dma0,
        &s3c_device_dma1,
        &s3c_device_dma2,
        &s3c_device_dma3,
#endif
#ifdef CONFIG_SMDK6410_SD_CH1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_SMDK6410_SD_CH0
	&s3c_device_hsmmc0,
#endif
	&s3c_device_i2c0,
#ifdef CONFIG_S3C_DEV_I2C1
	&s3c_device_i2c1,
#endif
	&s3c_device_camif,
	&s3c_device_lcd,
	&s3c_device_keypad,
	&s3c_device_ts,
	&s3c_device_vpp,
	&s3c_device_mfc,
	&s3c_device_rotator,
	&s3c_device_jpeg,
	&s3c_device_2d,
	&s3c_device_g3d,
	&sec_device_dpram,
	&sec_device_backlight,
#ifdef CONFIG_S3C64XX_ADC
	&s3c_device_adc,
#endif
	&s3c_device_rtc,
#ifdef CONFIG_SND_S3C64XX_SOC_I2S
	&s3c64xx_device_iis0,
#endif
	&s3c_device_usbgadget,
	&sec_device_i2c_pmic,			/* pmic(max8698) i2c. */
	&sec_device_i2c_common,			/* radio, sound, .. i2c. */
	&sec_device_headset,
	&sec_device_opt_prox,
	&sec_device_opt_light,
	&sec_device_ts,
	&sec_device_battery,
	&sec_device_rfkill,   //BT POWER ON-OFF
//	&sec_device_btsleep,  //BT SLEEP-AWAKE
	&sec_device_fuelgauge,
	&s3c64xx_device_spi1,
};

#ifdef CONFIG_S3C64XX_ADC
static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000, //41237
	.presc 			= 49,
	.oversampling_shift	= 2,//4
	.resol_bit 			= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform = {
	/* s3c6410 support 12-bit resolution */
	.delay	= 	10000,
	.presc 	= 	99,
	.resolution = 	12,
};
#endif

static void __init smdk6410_map_io(void)
{
	pm_power_off = saturn_pm_power_off;
	s3c64xx_init_io(smdk6410_iodesc, ARRAY_SIZE(smdk6410_iodesc));
	s3c_init_clocks(12000000);
        s3c_init_uarts(saturn_uartcfgs, ARRAY_SIZE(saturn_uartcfgs));
}

void __iomem *s3c6410_wdt_addr;

static void s3c6410_wdt_io_map(void)
{
        s3c6410_wdt_addr = ioremap(S3C64XX_PA_WATCHDOG,S3C64XX_SZ_WATCHDOG);
}

static void __init smdk6410_machine_init(void)
{

	s3c_config_gpio_table(ARRAY_SIZE(omnia_II_init_gpio_table),
		omnia_II_init_gpio_table);
	
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

#ifdef CONFIG_S3C64XX_ADC
	s3c_ts_set_platdata(&s3c_ts_platform);
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

	init_spi();

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	s3c6410_add_mem_devices (&pmem_setting);

	platform_add_devices(smdk6410_devices, ARRAY_SIZE(smdk6410_devices));
	s3c6410_pm_init();
	s3c6410_wdt_io_map();
	saturn_switch_init();

}

MACHINE_START(SMDK6410, "OMNIA II")
	/* Maintainer: Ben Dooks <ben@fluff.org> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S3C64XX_PA_SDRAM + 0x100,
	.init_irq	= s3c6410_init_irq,
	.fixup		= saturn_fixup,
	.map_io		= smdk6410_map_io,
	.init_machine	= smdk6410_machine_init,
#ifndef CONFIG_HIGH_RES_TIMERS
        .timer                  = &s3c64xx_timer,
#else
        .timer                  = &sec_timer,
#endif
MACHINE_END

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

        tmp = readw(base + S3C_RTCCON) & (S3C_RTCCON_TICEN | S3C_RTCCON_RTCEN );
        writew(tmp, base + S3C_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C_RTCCON);
		writew(tmp & ~ (S3C_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C_RTCCON) & S3C_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp|S3C_RTCCON_RTCEN, base+S3C_RTCCON);
		}

		if ((readw(base + S3C_RTCCON) & S3C_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp& ~S3C_RTCCON_CNTSEL, base+S3C_RTCCON);
		}

		if ((readw(base + S3C_RTCCON) & S3C_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C_RTCCON);
			writew(tmp & ~S3C_RTCCON_CLKRST, base+S3C_RTCCON);
		}
	}
}
#endif

void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

#ifndef PHONE_B7610
	end = S3C64XX_GPK(8 + rows);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = S3C64XX_GPK(8); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}

	end = S3C64XX_GPL(0 + columns);

	/* Set all the necessary GPL pins to special-function 0 */
	for (gpio = S3C64XX_GPL(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
#endif
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_AP_FLM_RXD, S3C_GPIO_SFN(GPIO_AP_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_FLM_TXD, S3C_GPIO_SFN(GPIO_AP_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_TXD, S3C_GPIO_PULL_NONE);
	}
	else if (port == 1) {
		s3c_gpio_cfgpin(GPIO_AP_BT_RXD, S3C_GPIO_SFN(GPIO_AP_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_BT_TXD, S3C_GPIO_SFN(GPIO_AP_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_BT_CTS, S3C_GPIO_SFN(GPIO_AP_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_AP_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_BT_RTS, S3C_GPIO_SFN(GPIO_AP_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_AP_BT_RTS, S3C_GPIO_PULL_NONE);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);

void s3c_reset_uart_cfg_gpio(unsigned char port)
{
#if 0  // dgahn.temp
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_AP_FLM_RXD, S3C_GPIO_SFN(GPIO_AP_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_FLM_TXD, S3C_GPIO_SFN(GPIO_AP_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_FLM_TXD, S3C_GPIO_PULL_NONE);
	}
#endif
	if (port == 1) {
		s3c_gpio_cfgpin(GPIO_AP_BT_RXD, 0);
		s3c_gpio_setpull(GPIO_AP_BT_RXD, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_AP_BT_TXD, 0);
		s3c_gpio_setpull(GPIO_AP_BT_TXD, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_AP_BT_CTS, 0);
		s3c_gpio_setpull(GPIO_AP_BT_CTS, S3C_GPIO_PULL_DOWN);
		s3c_gpio_cfgpin(GPIO_AP_BT_RTS, 0);
		s3c_gpio_setpull(GPIO_AP_BT_RTS, S3C_GPIO_PULL_DOWN);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(s3c_reset_uart_cfg_gpio);

unsigned int CURRENT_REV(void)
{
	return 1;
}
EXPORT_SYMBOL(CURRENT_REV);

int (*connected_device_status)(void) = NULL;
EXPORT_SYMBOL(connected_device_status);

void s3c_config_gpio_table(int array_size, int (*gpio_table)[6])
{
	u32 i, gpio;

	pr_debug("%s: ++\n", __func__);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		if (gpio < S3C64XX_GPIO_ALIVE_PART_BASE) { /* Off Part */
			pr_debug("%s: Off gpio=%d,%d\n", __func__, gpio, 
					S3C64XX_GPIO_ALIVE_PART_BASE);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_slp_cfgpin(gpio, gpio_table[i][4]);
			s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][5]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		} else if (gpio < S3C64XX_GPIO_MEM_PART_BASE) { /* Alive Part */
			pr_debug("%s: Alive gpio=%d\n", __func__, gpio);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		} else { /* Memory Part */
			pr_debug("%s: Memory gpio=%d\n", __func__, gpio);
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
			s3c_gpio_setpull(gpio, gpio_table[i][3]);
			s3c_gpio_slp_cfgpin(gpio, gpio_table[i][4]);
			s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][5]);
			if (gpio_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(gpio, gpio_table[i][2]);
		}
	}
	pr_debug("%s: --\n", __func__);
}
EXPORT_SYMBOL(s3c_config_gpio_table);


static void check_pmic(void)
{	
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(EN1, &reg_buff)) {
		pr_info("%s: BUCK1 (%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(EN1, 0);
	}
	if (Get_MAX8698_PM_REG(EN2, &reg_buff)) {
		pr_info("%s: BUCK2 (%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(EN2, 0);
	}
	if (Get_MAX8698_PM_REG(ELDO3, &reg_buff)) {
		pr_info("%s: OTGI 1.2V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO4, &reg_buff)) {
		pr_info("%s: VLED 3.3V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: VTF 3.0V (%d)\n", __func__, reg_buff);
		if (reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 0);
	}
	if (Get_MAX8698_PM_REG(ELDO6, &reg_buff)) {
		pr_info("%s: VLCD 1.8V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO7, &reg_buff)) {
		pr_info("%s: VLCD 3.0V (%d)\n", __func__, reg_buff);
	}
	if (Get_MAX8698_PM_REG(ELDO8, &reg_buff)) {
		pr_info("%s: OTG 3.3V (%d)\n", __func__, reg_buff);
	}
}

void s3c_config_sleep_gpio(void)
{	
	int spcon_val;
	printk("s3c_config_sleep_gpio");
	check_pmic();
	s3c_config_gpio_table(ARRAY_SIZE(omnia_II_sleep_gpio_table),
	omnia_II_sleep_gpio_table);

	spcon_val = __raw_readl(S3C64XX_SPCON);
	spcon_val = spcon_val & (~0xFFEC0000);
	__raw_writel(spcon_val, S3C64XX_SPCON);
	__raw_writel(0x20, S3C64XX_SPCONSLP);

	/* mem interface reg config in sleep mode */
	__raw_writel(0x00005000, S3C64XX_MEM0CONSLP0);
	__raw_writel(0x01041595, S3C64XX_MEM0CONSLP1);
	__raw_writel(0x10055000, S3C64XX_MEM1CONSLP);

}
EXPORT_SYMBOL(s3c_config_sleep_gpio);

void s3c_config_wakeup_gpio(void)
{
	unsigned char reg_buff = 0;
	if (Get_MAX8698_PM_REG(ELDO5, &reg_buff)) {
		pr_info("%s: VTF 3.0V (%d)\n", __func__, reg_buff);
		if (!reg_buff)
			Set_MAX8698_PM_REG(ELDO5, 1);
	}
}
EXPORT_SYMBOL(s3c_config_wakeup_gpio);

void s3c_config_wakeup_source(void)
{
	unsigned int eint0pend_val;

	/* Wake-up source 
	 * ONEDRAM_INT(EINT0), WLAN_HOST_WAKE(EINT1),  
	 * Power key(EINT5), T_FLASH_DETECT(EINT6), INTB(EINT9)
	 * DET_3.5(EINT10), EAR_SEND_END(EINT11),
	 * Hold key(EINT17), TA_CONNECTED(EINT19),
	 * BT_HOST_WAKE(EINT22), CHG_ING(EINT25)
	 */
	eint0pend_val= __raw_readl(S3C64XX_EINT0PEND);
	eint0pend_val |= (0x1 << 25) | (0x1 << 22) | (0x1 << 19) |
		(0x1 << 17) | (0x1 << 11) | (0x1 << 10) | (0x1 << 9) | (0x1 << 6) | (0x1 << 5) | (0x1 << 1) | 0x1;
	__raw_writel(eint0pend_val, S3C64XX_EINT0PEND);

	eint0pend_val = (0x1 << 25) | (0x1 << 22) | (0x1 << 19) |
		(0x1 << 17) | (0x1 << 11) | (0x1 << 10) | (0x1 << 9) | (0x1 << 6) | (0x1 << 5) | (0x1 << 1) | 0x1;
	__raw_writel(~eint0pend_val, S3C64XX_EINT0MASK);

	__raw_writel((0x0FFFFFFF & ~eint0pend_val), S3C_EINT_MASK);	

	/* Alarm Wakeup Enable */
	__raw_writel((__raw_readl(S3C_PWR_CFG) & ~(0x1 << 10)), S3C_PWR_CFG);
}
EXPORT_SYMBOL(s3c_config_wakeup_source);



