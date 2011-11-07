 
 /*****************************************************************************
 *
 * COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
 *
 *****************************************************************************/
 


#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>

#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>

#include "ak8973b.h"

#include <linux/earlysuspend.h>

#define E_COMPASS_ADDRESS	0x1c	/* CAD0 : 0, CAD1 : 0 */
#define I2C_DF_NOTIFY       0x01
#define IRQ_COMPASS_INT IRQ_EINT(2) /* EINT(2) */

#define O_MakmX 2
#define O_MakmY 3
#define O_MakmZ 4

static int MakmX = O_MakmX;
static int MakmY = O_MakmY;
static int MakmZ = O_MakmZ;

#define O_AX 6
#define O_AY 7
#define O_AZ 8
#define O_MX 9
#define O_MY 10
#define O_MZ 11

static int AX = O_AY;
static int AY = O_AX;
static int AZ = O_AZ;
static int MX = O_MX;
static int MY = O_MY;
static int MZ = O_MZ;

static int Makm_sign = 0; 
static int accel_inp_dev_sign = 5; 
static int mag_inpf_sign = 1; 

static int print_flag = 0; 

static struct i2c_client *this_client;

struct ak8973b_data {
	struct input_dev *input_dev;
	struct early_suspend	early_suspend;
};

static DECLARE_WAIT_QUEUE_HEAD(open_wq);

static atomic_t open_count;
static atomic_t open_flag;
static atomic_t reserve_open_flag;

static atomic_t m_flag; 
static atomic_t a_flag; 
static atomic_t t_flag; 
static atomic_t mv_flag;
static atomic_t	p_flag;

static short akmd_delay = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
static atomic_t suspend_flag = ATOMIC_INIT(0);
#endif /* CONFIG_HAS_EARLYSUSPEND */ 

/* following are the sysfs callback functions */

#define config_ctrl_reg(name,address) \
	static ssize_t name##_show(struct device *dev, struct device_attribute *attr, \
		char *buf) \
{ \
	struct i2c_client *client = to_i2c_client(dev); \
	return sprintf(buf, "%u\n", i2c_smbus_read_byte_data(client,address)); \
} \
static ssize_t name##_store(struct device *dev, struct device_attribute *attr, \
	const char *buf,size_t count) \
{ \
	struct i2c_client *client = to_i2c_client(dev); \
	unsigned long val = simple_strtoul(buf, NULL, 10); \
	if (val > 0xff) \
		return -EINVAL; \
	i2c_smbus_write_byte_data(client,address, val); \
		return count; \
} \
static DEVICE_ATTR(name, S_IWUSR | S_IRUGO, name##_show, name##_store)

config_ctrl_reg(ms1, AKECS_REG_MS1);

static int __devinit i2c_ak8973b_probe(struct i2c_client *, const struct i2c_device_id *);
static int __devexit i2c_ak8973b_remove(struct i2c_client *);
static int i2c_ak8973b_detect(struct i2c_client *, int kind, struct i2c_board_info *);

unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { I2C_CLIENT_END };
static unsigned short probe_addr[] = { 1, E_COMPASS_ADDRESS, I2C_CLIENT_END };


static struct i2c_client_address_data addr_data = {
	.normal_i2c     = normal_addr,
	.probe          = probe_addr,
	.ignore         = ignore,
};

//
// id table for new style i2c driver
static struct i2c_device_id ak8973b_id[] = {
	{ "ak8973b", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, ak8973b_id);

static struct i2c_driver ak8973b_i2c_driver = {
	.driver = {
		   .name = "ak8973b",
		   },
	.class			= I2C_CLASS_HWMON,
	.probe 			= i2c_ak8973b_probe,
	.remove			= __devexit_p(i2c_ak8973b_remove),
	.detect			= i2c_ak8973b_detect,
	.id_table		= ak8973b_id,
	.address_data	= &addr_data,
};

static char ak_e2prom_data[3];

static int AKI2C_RxData(char *rxData, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = rxData,
		 },
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	if (i2c_transfer(this_client->adapter, msgs, 2) < 0) {
		printk(KERN_ERR "AKI2C_RxData: transfer error \n");
		return -EIO;
	} else
		return 0;
}

static int AKI2C_TxData(char *txData, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	if (i2c_transfer(this_client->adapter, msg, 1) < 0) {
		printk(KERN_ERR "AKI2C_TxData: transfer error addr \n");
		return -EIO;
	} else
		return 0;
}

static int AKECS_Init(void)
{
	return 0;
}

static int akm_aot_open(struct inode *inode, struct file *file)
{
	int ret = -1;
	if (atomic_cmpxchg(&open_count, 0, 1) == 0) {
		if (atomic_cmpxchg(&open_flag, 0, 1) == 0) {
			atomic_set(&reserve_open_flag, 1);
			wake_up(&open_wq);
			ret = 0;
		}
	}
	return ret;
}

static int akm_aot_release(struct inode *inode, struct file *file)
{
	atomic_set(&reserve_open_flag, 0);
	atomic_set(&open_flag, 0);
	atomic_set(&open_count, 0);
	wake_up(&open_wq);
	return 0;
}

static void AKECS_Report_Value(short *rbuf)
{
short ax, ay, az, mx, my, mz;	

	struct ak8973b_data *data = i2c_get_clientdata(this_client);
	
	#if 0
	printk("Orientaion: yaw = %d, pitch = %d, roll = %d\n", rbuf[0],
			rbuf[1], rbuf[2]);
	printk("tmp = %d, m_stat= %d, g_stat=%d\n", rbuf[3],
			rbuf[4], rbuf[5]);
	printk("Acceleration:   x = %d LSB, y = %d LSB, z = %d LSB\n",
			rbuf[6], rbuf[7], rbuf[8]);
	printk("Magnetic:   x = %d LSB, y = %d LSB, z = %d LSB\n\n",
			rbuf[9], rbuf[10], rbuf[11]);
	#endif
	/*if flag is set, execute report */
	/* Report Orientation information */
	if (atomic_read(&m_flag)) {
		short AzConst = 180 * 64;
		short azimut = rbuf[0];
		short azimut_conv = ((azimut < AzConst) ? azimut + AzConst : azimut - AzConst);
		input_report_abs(data->input_dev, ABS_RX, azimut_conv);
//		input_report_abs(data->input_dev, ABS_RX, rbuf[0]);
		input_report_abs(data->input_dev, ABS_RY, -rbuf[1]);
		input_report_abs(data->input_dev, ABS_RZ, -rbuf[2]);
		input_report_abs(data->input_dev, ABS_RUDDER, rbuf[4]);
	}

	/* Report acceleration sensor information */
	ax = rbuf[AX];
	ay = rbuf[AY];
	az = rbuf[AZ];
	if ( accel_inp_dev_sign & 4)  ax *= -1;
	if ( accel_inp_dev_sign & 2)  ay *= -1;
	if ( accel_inp_dev_sign & 1)  az *= -1;

	if (atomic_read(&a_flag)) {
		input_report_abs(data->input_dev, ABS_X, ax);
		input_report_abs(data->input_dev, ABS_Y, ay);
		input_report_abs(data->input_dev, ABS_Z, az);
		input_report_abs(data->input_dev, ABS_WHEEL, rbuf[5]);
	}

	/* Report temperature information */
	if (atomic_read(&t_flag)) {
		input_report_abs(data->input_dev, ABS_THROTTLE, rbuf[3]);
	}

	/* Report magnetic sensor information */
	mx = rbuf[MX];
	my = rbuf[MY];
	mz = rbuf[MZ];
	if ( mag_inpf_sign & 4)  mx *= -1;
	if ( mag_inpf_sign & 2)  my *= -1;
	if ( mag_inpf_sign & 1)  mz *= -1;

	if (atomic_read(&mv_flag)) {
		input_report_abs(data->input_dev, ABS_HAT0X, mx);
		input_report_abs(data->input_dev, ABS_HAT0Y, my);
		input_report_abs(data->input_dev, ABS_BRAKE, mz);
	}
	/* Report proximity information */
	if (atomic_read(&p_flag)) {
		rbuf[12]=gp2a_get_proximity_value();
		gprintk("Proximity = %d\n", rbuf[12]);
		input_report_abs(data->input_dev, ABS_DISTANCE, rbuf[12]);
	}
	
	input_sync(data->input_dev);
}

static int AKECS_GetOpenStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
	return atomic_read(&open_flag);
}

static int AKECS_GetCloseStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) <= 0));
	return atomic_read(&open_flag);
}

static void AKECS_CloseDone(void)
{
	#if 0
	atomic_set(&m_flag, 1);
	atomic_set(&a_flag, 1);
	atomic_set(&t_flag, 1);
	atomic_set(&mv_flag, 1);
    atomic_set(&p_flag, 1);
	#else
	atomic_set(&m_flag, 0);
	atomic_set(&a_flag, 0);
	atomic_set(&t_flag, 0);
	atomic_set(&mv_flag, 0);
    atomic_set(&p_flag, 0);
	#endif
}



/*----------------------------------------------------------------------------*/
//Description : Resetting AKECS 
//WHO : AKEK GWLEE
//DATE : 2008 12 08
/*----------------------------------------------------------------------------*/
static void AKECS_Reset (void)
{
      
	gpio_set_value(GPIO_MSENSE_RST, GPIO_LEVEL_LOW);
	udelay(120);
	gpio_set_value(GPIO_MSENSE_RST, GPIO_LEVEL_HIGH);
	gprintk("[ak8973b] RESET COMPLETE\n");
}

/*----------------------------------------------------------------------------*/
//Description : Setting measurement to AKECS 
//WHO : AKEK GWLEE
//DATE : 2008 12 08
/*----------------------------------------------------------------------------*/
static int AKECS_SetMeasure (void)
{
	char buffer[2];

	gprintk("MEASURE MODE\n");
	/* Set measure mode */
	buffer[0] = AKECS_REG_MS1;
	buffer[1] = AKECS_MODE_MEASURE;

	/* Set data */
	return AKI2C_TxData(buffer, 2);
}

/*----------------------------------------------------------------------------*/
//Description : Setting EEPROM to AKECS 
//WHO : AKEK GWLEE
//DATE : 2008 12 08
/*----------------------------------------------------------------------------*/
static int AKECS_SetE2PRead ( void )
{
	char buffer[2];

	/* Set measure mode */
	buffer[0] = AKECS_REG_MS1;
	buffer[1] = AKECS_MODE_E2P_READ;
	/* Set data */
	return AKI2C_TxData(buffer, 2);
}


/*----------------------------------------------------------------------------*/
//Description : Power Down to AKECS 
//WHO : AKEK GWLEE
//DATE : 2008 12 08
/*----------------------------------------------------------------------------*/
static int AKECS_PowerDown (void)
{
	char buffer[2];
	int ret;

	/* Set powerdown mode */
	buffer[0] = AKECS_REG_MS1;
	buffer[1] = AKECS_MODE_POWERDOWN;
	/* Set data */
	ret = AKI2C_TxData(buffer, 2);
	if (ret < 0)
		return ret;

	/* Dummy read for clearing INT pin */
	buffer[0] = AKECS_REG_TMPS;
	/* Read data */
	ret = AKI2C_RxData(buffer, 1);
	if (ret < 0)
		return ret;

 return ret;
}

/*----------------------------------------------------------------------------*/
//Description : Get EEPROM Data to AKECS 
//WHO : AKEK GWLEE
//DATE : 2008 12 08
/*----------------------------------------------------------------------------*/
static int AKECS_GetEEPROMData (void)
{ 
	int ret;
	char buffer[RBUFF_SIZE + 1];

	ret = AKECS_SetE2PRead();
	if (ret < 0) return ret;

	memset(buffer, 0, RBUFF_SIZE + 1);
	buffer[0] = AKECS_EEP_EHXGA;
	ret =  AKI2C_RxData(buffer, 3);

	if (ret < 0) return ret;

	ak_e2prom_data[0]= buffer[0];
	ak_e2prom_data[1]= buffer[1];
	ak_e2prom_data[2]= buffer[2];
	gprintk("AKE2PROM_Data -->%d , %d, %d ----\n", ak_e2prom_data[0],ak_e2prom_data[1],ak_e2prom_data[2]);
	
	return ret;
}

static int AKECS_SetMode(char mode)
{
	int ret = 0;
	
	switch (mode) {
		case AKECS_MODE_MEASURE:
			ret = AKECS_SetMeasure();
			break;
		case AKECS_MODE_E2P_READ:
			ret = AKECS_SetE2PRead();
			break;
		case AKECS_MODE_POWERDOWN:
			ret = AKECS_PowerDown();
			break;
		default:
			return -EINVAL;
	}

	/* wait at least 300us after changing mode */
	msleep(1);
	return ret;
}

static int AKECS_TransRBuff(char *rbuf, int size)
{
	if(size < RBUFF_SIZE + 1)
		return -EINVAL;

	// read C0 - C4
	rbuf[0] = AKECS_REG_ST;
	return AKI2C_RxData(rbuf, RBUFF_SIZE + 1);

}


/*----------------------------------------------------------------------------*/
//Description : Get EEPROM Data to AKECS 
//WHO : AKEK GWLEE
//DATE : 2008 12 08
/*----------------------------------------------------------------------------*/
static int AKECS_GetData (short *rbuf)
{ 
	int ret;
  	char buffer[RBUFF_SIZE + 1];

	memset(buffer, 0, RBUFF_SIZE + 1);
	buffer[0] = AKECS_REG_TMPS;

	ret =  AKI2C_RxData(buffer, 4);  //temp, x, y, z
	if(ret<0) {
		printk(KERN_ERR "AKECS_GetData  failed--------------\n");
		return ret;
		}
	else	{
			
			rbuf[0] = 35 + (120 -buffer[0])*10/16;
			rbuf[1] = buffer[1];
			rbuf[2] = buffer[2];
			rbuf[3] = buffer[3];
			gprintk("AKECS_Get_MAG: temp = %d, x = %d, y = %d, z = %d\n", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
		}
  return ret;
}

static void AKECS_DATA_Measure(void)
{
	short  value[13];
	short mag_sensor[4] ={0, 0, 0, 0};

	AKECS_GetData(mag_sensor);
	value[0]=0;
	value[1]=0;
	value[2]=0;
	value[3]=mag_sensor[0];		/* temparature */
	value[4]=0;
	value[5]=0;
	value[6]=0;
	value[7]=0;
	value[8]=0;
	value[9]=mag_sensor[1];		/* mag_x */
	value[10]=mag_sensor[2];	/* mag_y */
	value[11]=mag_sensor[3];	/* mag_z */
	value[12]=gp2a_get_proximity_value();

	AKECS_Report_Value(value);
}


static int 
akm_aot_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
#if 0
	struct ak8973b_data *akm = i2c_get_clientdata(this_client);

	switch(cmd)
	{
		case EMCS_IOCTL_OPEN:
			gprintk("hrtimer_start!\n");
			ak8973b_timer_oper = TIMER_ON;
			hrtimer_start(&akm->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
			break;
		case EMCS_IOCTL_CLOSE:
			gprintk("hrtimer_cancel!\n");
			ak8973b_timer_oper = TIMER_OFF;
			hrtimer_cancel(&akm->timer);
			break;
		default:
			break;
	}
#endif

	void __user *argp = (void __user *)arg;
	short flag;

	switch (cmd) {
		case ECS_IOCTL_APP_SET_MFLAG:
		case ECS_IOCTL_APP_SET_AFLAG:
		case ECS_IOCTL_APP_SET_TFLAG:
		case ECS_IOCTL_APP_SET_MVFLAG:
		case ECS_IOCTL_APP_SET_PFLAG:
			if (copy_from_user(&flag, argp, sizeof(flag)))
				return -EFAULT;
			if (flag < 0 || flag > 1)
				return -EINVAL;
			break;
		case ECS_IOCTL_APP_SET_DELAY:
			if (copy_from_user(&flag, argp, sizeof(flag)))
				return -EFAULT;
			break;
		default:
			break;
	}

	switch (cmd) {
		case ECS_IOCTL_APP_SET_MFLAG:
			atomic_set(&m_flag, flag);
			break;
		case ECS_IOCTL_APP_GET_MFLAG:
			flag = atomic_read(&m_flag);
			break;
		case ECS_IOCTL_APP_SET_AFLAG:
			atomic_set(&a_flag, flag);
			break;
		case ECS_IOCTL_APP_GET_AFLAG:
			flag = atomic_read(&a_flag);
			break;
		case ECS_IOCTL_APP_SET_TFLAG:
			atomic_set(&t_flag, flag);
			break;
		case ECS_IOCTL_APP_GET_TFLAG:
			flag = atomic_read(&t_flag);
			break;
		case ECS_IOCTL_APP_SET_MVFLAG:
			atomic_set(&mv_flag, flag);
			break;
		case ECS_IOCTL_APP_GET_MVFLAG:
			flag = atomic_read(&mv_flag);
			break;
		case ECS_IOCTL_APP_SET_PFLAG:
			atomic_set(&p_flag, flag);
			break;
		case ECS_IOCTL_APP_GET_PFLAG:
			flag = atomic_read(&p_flag);
			break;
		case ECS_IOCTL_APP_SET_DELAY:
			akmd_delay = flag;
			break;
		case ECS_IOCTL_APP_GET_DELAY:
			flag = akmd_delay;
			break;
		case ECS_IOCTL_DEVMGR_GET_DATA:
			AKECS_DATA_Measure();
			break;
		default:
			return -ENOTTY;
	}

	switch (cmd) {
		case ECS_IOCTL_APP_GET_MFLAG:
		case ECS_IOCTL_APP_GET_AFLAG:
		case ECS_IOCTL_APP_GET_TFLAG:
		case ECS_IOCTL_APP_GET_MVFLAG:
		case ECS_IOCTL_APP_GET_PFLAG:
		case ECS_IOCTL_APP_GET_DELAY:
			if (copy_to_user(argp, &flag, sizeof(flag)))
				return -EFAULT;
			break;
		default:
			break;
	}

	return 0;
}

static int akmd_open(struct inode *inode, struct file *file)
{
	gprintk(KERN_INFO "[AK8973B] %s\n", __FUNCTION__);
	return nonseekable_open(inode, file);
}

static int akmd_release(struct inode *inode, struct file *file)
{
	gprintk(KERN_INFO "[AK8973B] %s\n", __FUNCTION__);
	AKECS_CloseDone();
	return 0;
}

	static int
akmd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		unsigned long arg)
{
	int i;
	void __user *argp = (void __user *)arg;

	char msg[RBUFF_SIZE + 1], rwbuf[16];//, numfrq[2];
	int ret = -1, status;
	short mode, value[12], delay; /* step_count,*/
	//  char *pbuffer = 0;
	gprintk("start\n");

	switch (cmd) {
#if 1 //bss to find the correct settings		
		case 10:
			Makm_sign = 0;
			break;

		case 11:
			Makm_sign = 1;
			break;

		case 12:
			Makm_sign = 2;
			break;

		case 13:
			Makm_sign = 3;
			break;

		case 14:
			Makm_sign = 4;
			break;

		case 15:
			Makm_sign = 5;
			break;

		case 16:
			Makm_sign = 6;
			break;

		case 17:
			Makm_sign = 7;
			break;

#ifdef PHONE_B7610
		case 20:
			MakmY = O_MakmX;
			MakmX = O_MakmY;
			MakmZ = O_MakmZ;
			break;

		case 21:
			MakmY = O_MakmX;
			MakmX = O_MakmZ;
			MakmZ = O_MakmY;
			break;

		case 22:
			MakmY = O_MakmY;
			MakmX = O_MakmX;
			MakmZ = O_MakmZ;
			break;

		case 23:
			MakmY = O_MakmY;
			MakmX = O_MakmZ;
			MakmZ = O_MakmX;
			break;

		case 24:
			MakmY = O_MakmZ;
			MakmX = O_MakmX;
			MakmZ = O_MakmY;
			break;

		case 25:
			MakmY = O_MakmZ;
			MakmX = O_MakmY;
			MakmZ = O_MakmX;
			break;
#else
		case 20:
			MakmX = O_MakmX;
			MakmY = O_MakmY;
			MakmZ = O_MakmZ;
			break;

		case 21:
			MakmX = O_MakmX;
			MakmY = O_MakmZ;
			MakmZ = O_MakmY;
			break;

		case 22:
			MakmX = O_MakmY;
			MakmY = O_MakmX;
			MakmZ = O_MakmZ;
			break;

		case 23:
			MakmX = O_MakmY;
			MakmY = O_MakmZ;
			MakmZ = O_MakmX;
			break;

		case 24:
			MakmX = O_MakmZ;
			MakmY = O_MakmX;
			MakmZ = O_MakmY;
			break;

		case 25:
			MakmX = O_MakmZ;
			MakmY = O_MakmY;
			MakmZ = O_MakmX;
			break;
#endif
		case 110:
			accel_inp_dev_sign = 0;
			break;

		case 111:
			accel_inp_dev_sign = 1;
			break;

		case 112:
			accel_inp_dev_sign = 2;
			break;

		case 113:
			accel_inp_dev_sign = 3;
			break;

		case 114:
			accel_inp_dev_sign = 4;
			break;

		case 115:
			accel_inp_dev_sign = 5;
			break;

		case 116:
			accel_inp_dev_sign = 6;
			break;

		case 117:
			accel_inp_dev_sign = 7;
			break;
#ifdef PHONE_B7610
		case 120:
			AY = O_AX;
			AX = -O_AY;
			AZ = -O_AZ;
			break;

		case 121:
			AY = O_AX;
			AX = -O_AZ;
			AZ = -O_AY;
			break;

		case 122:
			AY = O_AY;
			AX = -O_AX;
			AZ = -O_AZ;
			break;

		case 123:
			AY = O_AY;
			AX = -O_AZ;
			AZ = -O_AX;
			break;

		case 124:
			AY = O_AZ;
			AX = -O_AX;
			AZ = -O_AY;
			break;

		case 125:
			AY = O_AZ;
			AX = -O_AY;
			AZ = -O_AX;
			break;

#else
		case 120:
			AX = O_AX;
			AY = O_AY;
			AZ = O_AZ;
			break;

		case 121:
			AX = O_AX;
			AY = O_AZ;
			AZ = O_AY;
			break;

		case 122:
			AX = O_AY;
			AY = O_AX;
			AZ = O_AZ;
			break;

		case 123:
			AX = O_AY;
			AY = O_AZ;
			AZ = O_AX;
			break;

		case 124:
			AX = O_AZ;
			AY = O_AX;
			AZ = O_AY;
			break;

		case 125:
			AX = O_AZ;
			AY = O_AY;
			AZ = O_AX;
			break;
#endif
		case 210:
			mag_inpf_sign = 1;
			break;

		case 211:
			mag_inpf_sign = 1;
			break;

		case 212:
			mag_inpf_sign = 2;
			break;

		case 213:
			mag_inpf_sign = 3;
			break;

		case 214:
			mag_inpf_sign = 4;
			break;

		case 215:
			mag_inpf_sign = 5;
			break;

		case 216:
			mag_inpf_sign = 6;
			break;

		case 217:
			mag_inpf_sign = 7;
			break;
#ifdef PHONE_B7610
		case 220:
			MX = O_MX;
			MY = -O_MY;
			MZ = -O_MZ;
			break;

		case 221:
			MX = O_MX;
			MY = -O_MZ;
			MZ = -O_MY;
			break;

		case 222:
			MX = O_MY;
			MY = -O_MX;
			MZ = -O_MZ;
			break;

		case 223:
			MX = O_MY;
			MY = -O_MZ;
			MZ = -O_MX;
			break;

		case 224:
			MX = O_MZ;
			MY = -O_MX;
			MZ = -O_MY;
			break;

		case 225:
			MX = O_MZ;
			MY = -O_MY;
			MZ = -O_MX;
			break;
#else
		case 220:
			MX = O_MX;
			MY = O_MY;
			MZ = O_MZ;
			break;

		case 221:
			MX = O_MX;
			MY = O_MZ;
			MZ = O_MY;
			break;

		case 222:
			MX = O_MY;
			MY = O_MX;
			MZ = O_MZ;
			break;

		case 223:
			MX = O_MY;
			MY = O_MZ;
			MZ = O_MX;
			break;

		case 224:
			MX = O_MZ;
			MY = O_MX;
			MZ = O_MY;
			break;

		case 225:
			MX = O_MZ;
			MY = O_MY;
			MZ = O_MX;
			break;

#endif

		case 300:
			print_flag = 0;
			break;
		case 301:
			print_flag = 1;
			break;

#endif			
		case ECS_IOCTL_READ:
		case ECS_IOCTL_WRITE:
			if (copy_from_user(&rwbuf, argp, sizeof(rwbuf)))
				return -EFAULT;
			break;
		case ECS_IOCTL_SET_MODE:
			if (copy_from_user(&mode, argp, sizeof(mode)))
				return -EFAULT;
			break;
		case ECS_IOCTL_SET_YPR:
			if (copy_from_user(&value, argp, sizeof(value)))
				return -EFAULT;
			break;
			//case ECS_IOCTL_SET_STEP_CNT:
			// if (copy_from_user(&step_count, argp, sizeof(step_count)))
			//     return -EFAULT;
			// break;
		default:
			break;
	}

	switch (cmd) {
		case ECS_IOCTL_INIT:
			gprintk("[AK8973B] ECS_IOCTL_INIT %x\n", cmd);
			ret = AKECS_Init();
			if (ret < 0)
				return ret;
			break;
		case ECS_IOCTL_RESET:
			gprintk("[AK8973B] ECS_IOCTL_RESET %x\n", cmd);
			AKECS_Reset();
			break;
		case ECS_IOCTL_READ:
			gprintk("[AK8973B] ECS_IOCTL_READ %x\n", cmd);
			gprintk(" len %02x:", rwbuf[0]);
			gprintk(" addr %02x:", rwbuf[1]);
			gprintk("\n");
			if (rwbuf[0] < 1)
				return -EINVAL;
			ret = AKI2C_RxData(&rwbuf[1], rwbuf[0]);
			if (print_flag)  
				printk("Ak orig sensor X: %d, Y: %d, Z: %d\n", 
						rwbuf[O_MakmX], rwbuf[O_MakmY], rwbuf[O_MakmZ]);
#if 1  //bss 
			{ char akmX, akmY, akmZ;
				akmX = rwbuf[MakmX];
				akmY = rwbuf[MakmY];
				akmZ = rwbuf[MakmZ];
				if ( Makm_sign & 4) akmX *= -1;
				if ( Makm_sign & 2) akmY *= -1;
				if ( Makm_sign & 1) akmZ *= -1;
				rwbuf[O_MakmX] = akmX;
				rwbuf[O_MakmY] = akmY;
				rwbuf[O_MakmZ] = akmZ;
			}
#endif 

#if 0
//bss for i6500 eclair akmd2 on onmia_II
			// for akmd, must revert and re-sign the x,y values: x=-y, y=x!!
			{
				char ch = rwbuf[akmX];
				rwbuf[akmX] = rwbuf[akmY];
				rwbuf[akmY] = ch;
			}
#endif		
			for(i=0; i<rwbuf[0]; i++){
				gprintk(" %02x", rwbuf[i+1]);
			}
			gprintk(" ret = %d\n", ret);
			if (ret < 0)
				return ret;
			break;
		case ECS_IOCTL_WRITE:
			gprintk("[AK8973B] ECS_IOCTL_WRITE %x\n", cmd);
			gprintk(" len %02x:", rwbuf[0]);
			for(i=0; i<rwbuf[0]; i++){
				gprintk(" %02x", rwbuf[i+1]);
			}
			gprintk("\n");
			if (rwbuf[0] < 2)
				return -EINVAL;

#if 0  //bss 
			{ char akmX, akmY, akmZ;
				akmX = rwbuf[MakmX];
				akmY = rwbuf[MakmY];
				akmZ = rwbuf[MakmZ];
				if ( Makm_sign & 4) akmX *= -1;
				if ( Makm_sign & 2) akmY *= -1;
				if ( Makm_sign & 1) akmZ *= -1;
				rwbuf[O_MakmX] = akmX;
				rwbuf[O_MakmY] = akmY;
				rwbuf[O_MakmZ] = akmZ;
			}
#endif 


#if 0
//bss for i6500 eclair akmd2 on onmia_II
			if (rwbuf[0] == 4) {
				// the calibratin feedback must be exchanged too  x=y, y=-x !!
				char ch = rwbuf[akmX];
				rwbuf[akmX] = rwbuf[akmY];
				rwbuf[akmY] = ch;
			}			
#endif 
			ret = AKI2C_TxData(&rwbuf[1], rwbuf[0]);
			gprintk(" ret = %d\n", ret);
			if (ret < 0)
				return ret;
			break;
		case ECS_IOCTL_SET_MODE:
			gprintk("[AK8973B] ECS_IOCTL_SET_MODE %x mode=%x\n", cmd, mode);
			ret = AKECS_SetMode((char)mode);
			gprintk(" ret = %d\n", ret);
			if (ret < 0)
				return ret;
			break;
		case ECS_IOCTL_GETDATA:
			gprintk("[AK8973B] ECS_IOCTL_GETDATA %x\n", cmd);
			ret = AKECS_TransRBuff(msg, RBUFF_SIZE+1);
			gprintk(" ret = %d\n", ret);
			if (ret < 0)
				return ret;
			for(i=0; i<ret; i++){
				gprintk(" %02x", msg[i]);
			}
			break;
		case ECS_IOCTL_SET_YPR:
			gprintk("[AK8973B] ECS_IOCTL_SET_YPR %x ypr=%x\n", cmd, (unsigned int)value);
			AKECS_Report_Value(value);
			break;
		case ECS_IOCTL_GET_OPEN_STATUS:
			gprintk("[AK8973B] ECS_IOCTL_GET_OPEN_STATUS %x start\n", cmd);
			status = AKECS_GetOpenStatus();
			gprintk("[AK8973B] ECS_IOCTL_GET_OPEN_STATUS %x end status=%x\n", cmd, status);

			break;
		case ECS_IOCTL_GET_CLOSE_STATUS:
			gprintk("[AK8973B] ECS_IOCTL_GET_CLOSE_STATUS %x start\n", cmd);
			status = AKECS_GetCloseStatus();
			gprintk("[AK8973B] ECS_IOCTL_GET_CLOSE_STATUS %x end status=%x\n", cmd, status);
			break;
		case ECS_IOCTL_GET_DELAY:
			delay = akmd_delay;
			gprintk("[AK8973B] ECS_IOCTL_GET_DELAY %x delay=%x\n", cmd, delay);
			break;
		default:
			gprintk("Unknown cmd %x\n", cmd);
			return -ENOTTY;
	}

	switch (cmd) {
		case ECS_IOCTL_READ:
			if (copy_to_user(argp, &rwbuf, sizeof(rwbuf)))
				return -EFAULT;
			break;
		case ECS_IOCTL_GETDATA:
			if (copy_to_user(argp, &msg, sizeof(msg)))
				return -EFAULT;
			break;
		case ECS_IOCTL_GET_OPEN_STATUS:
		case ECS_IOCTL_GET_CLOSE_STATUS:
			if (copy_to_user(argp, &status, sizeof(status)))
				return -EFAULT;
			break;

		case ECS_IOCTL_GET_DELAY:
			if (copy_to_user(argp, &delay, sizeof(delay)))
				return -EFAULT;
			break;
		default:
			break;
	}

	return 0;
}




#ifdef CONFIG_HAS_EARLYSUSPEND
static void ak8973b_early_suspend(struct early_suspend *handler)
{
	printk("[COMPASS] %s\n", __func__);
	AKECS_SetMode(AKECS_MODE_POWERDOWN);
	#if 0
	atomic_set(&suspend_flag, 1);
	if (atomic_read(&open_flag) == 2)
		AKECS_SetMode(AKECS_MODE_POWERDOWN);

	atomic_set(&reserve_open_flag, atomic_read(&open_flag));
	atomic_set(&open_flag, 0);
	wake_up(&open_wq);
	#endif
}

static void ak8973b_late_resume(struct early_suspend *handler)
{
	printk("[COMPASS] %s\n", __func__);
	atomic_set(&suspend_flag, 0);
	atomic_set(&open_flag, atomic_read(&reserve_open_flag));
	wake_up(&open_wq);
}
#endif /* CONFIG_HAS_EARLYSUSPEND */ 

static void ak8973b_init_hw(void)
{
#if 0	
	s3c_gpio_cfgpin(GPIO_MSENSE_INT, S3C_GPIO_SFN(GPIO_MSENSE_INT_AF));
	s3c_gpio_setpull(GPIO_MSENSE_INT, S3C_GPIO_PULL_NONE);
	set_irq_type(IRQ_COMPASS_INT, IRQ_TYPE_EDGE_RISING);
#endif

	if(gpio_is_valid(GPIO_MSENSE_RST)){
		if(gpio_request(GPIO_MSENSE_RST, S3C_GPIO_LAVEL(GPIO_MSENSE_RST)))
		{
			printk(KERN_ERR "Failed to request GPIO_MSENSE_RST!\n");
		}
		gpio_direction_output(GPIO_MSENSE_RST, GPIO_LEVEL_HIGH);
	}
	s3c_gpio_setpull(GPIO_MSENSE_RST, S3C_GPIO_PULL_NONE);

	gprintk("gpio setting complete!\n");
}

static struct file_operations akmd_fops = {
	.owner = THIS_MODULE,
	.open = akmd_open,
	.release = akmd_release,
	.ioctl = akmd_ioctl,
};

static struct file_operations akm_aot_fops = {
	.owner = THIS_MODULE,
	.open = akm_aot_open,
	.release = akm_aot_release,
	.ioctl = akm_aot_ioctl,
};

static struct miscdevice akm_aot_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "akm8973_aot",
	.fops = &akm_aot_fops,
};

static struct miscdevice akmd_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "akm8973_daemon",
	.fops = &akmd_fops,
};


static int __devinit i2c_ak8973b_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;
	struct ak8973b_data *akm;

	gprintk("start\n");

	if ( !i2c_check_functionality(client->adapter,I2C_FUNC_SMBUS_BYTE_DATA) ) {
		printk(KERN_INFO "byte op is not permited.\n");
		goto ERROR0;
	}

	akm = kzalloc(sizeof(struct ak8973b_data), GFP_KERNEL);
	if (!akm) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	client->addr = E_COMPASS_ADDRESS;
	client->driver = &ak8973b_i2c_driver;
	client->flags = I2C_DF_NOTIFY | I2C_M_IGNORE_NAK;
	client->irq = IRQ_COMPASS_INT; /* EINT(2) */

	i2c_set_clientdata(client, akm);
	
	this_client = client;

	if(this_client == NULL)
	{
		printk("i2c_client is NULL\n");
		return -ENODEV;
	}

	akm->input_dev = input_allocate_device();

	if (!akm->input_dev) {
		err = -ENOMEM;
		printk("input_allocate_device failed\n");
		goto exit_input_dev_alloc_failed;
	}
	
	set_bit(EV_ABS, akm->input_dev->evbit);
	set_bit(EV_SYN, akm->input_dev->evbit);

	/* yaw */
	input_set_abs_params(akm->input_dev, ABS_RX, 0, 23040, 0, 0);
	/* pitch */
	input_set_abs_params(akm->input_dev, ABS_RY, -11520, 11520, 0, 0);
	/* roll */
	input_set_abs_params(akm->input_dev, ABS_RZ, -5760, 5760, 0, 0);
	/* x-axis acceleration */
	input_set_abs_params(akm->input_dev, ABS_X, -5760, 5760, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(akm->input_dev, ABS_Y, -5760, 5760, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(akm->input_dev, ABS_Z, -5760, 5760, 0, 0);
	/* temparature */
	input_set_abs_params(akm->input_dev, ABS_THROTTLE, -30, 85, 0, 0);
	/* status of magnetic sensor */
	input_set_abs_params(akm->input_dev, ABS_RUDDER, 0, 3, 0, 0);
	/* status of acceleration sensor */
	input_set_abs_params(akm->input_dev, ABS_WHEEL, 0, 3, 0, 0);
	/* x-axis of raw magnetic vector */
	input_set_abs_params(akm->input_dev, ABS_HAT0X, -2048, 2032, 0, 0);
	/* y-axis of raw magnetic vector */
	input_set_abs_params(akm->input_dev, ABS_HAT0Y, -2048, 2032, 0, 0);
	/* z-axis of raw magnetic vector */
	input_set_abs_params(akm->input_dev, ABS_BRAKE, -2048, 2032, 0, 0);
	/* proximity sensor */	
	input_set_abs_params(akm->input_dev, ABS_DISTANCE, -3, 3, 0, 0);
	
	akm->input_dev->name = "compass";

	err = input_register_device(akm->input_dev);
	if (err) {
		printk("Unable to register input device: %s\n",akm->input_dev->name);
		goto exit_input_register_device_failed;
	}

	err = misc_register(&akmd_device);
	if (err) {
		printk(KERN_ERR "akm8973_probe: akmd_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = misc_register(&akm_aot_device);
	if (err) {
		printk("akm_aot_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = device_create_file(&client->dev, &dev_attr_ms1);

	AKECS_GetEEPROMData();
		
	AKECS_SetMode(AKECS_MODE_POWERDOWN);
	udelay(100);


#ifdef CONFIG_HAS_EARLYSUSPEND
	akm->early_suspend.suspend = ak8973b_early_suspend;
	akm->early_suspend.resume = ak8973b_late_resume;
	register_early_suspend(&akm->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */ 
	
	printk("[AK8973B] COMPLETE PROBE\n");	
	return 0;

exit_misc_device_register_failed:
exit_input_register_device_failed:
	input_free_device(akm->input_dev);
exit_input_dev_alloc_failed:
	kfree(akm);
exit_alloc_data_failed:
	return err;

ERROR0:
	printk("%s ERROR0\n",__func__);/* add by inter.park */
	return err;
}

static int i2c_ak8973b_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
	strlcpy(info->type, "ak8973b", I2C_NAME_SIZE);
	return 0;
}

static int __devexit i2c_ak8973b_remove(struct i2c_client *client)
{
	struct ak8973b_data *akm = i2c_get_clientdata(client);
	
	input_unregister_device(akm->input_dev);

	misc_deregister(&akm_aot_device);
	misc_deregister(&akmd_device);
	
	kfree(akm);
	akm = NULL;	
	this_client = NULL;
	
	gpio_free(GPIO_MSENSE_RST);
	
	gprintk("end\n");
	return 0;
}


static int __init ak8973b_init(void)
{
	int ret = 0;
	gprintk("__start\n");
	ak8973b_init_hw();
	AKECS_Reset(); /*module reset */
	udelay(200);

	if((ret = i2c_add_driver(&ak8973b_i2c_driver)) < 0)
	{
		printk("[AK8973B] i2c_add_driver failed\n");
		return ret;
	}
	
	if(this_client == NULL)
	{
		printk("[AK8973B] this_client is NULL\n");
	}
	
	return ret;
}

static void __exit ak8973b_exit(void)
{
	gprintk("__exit\n");
	i2c_del_driver(&ak8973b_i2c_driver);
}

module_init(ak8973b_init);
module_exit(ak8973b_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_DESCRIPTION("AK8973B compass driver");
MODULE_LICENSE("GPL");
