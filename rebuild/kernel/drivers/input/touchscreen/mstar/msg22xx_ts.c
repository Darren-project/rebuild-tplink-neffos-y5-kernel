/*
* Mstar MSG22XX touchscreen driver
*
* Copyright (c) 2006-2012 MStar Semiconductor, Inc.
*
* Copyright (C) 2012 Bruce Ding <bruce.ding@mstarsemi.com>
*
* Copyright (c) 2014, The Linux Foundation. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/regulator/consumer.h>
#include <linux/spinlock.h>
#include <linux/input/msg22xx_ts.h>

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
#ifdef CONFIG_TOUCHSCREEN_PROXIMITY_SENSOR
#include <linux/input/vir_ps.h>
#endif

#undef CONFIG_MSG22XX_DEBUG

#ifdef CONFIG_MSG22XX_DEBUG
#define dprintk(msg...)	printk(KERN_INFO "msg22xx: " msg)
#else
#define dprintk(msg...)
#endif

#define CONFIG_ENABLE_TYPE_B_PROTOCOL       //use B(slot) protocol

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
#define SELF_MAX_TOUCH_NUM           (2)                    // for MSG21xxA/MSG22xx
#define MAX_TOUCH_NUM                SELF_MAX_TOUCH_NUM     // for MSG21xxA/MSG22xx
static unsigned char _gCurrPress[SELF_MAX_TOUCH_NUM] = {0}; // for MSG21xxA/MSG22xx
static unsigned char _gPrevTouchStatus = 0;

//static unsigned char _gPreviousTouch[MUTUAL_MAX_TOUCH_NUM] = {0}; // for MSG26xxM/MSG28xx
//static unsigned char _gCurrentTouch[MUTUAL_MAX_TOUCH_NUM] = {0};
static unsigned int _DrvFwCtrlPointDistance(unsigned short nX, unsigned short nY, unsigned short nPrevX, unsigned short nPrevY);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

/* gesture */
static unsigned char msg22xx_gesture_mode = 0; /* enable/disable gesture*/
static bool ts_gesture_wakeup_device_enable = false;
static unsigned int ts_gesture_wakeup_mode[2] = {0xFFFFFFFF, 0xFFFFFFFF};
static unsigned char data_normal[DEMO_MODE_PACKET_LENGTH] = {0};
static unsigned char data_gesture[GESTURE_WAKEUP_PACKET_LENGTH] = {0};
static unsigned char ts_irq_flag = 0; /* flag to match disable and enable irq */
static unsigned char ts_resume_suspend_flag = 0; /* flag to match resume and suspend */

/* global pointer */
static struct msg22xx_ts_data *msg22xx_data;
static struct mutex msg22xx_mutex;
static spinlock_t msg22xx_irqlock;
Msg22xxSwId_e g_SwId = MSG22XX_SW_ID_UNDEFINED;

static unsigned char g_OneDimenFwData[MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE*1024+MSG22XX_FIRMWARE_INFO_BLOCK_SIZE] = {0}; // used for MSG22XX
static unsigned char g_FwData[MAX_UPDATE_FIRMWARE_BUFFER_SIZE][1024];
static unsigned int g_FwDataCount = 0;
static unsigned char g_IsUpdateFirmware = 0x00;
static unsigned int g_IsUpdateInfoBlockFirst = 0;
static unsigned int g_UpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
static struct work_struct g_UpdateFirmwareBySwIdWork;
static struct workqueue_struct *g_UpdateFirmwareBySwIdWorkQueue = NULL;
static volatile bool g_FwUpdateState = false;

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
#endif

#ifdef CONFIG_TOUCHSCREEN_PROXIMITY_SENSOR
static unsigned char bEnableTpProximity;
static unsigned char bFaceClosingTp;
#endif

#ifdef TP_PRINT
static int tp_print_proc_read(struct msg22xx_ts_data *ts_data);
static void tp_print_create_entry(struct msg22xx_ts_data *ts_data);
#endif

static void msg22xx_reset_hw(struct msg22xx_ts_platform_data *pdata)
{
	gpio_direction_output(pdata->reset_gpio, 1);
	gpio_set_value_cansleep(pdata->reset_gpio, 0);
	/* Note that the RST must be in LOW 10ms at least */
	usleep(pdata->hard_reset_delay_ms * 1000);
	gpio_set_value_cansleep(pdata->reset_gpio, 1);
	/* Enable the interrupt service thread/routine for INT after 50ms */
	usleep(pdata->post_hard_reset_delay_ms * 1000);

}

static int read_i2c_seq(struct msg22xx_ts_data *ts_data, unsigned char addr, unsigned char *buf, unsigned short size)
{
	int rc = 0;
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			.flags = I2C_M_RD, /* read flag */
			.len = size,
			.buf = buf,
		},
	};

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	if (ts_data->client != NULL) {
		rc = i2c_transfer(ts_data->client->adapter, msgs, 1);
		if (rc < 0) {
			dev_err(&ts_data->client->dev, "%s error %d\n", __func__, rc);
		}
	} else {
		dev_err(&ts_data->client->dev, "ts_data->client is NULL\n");
	}

	return rc;
}

static int write_i2c_seq(struct msg22xx_ts_data *ts_data, unsigned char addr, unsigned char *buf, unsigned short size)
{
	int rc = 0;
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			/*
			* if read flag is undefined,
			* then it means write flag.
			*/
			.flags = 0,
			.len = size,
			.buf = buf,
		},
	};

	/*
	* If everything went ok (i.e. 1 msg transmitted), return #bytes
	* transmitted, else error code.
	*/
	if (ts_data->client != NULL) {
		rc = i2c_transfer(ts_data->client->adapter, msgs, 1);
		if (rc < 0) {
			dev_err(&ts_data->client->dev, "%s error %d\n", __func__, rc);
		}
	} else {
		dev_err(&ts_data->client->dev, "ts_data->client is NULL\n");
	}

	return rc;
}

static unsigned short read_reg(struct msg22xx_ts_data *ts_data, unsigned char bank, unsigned char addr)
{
	unsigned char tx_data[3] = {0x10, bank, addr};
	unsigned char rx_data[2] = {0};

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, tx_data, sizeof(tx_data));
	read_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, rx_data, sizeof(rx_data));

	return ((rx_data[1] << 8) | rx_data[0]);
}

static void write_reg(struct msg22xx_ts_data *ts_data, unsigned char bank,
	unsigned char addr, unsigned short data)
{
	unsigned char tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, tx_data, sizeof(tx_data));
}

static void write_reg_8bit(struct msg22xx_ts_data *ts_data, unsigned char bank,
	unsigned char addr, unsigned char data)
{
	unsigned char tx_data[4] = {0x10, bank, addr, data};
	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, tx_data, sizeof(tx_data));
}

static void dbbusDWIICEnterSerialDebugMode(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[5];

	/* Enter the Serial Debug Mode */
	data[0] = 0x53;
	data[1] = 0x45;
	data[2] = 0x52;
	data[3] = 0x44;
	data[4] = 0x42;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static void dbbusExitSerialDebugMode(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[1];

	// Exit the Serial Debug Mode
	data[0] = 0x45;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static void dbbusDWIICStopMCU(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[1];

	/* Stop the MCU */
	data[0] = 0x37;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static void dbbusNotStopMCU(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[1];

	// Not Stop the MCU
	data[0] = 0x36;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static void dbbusDWIICIICUseBus(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[1];

	/* IIC Use Bus */
	data[0] = 0x35;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static void dbbusIICNotUseBus(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[1];

	// IIC Not Use Bus
	data[0] = 0x34;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static void dbbusDWIICIICReshape(struct msg22xx_ts_data *ts_data)
{
	unsigned char data[1];

	/* IIC Re-shape */
	data[0] = 0x71;

	write_i2c_seq(ts_data, SLAVE_I2C_ID_DBBUS, data, sizeof(data));
}

static unsigned int msg22xx_get_ic_type(struct msg22xx_ts_data *ts_data)
{
	unsigned int ic_type = 0;
	unsigned char bank;
	unsigned char addr;

	msg22xx_reset_hw(ts_data->pdata);
	dbbusDWIICEnterSerialDebugMode(ts_data);
	dbbusDWIICStopMCU(ts_data);
	dbbusDWIICIICUseBus(ts_data);
	dbbusDWIICIICReshape(ts_data);
	msleep(300);

	/* stop mcu */
	write_reg_8bit(ts_data, 0x0F, 0xE6, 0x01);
	/* disable watch dog */
	write_reg(ts_data, 0x3C, 0x60, 0xAA55);
	/* get ic type */
	bank = MSTAR_CHIPTOP_REGISTER_BANK;
	addr = MSTAR_CHIPTOP_REGISTER_ICTYPE;
	ic_type = (0xff) & (read_reg(ts_data, bank, addr));

	dprintk("\n %s ic_type is %x, set is %x \n", __func__, ic_type, ts_data->pdata->ic_type);

	if (ic_type != ts_data->pdata->ic_type) {
		ic_type = 0;
	}

	msg22xx_reset_hw(ts_data->pdata);

	return ic_type;
}

static ssize_t msg22xx_gesture_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 1, "%d\n", msg22xx_gesture_mode);
}

static ssize_t msg22xx_gesture_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	int ret;
	unsigned long mode;

	ret = strict_strtoul(buf, 10, &mode);
	if(!ret) {
		if(msg22xx_gesture_mode != (unsigned char)mode) {
			msg22xx_gesture_mode = (unsigned char)mode;
		}
	}

	return ret;
}

static DEVICE_ATTR(gesture, 0666, msg22xx_gesture_show, msg22xx_gesture_store);

static ssize_t tp_print_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);
	tp_print_proc_read(ts_data);

	return snprintf(buf, 3, "%d\n", ts_data->suspended);
}

static ssize_t tp_print_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(tpp, (S_IRUGO | S_IWUSR), tp_print_show, tp_print_store);

#ifdef CONFIG_TOUCHSCREEN_PROXIMITY_SENSOR
static void _msg_enable_proximity(void)
{
	unsigned char tx_data[4] = {0};

	tx_data[0] = 0x52;
	tx_data[1] = 0x00;
	tx_data[2] = 0x47;
	tx_data[3] = 0xa0;
	mutex_lock(&msg22xx_mutex);
	write_i2c_seq(ts_data->client->addr, &tx_data[0], 4);
	mutex_unlock(&msg22xx_mutex);

	bEnableTpProximity = 1;
}

static void _msg_disable_proximity(void)
{
	unsigned char tx_data[4] = {0};

	tx_data[0] = 0x52;
	tx_data[1] = 0x00;
	tx_data[2] = 0x47;
	tx_data[3] = 0xa1;
	mutex_lock(&msg22xx_mutex);
	write_i2c_seq(ts_data->client->addr, &tx_data[0], 4);
	mutex_unlock(&msg22xx_mutex);

	bEnableTpProximity = 0;
	bFaceClosingTp = 0;
}

static void tsps_msg22xx_enable(int en)
{
	if (en) {
		_msg_enable_proximity();
	} else {
		_msg_disable_proximity();
	}
}

static int tsps_msg22xx_data(void)
{
	return bFaceClosingTp;
}
#endif

static int msg22xx_pinctrl_init(struct msg22xx_ts_data *ts_data)
{
	int retval;

	/* Get pinctrl if target uses pinctrl */
	ts_data->ts_pinctrl = devm_pinctrl_get(&(ts_data->client->dev));
	if (IS_ERR_OR_NULL(ts_data->ts_pinctrl)) {
		retval = PTR_ERR(ts_data->ts_pinctrl);
		dev_dbg(&ts_data->client->dev, "Target does not use pinctrl %d\n", retval);
		goto err_pinctrl_get;
	}

	ts_data->pinctrl_state_reset_active = pinctrl_lookup_state(
		ts_data->ts_pinctrl, PINCTRL_STATE_RESET_ACTIVE);
	if (IS_ERR_OR_NULL(ts_data->pinctrl_state_reset_active)) {
		retval = PTR_ERR(ts_data->pinctrl_state_reset_active);
		dev_dbg(&ts_data->client->dev, "Can't lookup %s pinstate %d\n",
			PINCTRL_STATE_RESET_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}

	ts_data->pinctrl_state_int_active= pinctrl_lookup_state(
		ts_data->ts_pinctrl, PINCTRL_STATE_INT_ACTIVE);
	if (IS_ERR_OR_NULL(ts_data->pinctrl_state_int_active)) {
		retval = PTR_ERR(ts_data->pinctrl_state_int_active);
		dev_dbg(&ts_data->client->dev, "Can't lookup %s pinstate %d\n",
			PINCTRL_STATE_INT_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}

	ts_data->pinctrl_state_int_suspend = pinctrl_lookup_state(
		ts_data->ts_pinctrl, PINCTRL_STATE_INT_SUSPEND);
	if (IS_ERR_OR_NULL(ts_data->pinctrl_state_int_suspend)) {
		retval = PTR_ERR(ts_data->pinctrl_state_int_suspend);
		dev_dbg(&ts_data->client->dev, "Can't lookup %s pinstate %d\n",
			PINCTRL_STATE_INT_SUSPEND, retval);
		goto err_pinctrl_lookup;
	}

	ts_data->pinctrl_state_release = pinctrl_lookup_state(
		ts_data->ts_pinctrl, PINCTRL_STATE_RELEASE);
	if (IS_ERR_OR_NULL(ts_data->pinctrl_state_release)) {
		retval = PTR_ERR(ts_data->pinctrl_state_release);
		dev_dbg(&ts_data->client->dev, "Can't lookup %s pinstate %d\n",
			PINCTRL_STATE_RELEASE, retval);
	}

	return 0;

err_pinctrl_lookup:
	devm_pinctrl_put(ts_data->ts_pinctrl);
err_pinctrl_get:
	ts_data->ts_pinctrl = NULL;
	return retval;
}

static unsigned char calculate_checksum(unsigned char *msg, int length)
{
	int checksum = 0, i;

	for (i = 0; i < length; i++) {
		checksum += msg[i];
	}

	return (unsigned char)((-checksum) & 0xFF);
}

static int parse_info(struct msg22xx_ts_data *ts_data)
{
	unsigned char checksum = 0;
	unsigned int x = 0, y = 0;
	unsigned int x2 = 0, y2 = 0;
	unsigned int delta_x = 0, delta_y = 0;
	unsigned int nReportPacketLength = DEMO_MODE_PACKET_LENGTH;
	unsigned char *data = data_normal;

	unsigned char nWakeupMode = 0;
	unsigned char bIsCorrectFormat = 0;
	static unsigned int m_gesture_value[2] = {0};

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    static unsigned char nPrevTouchNum = 0;
    static unsigned short szPrevX[SELF_MAX_TOUCH_NUM] = {0xFFFF, 0xFFFF};
    static unsigned short szPrevY[SELF_MAX_TOUCH_NUM] = {0xFFFF, 0xFFFF};
    static unsigned char szPrevPress[SELF_MAX_TOUCH_NUM] = {0};
    unsigned int i = 0;
    unsigned short szX[SELF_MAX_TOUCH_NUM] = {0};
    unsigned short szY[SELF_MAX_TOUCH_NUM] = {0};
    unsigned short nTemp = 0;
    unsigned char  nChangePoints = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
    _gCurrPress[0] = 0;
    _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
	if(msg22xx_gesture_mode != 0) {
		if(ts_gesture_wakeup_device_enable == true) {
			data = data_gesture;
			nReportPacketLength = GESTURE_WAKEUP_PACKET_LENGTH;
		}
	}
#endif

	mutex_lock(&msg22xx_mutex);
	read_i2c_seq(ts_data, ts_data->client->addr, &data[0], nReportPacketLength);
	mutex_unlock(&msg22xx_mutex);
	checksum = calculate_checksum(&data[0], (nReportPacketLength-1));
	dprintk("check sum: %x == %x?\n", data[nReportPacketLength-1], checksum);

	if (data[nReportPacketLength-1] != checksum) {
		dev_err(&ts_data->client->dev, "WRONG CHECKSUM\n");
		return -EINVAL;
	}

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
	if ((msg22xx_gesture_mode != 0) && (ts_gesture_wakeup_device_enable == true)) {
		dprintk("received raw data from touch panel as following:\n");
		dprintk("pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x pPacket[5]=%x \n", \
			data[0], data[1], data[2], data[3], data[4], data[5]);

		if ((data[0] == 0xA7) && (data[1] == 0x00) && (data[2] == 0x06) && (data[3] == PACKET_TYPE_GESTURE_WAKEUP)) {
			nWakeupMode = data[4];
			bIsCorrectFormat = 1;
		}

		if (bIsCorrectFormat) {
			dprintk("nWakeupMode = 0x%x\n", nWakeupMode);

			switch (nWakeupMode) {
			case 0x58:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG;
				dprintk("Light up screen by DOUBLE_CLICK gesture wakeup.\n");

				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x60:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG;
				dprintk("Light up screen by UP_DIRECT gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_UP, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_UP, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x61:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG;
				dprintk("Light up screen by DOWN_DIRECT gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_DOWN, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_DOWN, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x62:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG;

				dprintk("Light up screen by LEFT_DIRECT gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_LEFT, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_LEFT, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x63:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG;

				dprintk("Light up screen by RIGHT_DIRECT gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_RIGHT, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_RIGHT, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x64:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG;

				dprintk("Light up screen by m_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_M, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_M, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x65:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG;

				dprintk("Light up screen by W_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_W, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_W, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x66:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG;

				dprintk("Light up screen by C_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_C, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_C, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x67:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG;

				dprintk("Light up screen by e_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_E, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_E, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x68:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG;

				dprintk("Light up screen by V_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_V, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_V, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x69:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG;

				dprintk("Light up screen by O_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_O, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_O, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x6A:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG;

				dprintk("Light up screen by S_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_S, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_S, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x6B:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG;

				dprintk("Light up screen by Z_CHARACTER gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, KEY_Z, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, KEY_Z, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x6C:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE1_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE1_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER1, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER1, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x6D:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE2_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE2_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER2, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER2, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x6E:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE3_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE3_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER3, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER3, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
			case 0x6F:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE4_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE4_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER4, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER4, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x70:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE5_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE5_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER5, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER5, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x71:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE6_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE6_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER6, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER6, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x72:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE7_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE7_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER7, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER7, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x73:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE8_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE8_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER8, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER8, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x74:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE9_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE9_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER9, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER9, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x75:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE10_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE10_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER10, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER10, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x76:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE11_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE11_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER11, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER11, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x77:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE12_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE12_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER12, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER12, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x78:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE13_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE13_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER13, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER13, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x79:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE14_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE14_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER14, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER14, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x7A:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE15_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE15_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER15, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER15, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x7B:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE16_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE16_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER16, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER16, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x7C:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE17_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE17_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER17, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER17, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x7D:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE18_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE18_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER18, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER18, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x7E:
				m_gesture_value[0] = GESTURE_WAKEUP_MODE_RESERVE19_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE19_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER19, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER19, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x7F:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE20_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE20_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER20, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER20, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x80:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE21_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE21_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER21, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER21, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x81:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE22_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE22_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER22, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER22, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x82:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE23_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE23_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER23, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER23, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x83:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE24_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE24_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER24, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER24, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x84:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE25_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE25_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER25, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER25, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x85:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE26_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE26_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER26, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER26, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x86:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE27_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE27_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER27, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER27, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x87:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE28_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE28_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER28, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER28, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x88:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE29_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE29_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER29, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER29, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x89:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE30_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE30_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER30, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER30, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x8A:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE31_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE31_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER31, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER31, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x8B:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE32_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE32_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER32, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER32, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x8C:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE33_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE33_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER33, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER33, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x8D:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE34_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE34_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER34, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER34, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x8E:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE35_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE35_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER35, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER35, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x8F:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE36_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE36_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER36, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER36, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x90:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE37_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE37_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER37, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER37, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x91:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE38_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE38_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER38, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER38, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x92:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE39_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE39_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER39, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER39, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x93:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE40_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE40_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER40, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER40, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x94:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE41_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE41_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER41, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER41, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x95:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE42_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE42_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER42, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER42, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x96:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE43_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE43_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER43, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER43, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x97:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE44_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE44_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER44, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER44, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x98:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE45_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE45_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER45, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER45, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x99:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE46_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE46_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER46, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER46, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x9A:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE47_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE47_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER47, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER47, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x9B:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE48_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE48_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER48, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER48, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x9C:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE49_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE49_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER49, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER49, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x9D:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE50_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE50_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER50, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER50, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
			case 0x9E:
				m_gesture_value[1] = GESTURE_WAKEUP_MODE_RESERVE51_FLAG;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_RESERVE51_FLAG gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER51, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER51, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
			case 0xFF://Gesture Fail
				m_gesture_value[1] = 0xFF;

				dprintk("Light up screen by GESTURE_WAKEUP_MODE_FAIL gesture wakeup.\n");

				//input_report_key(ts_data->input_dev, RESERVER51, 1);
				input_report_key(ts_data->input_dev, KEY_POWER, 1);
				input_sync(ts_data->input_dev);
				//input_report_key(ts_data->input_dev, RESERVER51, 0);
				input_report_key(ts_data->input_dev, KEY_POWER, 0);
				input_sync(ts_data->input_dev);
				break;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
			default:
				m_gesture_value[0] = 0;
				m_gesture_value[1] = 0;
				dprintk("Un-supported gesture wakeup mode. Please check your device driver code.\n");
				break;
			}
		} else {
			dprintk("gesture wakeup packet format is incorrect.\n");
		}

		return -1;
	}

#endif
	dprintk("NORMAL ::::::received raw data from touch panel as following:\n");
	dprintk("pPacket[0]=%x \n pPacket[1]=%x pPacket[2]=%x pPacket[3]=%x pPacket[4]=%x \n pPacket[5]=%x pPacket[6]=%x pPacket[7]=%x \n", \
		data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	if (data[0] != 0x52) {
		dev_err(&ts_data->client->dev, "WRONG HEADER\n");
		return -EINVAL;
	}

	ts_data->info.keycode = 0xFF;
	if ((data[1] == 0xFF) && (data[2] == 0xFF) && (data[3] == 0xFF) && (data[4] == 0xFF) &&
		(data[6] == 0xFF)) {
			if ((data[5] == 0xFF) || (data[5] == 0)) {
				ts_data->info.keycode = 0xFF;
			} else if ((data[5] == 1) || (data[5] == 2) || (data[5] == 4) || (data[5] == 8)) {
					ts_data->info.keycode = data[5] >> 1;

					dev_dbg(&ts_data->client->dev,
						"ts_data->info.keycode index %d\n",
						ts_data->info.keycode);
			} else {
				dev_err(&ts_data->client->dev, "WRONG KEY\n");
				return -EINVAL;
			}
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
		_gPrevTouchStatus = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

	} else {// one  point

		x = (((data[1] & 0xF0) << 4) | data[2]);
		y = (((data[1] & 0x0F) << 8) | data[3]);
		delta_x = (((data[4] & 0xF0) << 4) | data[5]);
		delta_y = (((data[4] & 0x0F) << 8) | data[6]);

		if ((delta_x == 0) && (delta_y == 0)) {
			ts_data->info.point[0].x = x * ts_data->pdata->x_max / TPD_WIDTH;
			ts_data->info.point[0].y = y * ts_data->pdata->y_max / TPD_HEIGHT;
			ts_data->info.count = 1;
#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
            _gCurrPress[0] = 1;
            _gCurrPress[1] = 0;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
		} else {  // two point
			if (delta_x > 2048) {
				delta_x -= 4096;
			}
			if (delta_y > 2048) {
				delta_y -= 4096;
            }
			x2 = (unsigned int)((signed short)x + (signed short)delta_x);
			y2 = (unsigned int)((signed short)y + (signed short)delta_y);
			ts_data->info.point[0].x = x * ts_data->pdata->x_max / TPD_WIDTH;
			ts_data->info.point[0].y = y * ts_data->pdata->y_max / TPD_HEIGHT;
			ts_data->info.point[1].x = x2 * ts_data->pdata->x_max / TPD_WIDTH;
			ts_data->info.point[1].y = y2 * ts_data->pdata->y_max / TPD_HEIGHT;
			ts_data->info.count = ts_data->pdata->num_max_touches;

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
                _gCurrPress[0] = 1;
                _gCurrPress[1] = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
            }

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
        if (_gPrevTouchStatus == 1)
        {
            for (i = 0; i < MAX_TOUCH_NUM; i ++)
            {
                szX[i] = ts_data->info.point[i].x;
                szY[i] = ts_data->info.point[i].y;
            }
            if (/*(pInfo->nFingerNum == 1)&&*/(nPrevTouchNum == 2))
            {
                if (_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[0], szPrevY[0]) > _DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]))
                {
                    nChangePoints = 1;
                }
            }
            else if ((ts_data->info.count == 2) && (nPrevTouchNum == 1))
            {
                if (szPrevPress[0] == 1)
                {
                    if(_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[0] ,szPrevY[0]) > _DrvFwCtrlPointDistance(szX[1], szY[1], szPrevX[0], szPrevY[0]))
                    {
                        nChangePoints = 1;
                    }
                }
                else
                {
                    if (_DrvFwCtrlPointDistance(szX[0], szY[0], szPrevX[1], szPrevY[1]) < _DrvFwCtrlPointDistance(szX[1], szY[1], szPrevX[1], szPrevY[1]))
                    {
                        nChangePoints = 1;
                    }
                }
            }
            else if ((ts_data->info.count == 1) && (nPrevTouchNum == 1))
            {
                if (_gCurrPress[0] != szPrevPress[0])
                {
                    nChangePoints = 1;
                }
            }

            if (nChangePoints == 1)
            {
                nTemp = _gCurrPress[0];
                _gCurrPress[0] = _gCurrPress[1];
                _gCurrPress[1] = nTemp;

                nTemp = ts_data->info.point[0].x;
                ts_data->info.point[0].x = ts_data->info.point[1].x;
                ts_data->info.point[1].x = nTemp;

                nTemp = ts_data->info.point[0].y;
                ts_data->info.point[0].y = ts_data->info.point[1].y;
                ts_data->info.point[1].y = nTemp;

            }
        }

        // Save current status
        for (i = 0; i < MAX_TOUCH_NUM; i ++)
        {
            szPrevPress[i] = _gCurrPress[i];
            szPrevX[i] = ts_data->info.point[i].x;
            szPrevY[i] = ts_data->info.point[i].y;
        }
        nPrevTouchNum = ts_data->info.count;

        _gPrevTouchStatus = 1;
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL
	}

	return 0;
}

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
static unsigned int _DrvFwCtrlPointDistance(unsigned short nX, unsigned short nY, unsigned short nPrevX, unsigned short nPrevY)
{
    unsigned int nRetVal = 0;

    nRetVal = (((nX-nPrevX)*(nX-nPrevX))+((nY-nPrevY)*(nY-nPrevY)));

    return nRetVal;
}
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

static void touch_driver_touch_released(struct msg22xx_ts_data *ts_data)
{
	int i;

	for (i = 0; i < ts_data->pdata->num_max_touches; i++) {
		input_mt_slot(ts_data->input_dev, i);
		input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, 0);
	}

	input_report_key(ts_data->input_dev, BTN_TOUCH, 0);
	input_report_key(ts_data->input_dev, BTN_TOOL_FINGER, 0);
	input_sync(ts_data->input_dev);
}

/* read data through I2C then report data to input
sub-system when interrupt occurred  */
static irqreturn_t msg22xx_ts_interrupt(int irq, void *dev_id)
{
	int i = 0;
	static int last_keycode = 0xFF;
	static int last_count;
	struct msg22xx_ts_data *ts_data = dev_id;

	dprintk("\n interrupt!\n");

	ts_data->info.count = 0;
	if (0 == parse_info(ts_data)) {
		if (ts_data->info.keycode != 0xFF) {   /* key touch pressed */
			if (ts_data->info.keycode < ts_data->pdata->num_buttons) {
					if (ts_data->info.keycode != last_keycode) {
						dev_dbg(&ts_data->client->dev, "key touch pressed");

						input_report_key(ts_data->input_dev, BTN_TOUCH, 1);
						input_report_key(ts_data->input_dev,
							ts_data->pdata->button_map[ts_data->info.keycode], 1);

						last_keycode = ts_data->info.keycode;
					} else {
						/* pass duplicate key-pressing */
						dev_dbg(&ts_data->client->dev, "REPEATED KEY\n");
					}
			} else {
				dev_dbg(&ts_data->client->dev, "WRONG KEY\n");
			}
		} else {  /* key touch released */
			if (last_keycode != 0xFF) {
				dev_dbg(&ts_data->client->dev, "key touch released");

				input_report_key(ts_data->input_dev, BTN_TOUCH, 0);
				input_report_key(ts_data->input_dev,
					ts_data->pdata->button_map[last_keycode], 0);

				last_keycode = 0xFF;
			}
		}

		if (ts_data->info.count > 0)	{ /* point touch pressed */
			for (i = 0; i < ts_data->pdata->num_max_touches; i++) {
                if(_gCurrPress[i] == 0)
                    continue;
				input_mt_slot(ts_data->input_dev, i);
				input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, 1);
				input_report_abs(ts_data->input_dev, ABS_MT_TOUCH_MAJOR, 1);
				input_report_abs(ts_data->input_dev, ABS_MT_POSITION_X,
					ts_data->info.point[i].x);
				input_report_abs(ts_data->input_dev, ABS_MT_POSITION_Y,
					ts_data->info.point[i].y);
			}
		}

		if (last_count > ts_data->info.count) {
			for (i = 0; i < ts_data->pdata->num_max_touches; i++) {
                if(_gCurrPress[i] == 1)
                    continue;
				input_mt_slot(ts_data->input_dev, i);
				input_mt_report_slot_state(ts_data->input_dev, MT_TOOL_FINGER, 0);
			}
		}
		last_count = ts_data->info.count;

		input_report_key(ts_data->input_dev, BTN_TOUCH, ts_data->info.count > 0);
		input_report_key(ts_data->input_dev, BTN_TOOL_FINGER, ts_data->info.count > 0);

		input_sync(ts_data->input_dev);
	}

	return IRQ_HANDLED;
}

static int msg22xx_ts_power_init(struct msg22xx_ts_data *ts_data, bool init)
{
	int rc;

	if (init) {
		ts_data->vdd = regulator_get(&ts_data->client->dev, "vdd");
		if (IS_ERR(ts_data->vdd)) {
			rc = PTR_ERR(ts_data->vdd);
			dev_err(&ts_data->client->dev,
				"Regulator get failed vdd rc=%d\n", rc);
			return rc;
		}

		if (regulator_count_voltages(ts_data->vdd) > 0) {
			rc = regulator_set_voltage(ts_data->vdd,
				MSTAR_VTG_MIN_UV,
				MSTAR_VTG_MAX_UV);
			if (rc) {
				dev_err(&ts_data->client->dev,
					"Regulator set_vtg failed vdd rc=%d\n",
					rc);
				goto reg_vdd_put;
			}
		}

		ts_data->vcc_i2c = regulator_get(&ts_data->client->dev,
			"vcc_i2c");
		if (IS_ERR(ts_data->vcc_i2c)) {
			rc = PTR_ERR(ts_data->vcc_i2c);
			dev_err(&ts_data->client->dev,
				"Regulator get failed vcc_i2c rc=%d\n", rc);
			goto reg_vdd_set_vtg;
		}

		if (regulator_count_voltages(ts_data->vcc_i2c) > 0) {
			rc = regulator_set_voltage(ts_data->vcc_i2c,
				MSTAR_I2C_VTG_MIN_UV,
				MSTAR_I2C_VTG_MAX_UV);
			if (rc) {
				dev_err(&ts_data->client->dev,
					"Regulator set_vtg failed vcc_i2c rc=%d\n", rc);
				goto reg_vcc_i2c_put;
			}
		}
	} else {
		if (regulator_count_voltages(ts_data->vdd) > 0)
			regulator_set_voltage(ts_data->vdd, 0,
			MSTAR_VTG_MAX_UV);

		regulator_put(ts_data->vdd);

		if (regulator_count_voltages(ts_data->vcc_i2c) > 0)
			regulator_set_voltage(ts_data->vcc_i2c, 0,
			MSTAR_I2C_VTG_MAX_UV);

		regulator_put(ts_data->vcc_i2c);
	}

	return 0;

reg_vcc_i2c_put:
	regulator_put(ts_data->vcc_i2c);
reg_vdd_set_vtg:
	if (regulator_count_voltages(ts_data->vdd) > 0)
		regulator_set_voltage(ts_data->vdd, 0, MSTAR_VTG_MAX_UV);
reg_vdd_put:
	regulator_put(ts_data->vdd);
	return rc;
}

static int msg22xx_ts_power_on(struct msg22xx_ts_data *ts_data, bool on)
{
	int rc;

	if (!on)
		goto power_off;

	rc = regulator_enable(ts_data->vdd);
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vdd enable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_enable(ts_data->vcc_i2c);
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vcc_i2c enable failed rc=%d\n", rc);
		regulator_disable(ts_data->vdd);
	}

	if (gpio_is_valid(ts_data->pdata->cam_avdd_gpio)) {
		msleep(10);
		gpio_set_value_cansleep(ts_data->pdata->cam_avdd_gpio, 1);
	}

	return rc;

power_off:

	if (gpio_is_valid(ts_data->pdata->cam_avdd_gpio)) {
		gpio_set_value_cansleep(ts_data->pdata->cam_avdd_gpio, 0);
		msleep(5);
	}

	rc = regulator_disable(ts_data->vdd);
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vdd disable failed rc=%d\n", rc);
		return rc;
	}

	rc = regulator_disable(ts_data->vcc_i2c);
	if (rc) {
		dev_err(&ts_data->client->dev,
			"Regulator vcc_i2c disable failed rc=%d\n", rc);
		rc = regulator_enable(ts_data->vdd);
	}

	return rc;
}

static int msg22xx_ts_gpio_configure(struct msg22xx_ts_data *ts_data, bool on)
{
	int ret = 0;

	if (!on)
		goto pwr_deinit;

	if (gpio_is_valid(ts_data->pdata->irq_gpio)) {
		ret = gpio_request(ts_data->pdata->irq_gpio,
			"msg22xx_irq_gpio");
		if (ret) {
			dev_err(&ts_data->client->dev,
				"Failed to request GPIO[%d], %d\n",
				ts_data->pdata->irq_gpio, ret);
			goto err_irq_gpio_req;
		}
		ret = gpio_direction_input(ts_data->pdata->irq_gpio);
		if (ret) {
			dev_err(&ts_data->client->dev,
				"Failed to set direction for gpio[%d], %d\n",
				ts_data->pdata->irq_gpio, ret);
			goto err_irq_gpio_dir;
		}
		gpio_set_value_cansleep(ts_data->pdata->irq_gpio, 0);
	} else {
		dev_err(&ts_data->client->dev, "irq gpio not provided\n");
		goto err_irq_gpio_req;
	}

	if (gpio_is_valid(ts_data->pdata->reset_gpio)) {
		ret = gpio_request(ts_data->pdata->reset_gpio,
			"msg22xx_reset_gpio");
		if (ret) {
			dev_err(&ts_data->client->dev,
				"Failed to request GPIO[%d], %d\n",
				ts_data->pdata->reset_gpio, ret);
			goto err_reset_gpio_req;
		}

		/* power on TP */
		ret = gpio_direction_output(
			ts_data->pdata->reset_gpio, 1);
		if (ret) {
			dev_err(&ts_data->client->dev,
				"Failed to set direction for GPIO[%d], %d\n",
				ts_data->pdata->reset_gpio, ret);
			goto err_reset_gpio_dir;
		}
		msleep(100);
		gpio_set_value_cansleep(ts_data->pdata->reset_gpio, 0);
		msleep(20);
		gpio_set_value_cansleep(ts_data->pdata->reset_gpio, 1);
		msleep(200);
	} else {
		dev_err(&ts_data->client->dev, "reset gpio not provided\n");
		goto err_reset_gpio_req;
	}

	return 0;

err_reset_gpio_dir:
	if (gpio_is_valid(ts_data->pdata->reset_gpio))
		gpio_free(ts_data->pdata->irq_gpio);
err_reset_gpio_req:
err_irq_gpio_dir:
	if (gpio_is_valid(ts_data->pdata->irq_gpio))
		gpio_free(ts_data->pdata->irq_gpio);
err_irq_gpio_req:
	return ret;

pwr_deinit:
	if (gpio_is_valid(ts_data->pdata->irq_gpio))
		gpio_free(ts_data->pdata->irq_gpio);
	if (gpio_is_valid(ts_data->pdata->reset_gpio)) {
		gpio_set_value_cansleep(ts_data->pdata->reset_gpio, 0);
		ret = gpio_direction_input(ts_data->pdata->reset_gpio);
		if (ret)
			dev_err(&ts_data->client->dev,
			"Unable to set direction for gpio [%d]\n",
			ts_data->pdata->reset_gpio);
		gpio_free(ts_data->pdata->reset_gpio);
	}
	return 0;
}

static void msg22xx_disable_irq(struct msg22xx_ts_data *ts_data) {
	unsigned long irq_flag;

	spin_lock_irqsave(&msg22xx_irqlock, irq_flag);

	if (0 == ts_irq_flag) {
		disable_irq(ts_data->client->irq);
		ts_irq_flag = 1;
	}

	spin_unlock_irqrestore(&msg22xx_irqlock, irq_flag);
}

static void msg22xx_enable_irq(struct msg22xx_ts_data *ts_data) {
	unsigned long irq_flag;

	spin_lock_irqsave(&msg22xx_irqlock, irq_flag);

	if (1 == ts_irq_flag) {
		enable_irq(ts_data->client->irq);
		ts_irq_flag = 0;
	}

	spin_unlock_irqrestore(&msg22xx_irqlock, irq_flag);
}

#ifdef CONFIG_PM
static int msg22xx_ts_resume(struct device *dev)
{
	int retval = 0;
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);

	if (1 == ts_resume_suspend_flag) {
		mutex_lock(&ts_data->ts_mutex);

		retval = msg22xx_ts_power_on(ts_data, true);
		if (retval) {
			dev_err(dev, "msg22xx_ts power on failed");
			mutex_unlock(&ts_data->ts_mutex);
			return retval;
		}
        /*
        * after power on, pull up int
        */
        usleep(1500);   /*sleep at least 1ms before pull up*/
        if (ts_data->ts_pinctrl) {
            retval = pinctrl_select_state(ts_data->ts_pinctrl, ts_data->pinctrl_state_int_active);
            if (retval < 0){
                dev_err(&ts_data->client->dev,
                "Failed to select %s pinatate %d\n", PINCTRL_STATE_INT_ACTIVE, retval);
            }
        }

		msg22xx_enable_irq(ts_data);

		mutex_unlock(&ts_data->ts_mutex);
		ts_resume_suspend_flag = 0;
	}

	return 0;
}

static int msg22xx_ts_suspend(struct device *dev)
{
	int retval = 0;
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);

	if (0 == ts_resume_suspend_flag) {
		mutex_lock(&ts_data->ts_mutex);

		msg22xx_disable_irq(ts_data);

		touch_driver_touch_released(ts_data);

		retval = msg22xx_ts_power_on(ts_data, false);
		if (retval) {
			dev_err(dev, "msg22xx_ts power off failed");
			mutex_unlock(&ts_data->ts_mutex);
			return retval;
		}
        /*
        * after power off, pull down int
        */
        if (ts_data->ts_pinctrl) {
            retval = pinctrl_select_state(ts_data->ts_pinctrl, ts_data->pinctrl_state_int_suspend);
            if (retval < 0){
                dev_err(&ts_data->client->dev,
                "Failed to select %s pinatate %d\n", PINCTRL_STATE_INT_SUSPEND, retval);
            }
        }

		mutex_unlock(&ts_data->ts_mutex);
		ts_resume_suspend_flag = 1;
	}

	return 0;
}
#else
static int msg22xx_ts_resume(struct device *dev)
{
	return 0;
}
static int msg22xx_ts_suspend(struct device *dev)
{
	return 0;
}
#endif

static int msg22xx_debug_suspend_set(void *_data, u64 val)
{
	struct msg22xx_ts_data *data = _data;

	mutex_lock(&data->input_dev->mutex);

	if (val)
		msg22xx_ts_suspend(&data->client->dev);
	else
		msg22xx_ts_resume(&data->client->dev);

	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

static int msg22xx_debug_suspend_get(void *_data, u64 *val)
{
	struct msg22xx_ts_data *data = _data;

	mutex_lock(&data->input_dev->mutex);
	*val = data->suspended;
	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_suspend_fops, msg22xx_debug_suspend_get,
	msg22xx_debug_suspend_set, "%lld\n");

/* gesture */
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
static void msg22xx_open_gesture_wakeup(struct msg22xx_ts_data *ts_data, unsigned int *pMode)
{
	unsigned char szDbBusTxData[4] = {0};
	unsigned int i = 0;
	int rc = 0;

	dprintk("*** %s() ***\n", __func__);

	dprintk("wakeup mode 0 = 0x%x\n", pMode[0]);
	dprintk("wakeup mode 1 = 0x%x\n", pMode[1]);

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
	szDbBusTxData[0] = 0x59;
	szDbBusTxData[1] = 0x00;
	szDbBusTxData[2] = ((pMode[1] & 0xFF000000) >> 24);
	szDbBusTxData[3] = ((pMode[1] & 0x00FF0000) >> 16);

	while (i < 5) {
		rc = write_i2c_seq(ts_data, ts_data->client->addr, &szDbBusTxData[0], 4);
		udelay(1000); // delay 1ms

		if (rc > 0) {
			dprintk("Enable gesture wakeup index 0 success\n");
			break;
		}

		mdelay(10);
		i++;
	}
	if (i == 5) {
		dprintk("Enable gesture wakeup index 0 failed\n");
	}

	szDbBusTxData[0] = 0x59;
	szDbBusTxData[1] = 0x01;
	szDbBusTxData[2] = ((pMode[1] & 0x0000FF00) >> 8);
	szDbBusTxData[3] = ((pMode[1] & 0x000000FF) >> 0);

	while (i < 5) {
		rc = write_i2c_seq(ts_data, ts_data->client->addr, &szDbBusTxData[0], 4);
		udelay(1000); // delay 1ms

		if (rc > 0) {
			dprintk("Enable gesture wakeup index 1 success\n");
			break;
		}

		mdelay(10);
		i++;
	}
	if (i == 5) {
		dprintk("Enable gesture wakeup index 1 failed\n");
	}

	szDbBusTxData[0] = 0x59;
	szDbBusTxData[1] = 0x02;
	szDbBusTxData[2] = ((pMode[0] & 0xFF000000) >> 24);
	szDbBusTxData[3] = ((pMode[0] & 0x00FF0000) >> 16);

	while (i < 5) {
		rc = write_i2c_seq(ts_data, ts_data->client->addr, &szDbBusTxData[0], 4);
		udelay(1000); // delay 1ms

		if (rc > 0) {
			dprintk("Enable gesture wakeup index 2 success\n");
			break;
		}

		mdelay(10);
		i++;
	}
	if (i == 5) {
		dprintk("Enable gesture wakeup index 2 failed\n");
	}

	szDbBusTxData[0] = 0x59;
	szDbBusTxData[1] = 0x03;
	szDbBusTxData[2] = ((pMode[0] & 0x0000FF00) >> 8);
	szDbBusTxData[3] = ((pMode[0] & 0x000000FF) >> 0);

	while (i < 5) {
		rc = write_i2c_seq(ts_data, ts_data->client->addr, &szDbBusTxData[0], 4);
		udelay(1000); // delay 1ms

		if(rc > 0) {
			dprintk("Enable gesture wakeup index 3 success\n");
			break;
		}

		mdelay(10);
		i++;
	}
	if (i == 5) {
		dprintk("Enable gesture wakeup index 3 failed\n");
	}
	ts_gesture_wakeup_device_enable = true; // gesture wakeup is enabled
#else
	szDbBusTxData[0] = 0x58;
	szDbBusTxData[1] = ((pMode[0] & 0x0000FF00) >> 8);
	szDbBusTxData[2] = ((pMode[0] & 0x000000FF) >> 0);

	while(i < 5) {
		mdelay(20);
		rc = write_i2c_seq(ts_data, ts_data->client->addr, &szDbBusTxData[0], 3);

		if(rc > 0) {
			dprintk("Enable gesture wakeup success\n");
			break;
		}

		mdelay(10);
		i++;
	}
	if (i == 5) {
		dprintk("Enable gesture wakeup failed\n");
	}

	ts_gesture_wakeup_device_enable = true; // gesture wakeup is enabled
	dprintk("\n %s: ts_gesture_wakeup_device_enable is true\n",__func__);

#endif
}
#endif

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct msg22xx_ts_data *ts_data =
		container_of(self, struct msg22xx_ts_data, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK) {
			if(g_FwUpdateState == true) {
				dprintk("%s while update firmware, don't power on/off  touch screen\n", __func__);
				return 0;
			}
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
			if(msg22xx_gesture_mode != 0) {
				if(ts_gesture_wakeup_device_enable == true) {
					ts_gesture_wakeup_device_enable = false;
					if(device_may_wakeup(&ts_data->client->dev)) {
						disable_irq_wake(ts_data->client->irq);
						dprintk("\n %s disable a wakeup irq\n", __func__);
					}
					msg22xx_reset_hw(ts_data->pdata);
				}

			} else {
				msg22xx_ts_resume(&ts_data->client->dev);
			}
#else
			msg22xx_ts_resume(&ts_data->client->dev);
#endif
		} else if (*blank == FB_BLANK_POWERDOWN) {
			if(g_FwUpdateState == true) {
				dprintk("%s while update firmware, don't power on/off  touch screen\n", __func__);
				return 0;
			}
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
			if(msg22xx_gesture_mode != 0) {
				if(device_may_wakeup(&ts_data->client->dev)) {
					enable_irq_wake(ts_data->client->irq);
					dprintk("\n %s be a wakeup irq\n", __func__);
				}

				if (ts_gesture_wakeup_mode[0] != 0x00000000 || ts_gesture_wakeup_mode[1] != 0x00000000) {
					msg22xx_open_gesture_wakeup(ts_data, &ts_gesture_wakeup_mode[0]);
					return 0;
				}
			} else {
				msg22xx_ts_suspend(&ts_data->client->dev);
			}
#else
			msg22xx_ts_suspend(&ts_data->client->dev);
#endif
		}
	}
	return 0;
}
#endif

static int msg22xx_get_dt_coords(struct device *dev, char *name, struct msg22xx_ts_platform_data *pdata)
{
	unsigned int coords[FT_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(unsigned int);
	if (coords_size != FT_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (!strcmp(name, "mstar,panel-coords")) {
		pdata->panel_minx = coords[0];
		pdata->panel_miny = coords[1];
		pdata->panel_maxx = coords[2];
		pdata->panel_maxy = coords[3];
	} else if (!strcmp(name, "mstar,display-coords")) {
		pdata->x_min = coords[0];
		pdata->y_min = coords[1];
		pdata->x_max = coords[2];
		pdata->y_max = coords[3];
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}

static int msg22xx_parse_dt(struct device *dev, struct msg22xx_ts_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	struct property *prop;
	unsigned int temp_val;

	rc = msg22xx_get_dt_coords(dev, "mstar,panel-coords", pdata);
	if (rc && (rc != -EINVAL)) {
		return rc;
	}
	rc = msg22xx_get_dt_coords(dev, "mstar,display-coords", pdata);
	if (rc) {
		return rc;
	}
	rc = of_property_read_u32(np, "mstar,hard-reset-delay-ms", &temp_val);
	if (!rc) {
		pdata->hard_reset_delay_ms = temp_val;
	} else {
		return rc;
	}
	rc = of_property_read_u32(np, "mstar,post-hard-reset-delay-ms", &temp_val);
	if (!rc) {
		pdata->post_hard_reset_delay_ms = temp_val;
	} else {
		return rc;
	}
	/* reset, irq gpio info */
	pdata->reset_gpio = of_get_named_gpio_flags(np, "mstar,reset-gpio", 0, &pdata->reset_gpio_flags);
	if (pdata->reset_gpio < 0) {
		return pdata->reset_gpio;
	}
	pdata->irq_gpio = of_get_named_gpio_flags(np, "mstar,irq-gpio", 0, &pdata->irq_gpio_flags);
	if (pdata->irq_gpio < 0) {
		return pdata->irq_gpio;
	}
	rc = of_property_read_u32(np, "mstar,ic-type", &temp_val);
	if (rc && (rc != -EINVAL)) {
		return rc;
        }
	pdata->ic_type = temp_val;

	rc = of_property_read_u32(np, "mstar,num-max-touches", &temp_val);
	if (!rc) {
		pdata->num_max_touches = temp_val;
	} else {
		return rc;
	}
	prop = of_find_property(np, "mstar,button-map", NULL);
	if (prop) {
		pdata->num_buttons = prop->length / sizeof(temp_val);
		if (pdata->num_buttons > MAX_BUTTONS)
			return -EINVAL;

		rc = of_property_read_u32_array(np,
			"mstar,button-map", pdata->button_map,
			pdata->num_buttons);
		if (rc) {
			dev_err(dev, "Unable to read key codes\n");
			return rc;
		}
	}

	pdata->cam_avdd_gpio = of_get_named_gpio(np, "mstar,cam-avdd-gpio", 0);
	if (pdata->cam_avdd_gpio < 0) {
		dev_err(dev, "Failed to parse cam_avdd_gpio\n");
		return pdata->cam_avdd_gpio;
	} else {
		rc = gpio_request(pdata->cam_avdd_gpio, "msg22xx_cam_avdd_gpio");
		if (rc) {
			dev_err(dev,"Failed to request cam_avdd_gpio GPIO[%d], %d\n", pdata->cam_avdd_gpio, rc);
			return rc;
		}

		rc = gpio_direction_output(pdata->cam_avdd_gpio, 0);
		if (rc) {
			dev_err(dev,"Failed to set direction for cam_avdd_gpio GPIO[%d], %d\n", pdata->cam_avdd_gpio, rc);
		}
	}

	return 0;
}

static unsigned int msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EmemType_e eEmemType)
{
	unsigned short nCrcDown = 0;
	unsigned int nRetVal = 0;
	unsigned int retryCount = 0;

	dprintk("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

	dbbusDWIICEnterSerialDebugMode(msg22xx_data);
	dbbusDWIICStopMCU(msg22xx_data);
	dbbusDWIICIICUseBus(msg22xx_data);
	dbbusDWIICIICReshape(msg22xx_data);
	mdelay(300);

	// RIU password
	write_reg(msg22xx_data, 0x16, 0x1a, 0xABBA);

	// Set PCE high
	write_reg_8bit(msg22xx_data, 0x16, 0x18, 0x40);

	if (eEmemType == EMEM_MAIN) {
		// Set start address and end address for main block
		write_reg(msg22xx_data, 0x16, 0x00, 0x0000);
		write_reg(msg22xx_data, 0x16, 0x40, 0xBFF8);
	} else if (eEmemType == EMEM_INFO) {
		// Set start address and end address for info block
		write_reg(msg22xx_data, 0x16, 0x00, 0xC000);
		write_reg(msg22xx_data, 0x16, 0x40, 0xC1F8);
	}

	// CRC reset
	write_reg(msg22xx_data, 0x16, 0x4E, 0x0001);
	write_reg(msg22xx_data, 0x16, 0x4E, 0x0000);

	// Trigger CRC check
	write_reg_8bit(msg22xx_data, 0x16, 0x0E, 0x20);
	mdelay(10);

	nCrcDown = read_reg(msg22xx_data,0x16, 0x4E);

	while ((nCrcDown != 2) && (retryCount < 3)) {
		dprintk("Wait CRC down\n");
		mdelay(10);
		nCrcDown = read_reg(msg22xx_data,0x16, 0x4E);
		retryCount++;
	}

	nRetVal = read_reg(msg22xx_data,0x16, 0x52);

	nRetVal = (nRetVal << 16) | (read_reg(msg22xx_data,0x16, 0x50));

	dprintk("Hardware CRC = 0x%x\n", nRetVal);

	dbbusIICNotUseBus(msg22xx_data);
	dbbusNotStopMCU(msg22xx_data);
	dbbusExitSerialDebugMode(msg22xx_data);

	return nRetVal;
}

static unsigned short msg22xx_DrvFwCtrlMsg22xxGetSwId(EmemType_e eEmemType)
{
	unsigned short nRetVal = 0;
	unsigned short nRegData1 = 0;

	dprintk("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

	dbbusDWIICEnterSerialDebugMode(msg22xx_data);
	dbbusDWIICStopMCU(msg22xx_data);
	dbbusDWIICIICUseBus(msg22xx_data);
	dbbusDWIICIICReshape(msg22xx_data);
	mdelay(100);

	// Stop MCU
	write_reg_8bit(msg22xx_data ,0x0F ,0xE6, 0x01);

	// Stop watchdog
	write_reg(msg22xx_data, 0x3C, 0x60, 0xAA55);

	// RIU password
	write_reg(msg22xx_data, 0x16, 0x1A, 0xABBA);

	if (eEmemType == EMEM_MAIN) { // Read SW ID from main block
		write_reg(msg22xx_data, 0x16, 0x00, 0xBFF4);// Set start address for main block SW ID
	} else if (eEmemType == EMEM_INFO) { // Read SW ID from info block
		write_reg(msg22xx_data, 0x16, 0x00, 0xC1EC);// Set start address for info block SW ID
	}

    /*
      Ex. SW ID in Main Block :
          Major low byte at address 0xBFF4
          Major high byte at address 0xBFF5
          SW ID in Info Block :
          Major low byte at address 0xC1EC
          Major high byte at address 0xC1ED
    */

	write_reg_8bit(msg22xx_data, 0x16, 0x0E, 0x01);

	nRegData1 = read_reg(msg22xx_data, 0x16, 0x04);

	nRetVal = ((nRegData1 >> 8) & 0xFF) << 8;
	nRetVal |= (nRegData1 & 0xFF);
	write_reg(msg22xx_data, 0x16, 0x00, 0x0000);

	// Clear RIU password
	write_reg(msg22xx_data, 0x16, 0x1A, 0x0000);

	dprintk("SW ID = 0x%x\n", nRetVal);

	dbbusIICNotUseBus(msg22xx_data);
	dbbusNotStopMCU(msg22xx_data);
	dbbusExitSerialDebugMode(msg22xx_data);

	return nRetVal;
}

static void msg22xx_DrvFwCtrlGetCustomerFirmwareVersion(unsigned short *pMajor, unsigned short *pMinor, unsigned char **ppVersion)
{
	unsigned short nRegData1, nRegData2;

	dprintk("*** %s() ***\n", __func__);

	mutex_lock(&msg22xx_mutex);

	msg22xx_reset_hw(msg22xx_data->pdata);

	dbbusDWIICEnterSerialDebugMode(msg22xx_data);
	dbbusDWIICStopMCU(msg22xx_data);
	dbbusDWIICIICUseBus(msg22xx_data);
	dbbusDWIICIICReshape(msg22xx_data);
	mdelay(100);

	// Stop MCU
	write_reg_8bit(msg22xx_data ,0x0F ,0xE6, 0x01);

	// Stop watchdog
	write_reg(msg22xx_data, 0x3C, 0x60, 0xAA55);

	// RIU password
	write_reg(msg22xx_data, 0x16, 0x1A, 0xABBA);

	write_reg(msg22xx_data, 0x16, 0x00, 0xBFF4); // Set start address for customer firmware version on main block

	write_reg_8bit(msg22xx_data, 0x16, 0x0E, 0x01);

	nRegData1 = read_reg(msg22xx_data, 0x16, 0x04);
	nRegData2 =  read_reg(msg22xx_data, 0x16, 0x06);

	*pMajor = (((nRegData1 >> 8) & 0xFF) << 8) + (nRegData1 & 0xFF);
	*pMinor = (((nRegData2 >> 8) & 0xFF) << 8) + (nRegData2 & 0xFF);

	write_reg(msg22xx_data, 0x16, 0x00, 0x0000);

	// Clear RIU password
	write_reg(msg22xx_data, 0x16, 0x1A, 0x0000);

	dbbusIICNotUseBus(msg22xx_data);
	dbbusNotStopMCU(msg22xx_data);
	dbbusExitSerialDebugMode(msg22xx_data);

	msg22xx_reset_hw(msg22xx_data->pdata);

	mutex_unlock(&msg22xx_mutex);

	dprintk("*** major = %d ***\n", *pMajor);
	dprintk("*** minor = %d ***\n", *pMinor);

	if (*ppVersion == NULL) {
		*ppVersion = kzalloc(sizeof(unsigned char)*6, GFP_KERNEL);
	}

	sprintf(*ppVersion, "%03d%03d", *pMajor, *pMinor);
}

static void msg22xx_convert_data_to_onedimen(unsigned char szTwoDimenFwData[][1024], unsigned char* pOneDimenFwData)
{
	unsigned int i, j;

	dprintk("*** %s() ***\n", __func__);

	for (i = 0; i < (MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE+1); i++) {
		if (i < MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE) {
			for (j = 0; j < 1024; j ++) {
				pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
			}
		} else {
			for (j = 0; j < 512; j ++) {
				pOneDimenFwData[i*1024+j] = szTwoDimenFwData[i][j];
			}
		}
	}
}

static unsigned short msg22xx_DrvFwCtrlMsg22xxGetTrimByte1(void)
{
	unsigned short nRegData = 0;
	unsigned short nTrimByte1 = 0;

	dprintk("*** %s() ***\n", __func__);

	write_reg(msg22xx_data, 0x16, 0x1E, 0xBEAF);
	write_reg(msg22xx_data, 0x16, 0x08, 0x0006);
	write_reg(msg22xx_data, 0x16, 0x0E, 0x0010);
	write_reg(msg22xx_data, 0x16, 0x08, 0x1006);
	write_reg(msg22xx_data, 0x16, 0x00, 0x0001);
	write_reg(msg22xx_data, 0x16, 0x0E, 0x0010);

	mdelay(10);

	write_reg(msg22xx_data, 0x16, 0x08, 0x1846);
	write_reg(msg22xx_data, 0x16, 0x0E, 0x0010);

	mdelay(10);

	nRegData = read_reg(msg22xx_data, 0x16, 0x24);
	nRegData = nRegData & 0xFF;

	write_reg(msg22xx_data, 0x16, 0x1E, 0x0000);

	nTrimByte1 = nRegData;

	dprintk("nTrimByte1 = 0x%X ***\n", nTrimByte1);

	return nTrimByte1;
}

void msg22xx_RegSet16BitValueOn(unsigned char nAddrH, unsigned char nAddrL, unsigned short nData) //Set bit on nData from 0 to 1
{
	unsigned short rData  = read_reg(msg22xx_data, nAddrH, nAddrL);
	rData |= nData;
	write_reg(msg22xx_data, nAddrH, nAddrL, rData);
}

void msg22xx_RegSet16BitValueOff(unsigned char nAddrH, unsigned char nAddrL, unsigned short nData) //Set bit on nData from 1 to 0
{
	unsigned short rData  = read_reg(msg22xx_data, nAddrH, nAddrL);

	rData &= (~nData);
	write_reg(msg22xx_data, nAddrH, nAddrL, rData);
}

static void msg22xx_DrvFwCtrlMsg22xxChangeVoltage(void)
{
	unsigned short nTrimValue = 0;
	unsigned short nNewTrimValue = 0;
	unsigned short nTempValue = 0;

	dprintk("*** %s() ***\n", __func__);

	write_reg(msg22xx_data, 0x18, 0x40, 0xA55A);

	udelay(1000); // delay 1 ms

	nTrimValue = read_reg(msg22xx_data, 0x18, 0x20);

	udelay(1000); // delay 1 ms

	nTrimValue = nTrimValue & 0x1F;
	nTempValue = 0x1F & nTrimValue;
	nNewTrimValue = (nTempValue + 0x07);

	if (nNewTrimValue >= 0x20) {
		nNewTrimValue = nNewTrimValue - 0x20;
	} else {
		nNewTrimValue = nNewTrimValue;
	}

	if ((nTempValue & 0x10) != 0x10) {
		if (nNewTrimValue >= 0x0F && nNewTrimValue < 0x1F) {
			nNewTrimValue = 0x0F;
		} else {
			nNewTrimValue = nNewTrimValue;
		}
	}

	write_reg(msg22xx_data, 0x18, 0x42, nNewTrimValue);

	udelay(1000); // delay 1 ms

	msg22xx_RegSet16BitValueOn(0x18, 0x42, BIT5);

	udelay(1000); // delay 1 ms
}

static void msg22xx_DrvFwCtrlMsg22xxRestoreVoltage(void)
{
	dprintk("*** %s() ***\n", __func__);

	msg22xx_RegSet16BitValueOff(0x18, 0x42, BIT5);

	udelay(1000); // delay 1 ms

	write_reg(msg22xx_data, 0x18, 0x40, 0x0000);

	udelay(1000); // delay 1 ms
}

static void msg22xx_DrvFwCtrlMsg22xxEraseEmem(EmemType_e eEmemType)
{
	unsigned int i = 0;
	unsigned int nEraseCount = 0;
	unsigned int nMaxEraseTimes = 0;
	unsigned int nTimeOut = 0;
	unsigned short nRegData = 0;
	unsigned short nTrimByte1 = 0;

	dprintk("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

	dbbusDWIICEnterSerialDebugMode(msg22xx_data);
	dbbusDWIICStopMCU(msg22xx_data);
	dbbusDWIICIICUseBus(msg22xx_data);
	dbbusDWIICIICReshape(msg22xx_data);
	mdelay(100);

	dprintk("Erase start\n");

	/* stop mcu */
	write_reg(msg22xx_data, 0x0F, 0xE6, 0x0001);

	nTrimByte1 = msg22xx_DrvFwCtrlMsg22xxGetTrimByte1();

	msg22xx_DrvFwCtrlMsg22xxChangeVoltage();

	/* disable watch dog */
	write_reg_8bit(msg22xx_data, 0x3C, 0x60, 0x55);
	write_reg_8bit(msg22xx_data, 0x3C, 0x61, 0xAA);

	/* set PROGRAM password */
	write_reg_8bit(msg22xx_data, 0x16, 0x1A, 0xBA);
	write_reg_8bit(msg22xx_data, 0x16, 0x1B, 0xAB);

	if (nTrimByte1 == 0xCA) {
		nMaxEraseTimes = MAX_ERASE_EFLASH_TIMES;
	} else {
		nMaxEraseTimes = 1;
	}

	for (nEraseCount = 0; nEraseCount < nMaxEraseTimes; nEraseCount ++) {
		if(eEmemType == EMEM_ALL) { // 48KB + 512Byte
			dprintk("Erase all block %d times\n", nEraseCount);

			// Clear pce
			write_reg_8bit(msg22xx_data, 0x16, 0x18, 0x80);
			mdelay(100);

			// Chip erase
			write_reg(msg22xx_data, 0x16, 0x0E, BIT3);

			dprintk("Wait erase done flag\n");

			while (1) { // Wait erase done flag
				nRegData = read_reg(msg22xx_data, 0x16, 0x10);// Memory status
				nRegData = nRegData & BIT1;

				dprintk("Wait erase done flag nRegData = 0x%x\n", nRegData);

				if (nRegData == BIT1) {
					break;
				}

				mdelay(50);

				if ((nTimeOut ++) > 30) {
					dprintk("Erase all block %d times failed. Timeout.\n", nEraseCount);

					if (nEraseCount == (nMaxEraseTimes - 1)) {
						goto EraseEnd;
					}
				}
			}
		}else if(eEmemType == EMEM_MAIN) {// 48KB (32+8+8)
			dprintk("Erase main block %d times\n", nEraseCount);

			for (i = 0; i < 3; i ++) {
				// Clear pce
				write_reg_8bit(msg22xx_data, 0x16, 0x18, 0x80);
				mdelay(10);

				if (i == 0) {
					write_reg(msg22xx_data, 0x16, 0x00, 0x0000);
				} else if (i == 1) {
					write_reg(msg22xx_data, 0x16, 0x00, 0x8000);
				} else if (i == 2) {
					write_reg(msg22xx_data, 0x16, 0x00, 0xA000);
				}

				// Sector erase
				write_reg(msg22xx_data, 0x16, 0x0E, (read_reg(msg22xx_data, 0x16, 0x0E) | BIT2));

				dprintk("Wait erase done flag\n");

				nRegData = 0;
				nTimeOut = 0;

				while (1) {  // Wait erase done flag
					nRegData = read_reg(msg22xx_data, 0x16, 0x10);//Memory status
					nRegData = nRegData & BIT1;

					dprintk("Wait erase done flag nRegData = 0x%x\n", nRegData);

					if (nRegData == BIT1) {
						break;
					}
					mdelay(50);

					if ((nTimeOut ++) > 30)
					{
						dprintk("Erase main block %d times failed. Timeout.\n", nEraseCount);

						if (nEraseCount == (nMaxEraseTimes - 1))
						{
							goto EraseEnd;
						}
					}
				}
			}
		}		else if (eEmemType == EMEM_INFO) {  // 512Byte
			dprintk("Erase info block %d times\n", nEraseCount);

			// Clear pce
			write_reg_8bit(msg22xx_data, 0x16, 0x18, 0x80);
			mdelay(10);

			write_reg(msg22xx_data, 0x16, 0x00, 0xC000);

			// Sector erase
			write_reg(msg22xx_data, 0x16, 0x0E, (read_reg(msg22xx_data, 0x16, 0x0E) | BIT2));

			dprintk("Wait erase done flag\n");

			while (1) { // Wait erase done flag
				nRegData = read_reg(msg22xx_data, 0x16, 0x10); // Memory status
				nRegData = nRegData & BIT1;

				dprintk("Wait erase done flag nRegData = 0x%x\n", nRegData);

				if (nRegData == BIT1) {
					break;
				}
				mdelay(50);

				if ((nTimeOut ++) > 30) {
					dprintk("Erase info block %d times failed. Timeout.\n", nEraseCount);

					if(nEraseCount == (nMaxEraseTimes - 1)) {
						goto EraseEnd;
					}
				}
			}
		}
	}

	msg22xx_DrvFwCtrlMsg22xxRestoreVoltage();

EraseEnd:
	dprintk("Erase end\n");
	dbbusIICNotUseBus(msg22xx_data);
	dbbusNotStopMCU(msg22xx_data);
	dbbusExitSerialDebugMode(msg22xx_data);
}

static void msg22xx_DrvFwCtrlMsg22xxProgramEmem(EmemType_e eEmemType)
{
	unsigned int i, j;
	unsigned int nRemainSize = 0, nBlockSize = 0, nSize = 0, index = 0;
	unsigned int nTimeOut = 0;
	unsigned short nRegData = 0;
	unsigned char szDbBusTxData[128] = {0};

	unsigned int nSizePerWrite = 125;

	dprintk("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

	dbbusDWIICEnterSerialDebugMode(msg22xx_data);
	dbbusDWIICStopMCU(msg22xx_data);
	dbbusDWIICIICUseBus(msg22xx_data);
	dbbusDWIICIICReshape(msg22xx_data);

	// Hold reset pin before program
	write_reg_8bit(msg22xx_data, 0x1E, 0x06, 0x00);

	dprintk("Program start\n");

	write_reg(msg22xx_data, 0x16, 0x1A, 0xABBA);

	write_reg(msg22xx_data, 0x16, 0x18, (read_reg(msg22xx_data, 0x16, 0x18) | 0x80));

	if (eEmemType == EMEM_MAIN) {
		dprintk("Program main block\n");
		write_reg(msg22xx_data, 0x16, 0x00, 0x0000);// Set start address of main block
		nRemainSize = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024; //48KB
		index = 0;
	} else if (eEmemType == EMEM_INFO) {
		dprintk("Program info block\n");
		write_reg(msg22xx_data, 0x16, 0x00, 0xC000);// Set start address of info block
		nRemainSize = MSG22XX_FIRMWARE_INFO_BLOCK_SIZE; //512Byte
		index = MSG22XX_FIRMWARE_MAIN_BLOCK_SIZE * 1024;
	} else {
		dprintk("eEmemType = %d is not supported for program e-memory.\n", eEmemType);
		return;
	}
	write_reg(msg22xx_data, 0x16, 0x0C, (read_reg(msg22xx_data, 0x16, 0x0C) | 0x01));
	// Program start
	szDbBusTxData[0] = 0x10;
	szDbBusTxData[1] = 0x16;
	szDbBusTxData[2] = 0x02;

	write_i2c_seq(msg22xx_data, SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 3);

	szDbBusTxData[0] = 0x20;

	write_i2c_seq(msg22xx_data, SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);

	i = 0;

	while (nRemainSize > 0) {
		if (nRemainSize > nSizePerWrite) {
			nBlockSize = nSizePerWrite;
		} else {
			nBlockSize = nRemainSize;
		}

		szDbBusTxData[0] = 0x10;
		szDbBusTxData[1] = 0x16;
		szDbBusTxData[2] = 0x02;

		nSize = 3;

		for (j = 0; j < nBlockSize; j ++) {
			szDbBusTxData[3+j] = g_OneDimenFwData[index+(i*nSizePerWrite)+j];
			nSize ++;
		}
		i ++;
		write_i2c_seq(msg22xx_data, SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], nSize);
		nRemainSize = nRemainSize - nBlockSize;
	}

	// Program end
	szDbBusTxData[0] = 0x21;

	write_i2c_seq(msg22xx_data, SLAVE_I2C_ID_DBBUS, &szDbBusTxData[0], 1);

	write_reg(msg22xx_data, 0x16, 0x0C, (read_reg(msg22xx_data, 0x16, 0x0C) & (~0x01)));

	dprintk("Wait write done flag\n");

	while (1) { // Wait write done flag
		// Polling 0x1610 is 0x0002
		// Memory status
		nRegData = read_reg(msg22xx_data, 0x16, 0x10);
		nRegData = nRegData & BIT1;

		dprintk("Wait write done flag nRegData = 0x%x\n", nRegData);

		if (nRegData == BIT1) {
			break;
		}
		mdelay(10);

		if ((nTimeOut ++) > 30) {
			dprintk("Write failed. Timeout.\n");

			goto ProgramEnd;
		}
	}

ProgramEnd:
	dprintk("Program end\n");
	dbbusIICNotUseBus(msg22xx_data);
	dbbusNotStopMCU(msg22xx_data);
	dbbusExitSerialDebugMode(msg22xx_data);
}

static unsigned int msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(unsigned char szTmpBuf[], EmemType_e eEmemType)
{
	unsigned int nRetVal = 0;

	dprintk("*** %s() eEmemType = %d ***\n", __func__, eEmemType);

	if (szTmpBuf != NULL) {
		if (eEmemType == EMEM_MAIN) {  // Read main block CRC(48KB-4) from bin file
			nRetVal  = szTmpBuf[0xBFFF] << 24;
			nRetVal |= szTmpBuf[0xBFFE] << 16;
			nRetVal |= szTmpBuf[0xBFFD] << 8;
			nRetVal |= szTmpBuf[0xBFFC];
		} else if (eEmemType == EMEM_INFO) {  // Read info block CRC(512Byte-4) from bin file
			nRetVal  = szTmpBuf[0xC1FF] << 24;
			nRetVal |= szTmpBuf[0xC1FE] << 16;
			nRetVal |= szTmpBuf[0xC1FD] << 8;
			nRetVal |= szTmpBuf[0xC1FC];
		}
	}

	return nRetVal;
}

static int msg22xx_DrvFwCtrlMsg22xxUpdateFirmwareBySwId(void)
{
	int nRetVal = -1;
	unsigned int nCrcInfoA = 0, nCrcInfoB = 0, nCrcMainA = 0, nCrcMainB = 0;

	dprintk("*** %s() ***\n", __func__);

	dprintk("g_IsUpdateInfoBlockFirst = %d, g_IsUpdateFirmware = 0x%x\n", g_IsUpdateInfoBlockFirst, g_IsUpdateFirmware);

	msg22xx_convert_data_to_onedimen(g_FwData, g_OneDimenFwData);

	if (g_IsUpdateInfoBlockFirst == 1) {
		if ((g_IsUpdateFirmware & 0x10) == 0x10) {
			msg22xx_DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
			msg22xx_DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

			nCrcInfoA = msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(g_OneDimenFwData, EMEM_INFO);
			nCrcInfoB = msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

			dprintk("nCrcInfoA = 0x%x, nCrcInfoB = 0x%x\n", nCrcInfoA, nCrcInfoB);

			if (nCrcInfoA == nCrcInfoB) {
				msg22xx_DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
				msg22xx_DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

				nCrcMainA = msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(g_OneDimenFwData, EMEM_MAIN);
				nCrcMainB = msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

				dprintk("nCrcMainA = 0x%x, nCrcMainB = 0x%x\n", nCrcMainA, nCrcMainB);

				if (nCrcMainA == nCrcMainB) {
					g_IsUpdateFirmware = 0x00;
					nRetVal = 0;
				} else {
					g_IsUpdateFirmware = 0x01;
				}
			} else {
				g_IsUpdateFirmware = 0x11;
			}
		} else if ((g_IsUpdateFirmware & 0x01) == 0x01) {
			msg22xx_DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
			msg22xx_DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

			nCrcMainA = msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(g_OneDimenFwData, EMEM_MAIN);
			nCrcMainB = msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

			dprintk("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

			if (nCrcMainA == nCrcMainB) {
				g_IsUpdateFirmware = 0x00;
				nRetVal = 0;
			} else {
				g_IsUpdateFirmware = 0x01;
			}
		}
	} else {  //g_IsUpdateInfoBlockFirst ==  0
		if ((g_IsUpdateFirmware & 0x10) == 0x10) {
			msg22xx_DrvFwCtrlMsg22xxEraseEmem(EMEM_MAIN);
			msg22xx_DrvFwCtrlMsg22xxProgramEmem(EMEM_MAIN);

			nCrcMainA = msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(g_OneDimenFwData, EMEM_MAIN);
			nCrcMainB = msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_MAIN);

			dprintk("nCrcMainA=0x%x, nCrcMainB=0x%x\n", nCrcMainA, nCrcMainB);

			if (nCrcMainA == nCrcMainB) {
				msg22xx_DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
				msg22xx_DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

				nCrcInfoA = msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(g_OneDimenFwData, EMEM_INFO);
				nCrcInfoB = msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

				dprintk("nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

				if (nCrcInfoA == nCrcInfoB) {
					g_IsUpdateFirmware = 0x00;
					nRetVal = 0;
				} else {
					g_IsUpdateFirmware = 0x01;
				}
			} else {
				g_IsUpdateFirmware = 0x11;
			}
		} else if ((g_IsUpdateFirmware & 0x01) == 0x01) {
			msg22xx_DrvFwCtrlMsg22xxEraseEmem(EMEM_INFO);
			msg22xx_DrvFwCtrlMsg22xxProgramEmem(EMEM_INFO);

			nCrcInfoA = msg22xx_DrvFwCtrlMsg22xxRetrieveFrimwareCrcFromBinFile(g_OneDimenFwData, EMEM_INFO);
			nCrcInfoB = msg22xx_DrvFwCtrlMsg22xxGetFirmwareCrcByHardware(EMEM_INFO);

			dprintk("nCrcInfoA=0x%x, nCrcInfoB=0x%x\n", nCrcInfoA, nCrcInfoB);

			if (nCrcInfoA == nCrcInfoB) {
				g_IsUpdateFirmware = 0x00;
				nRetVal = 0;
			} else {
				g_IsUpdateFirmware = 0x01;
			}
		}
	}

	return nRetVal;
}

static void msg22xx_DrvFwCtrlUpdateFirmwareBySwIdDoWork(struct work_struct *pWork)
{
	int nRetVal = 0;

	dprintk("*** %s() g_UpdateRetryCount = %d ***\n", __func__, g_UpdateRetryCount);

	nRetVal = msg22xx_DrvFwCtrlMsg22xxUpdateFirmwareBySwId();

	dprintk("*** update firmware by sw id result = %d ***\n", nRetVal);

	if (nRetVal == 0) {
		dprintk("update firmware by sw id success\n");
		msg22xx_reset_hw(msg22xx_data->pdata);

		msg22xx_enable_irq(msg22xx_data);

		g_IsUpdateInfoBlockFirst = 0;
		g_IsUpdateFirmware = 0x00;
	} else { //nRetVal == -1
		g_UpdateRetryCount --;
		if (g_UpdateRetryCount > 0) {
			dprintk("g_UpdateRetryCount = %d\n", g_UpdateRetryCount);
			queue_work(g_UpdateFirmwareBySwIdWorkQueue, &g_UpdateFirmwareBySwIdWork);
		} else {
			dprintk("update firmware by sw id failed\n");

			msg22xx_reset_hw(msg22xx_data->pdata);

			msg22xx_enable_irq(msg22xx_data);

			g_IsUpdateInfoBlockFirst = 0;
			g_IsUpdateFirmware = 0x00;
		}
	}

	g_FwUpdateState = false;
}

static int msg22xx_prepare_fw_data(struct device *dev)
{
	int count;
	int i;
	int ret;
	int major, minor;
	const struct firmware *fw = NULL;
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);

	ret = request_firmware(&fw, ts_data->pdata->fw_name, dev);
	if (ret < 0) {
		dev_err(dev, "Request firmware failed - %s (%d)\n",
			ts_data->pdata->fw_name, ret);
		return ret;
	}

	major = fw->data[0xBFF5]<<8 | fw->data[0xBFF4];
	minor = fw->data[0xBFF7]<<8 | fw->data[0xBFF6];

	count = fw->size / 1024;

	/* msg2238 48.5KB */
	for (i = 0; i < count; i++) {
		memcpy(g_FwData[i], fw->data + (i * 1024), 1024);
	}

	if (fw->size > count * 1024) {
		memcpy(g_FwData[i], fw->data + (i * 1024), 512);
	}

	dprintk("New firmware: %d.%d\n", major, minor);

	return fw->size;
}

static void msg22xx_DrvFwCtrlMsg22xxCheckFirmwareUpdateBySwId(struct msg22xx_ts_data *ts_data)
{
	unsigned short nUpdateBinMajor = 0, nUpdateBinMinor = 0;
	unsigned short nMajor = 0, nMinor = 0;
	unsigned char *pVersion = NULL;

	dprintk("*** %s() ***\n", __func__);

	msg22xx_disable_irq(ts_data);

	g_UpdateFirmwareBySwIdWorkQueue = create_singlethread_workqueue("update_firmware_by_sw_id");
	INIT_WORK(&g_UpdateFirmwareBySwIdWork, msg22xx_DrvFwCtrlUpdateFirmwareBySwIdDoWork);

	msg22xx_DrvFwCtrlGetCustomerFirmwareVersion(&nMajor, &nMinor, &pVersion);

	/* if the header file is the only one, copy the data and ignore SW_ID */
	if ((g_SwId == MSG22XX_SW_ID_BOOYI) || (g_SwId == MSG22XX_SW_ID_DJN)) {
		nUpdateBinMajor = g_FwData[0x2F][0x03F5]<<8 | g_FwData[0x2F][0x03F4]; // 0xBFF5  0xBFF4
		nUpdateBinMinor = g_FwData[0x2F][0x03F7]<<8 | g_FwData[0x2F][0x03F6];//0xBFF7  0xBFF6
	} else { //g_SwId == MSG22XX_SW_ID_UNDEFINED
		dprintk("g_SwId = 0x%x is an undefined SW ID.\n", g_SwId);

		g_SwId = MSG22XX_SW_ID_UNDEFINED;
		nUpdateBinMajor = 0;
		nUpdateBinMinor = 0;
	}

	dprintk("g_SwId=0x%x, nMajor=%d, nMinor=%d, nUpdateBinMajor=%d, nUpdateBinMinor=%d\n", g_SwId, nMajor, nMinor, nUpdateBinMajor, nUpdateBinMinor);

	if (nUpdateBinMinor > nMinor) {
		if(g_SwId < MSG22XX_SW_ID_UNDEFINED && g_SwId != 0x0000 && g_SwId != 0xFFFF) {
			g_FwDataCount = 0; // Reset g_FwDataCount to 0 after copying update firmware data to temp buffer

			g_UpdateRetryCount = UPDATE_FIRMWARE_RETRY_COUNT;
			g_IsUpdateInfoBlockFirst = 1; // Set 1 for indicating main block is complete
			g_IsUpdateFirmware = 0x11;
			queue_work(g_UpdateFirmwareBySwIdWorkQueue, &g_UpdateFirmwareBySwIdWork);
			return;
		} else {
			dprintk("The sw id is invalid.\n");
			dprintk("Go to normal boot up process.\n");
		}
	} else {
		dprintk("The update bin version is older than or equal to the current firmware version on e-flash.\n");
		dprintk("Go to normal boot up process.\n");
	}

	g_FwUpdateState = false;

	msg22xx_reset_hw(ts_data->pdata);

	msg22xx_enable_irq(ts_data);
}

static ssize_t msg22xx_fw_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);
	return snprintf(buf, MSTAR_FW_NAME_MAX_LEN - 1, "%s\n", ts_data->pdata->fw_name);
}

static ssize_t msg22xx_fw_name_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned char *vendor_djn = "djn";
	unsigned char *vendor_booyi = "booyi";
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);

	if (size > MSTAR_FW_NAME_MAX_LEN - 1) {
		return -EINVAL;
	}

	g_SwId = msg22xx_DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
	dprintk("%s:size is %d, name is %s", __func__,size, buf);

	if (((MSG22XX_SW_ID_DJN == g_SwId) && (NULL != strstr(buf, vendor_djn))) || \
		((MSG22XX_SW_ID_BOOYI == g_SwId) && (NULL != strstr(buf, vendor_booyi)))) {
		memcpy(ts_data->pdata->fw_name, buf, size);
	} else {
		dprintk("%s:wrong name, and do't handle it", __func__);
	}

	return size;
}

static DEVICE_ATTR(fw_name, (S_IRUGO | S_IWUSR), msg22xx_fw_name_show, msg22xx_fw_name_store);

static ssize_t msg22xx_firmware_update_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 3, "%d\n", g_FwUpdateState);
}

static ssize_t msg22xx_firmware_update_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	struct msg22xx_ts_data *ts_data = dev_get_drvdata(dev);
	g_FwUpdateState = true;

	ret = msg22xx_prepare_fw_data(dev);
	if (ret < 0) {
		dev_err(dev, "Request firmware failed -(%d)\n", ret);
		g_FwUpdateState = false;
		return ret;
	}
	msg22xx_DrvFwCtrlMsg22xxCheckFirmwareUpdateBySwId(ts_data);

	// if touch screen is waiting for gesture wakeup
#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
	if((msg22xx_gesture_mode != 0) && (ts_gesture_wakeup_device_enable == true)) {
		if(device_may_wakeup(&ts_data->client->dev)) {
			enable_irq_wake(ts_data->client->irq);
			dprintk("\n %s be a wakeup irq\n", __func__);
		}

		if (ts_gesture_wakeup_mode[0] != 0x00000000 || ts_gesture_wakeup_mode[1] != 0x00000000) {
			msg22xx_open_gesture_wakeup(ts_data, &ts_gesture_wakeup_mode[0]);
			return 0;
		}
	}
#endif

	return ret;
}

static DEVICE_ATTR(fw_update, (S_IRUGO | S_IWUSR), msg22xx_firmware_update_show, msg22xx_firmware_update_store);

static ssize_t msg22xx_hw_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned short fw_major = 0;
	unsigned short fw_minor = 0;
	unsigned char *pVersion = NULL;
	unsigned char touch_ic[2][TOUCHSCREEN_INFO_MAX] = {{"djn"}, {"booyi"}};
	unsigned char touch_update[2][TOUCHSCREEN_INFO_MAX] = {{"djn_mstar_firmware.fw"}, {"booyi_mstar_firmware.fw"}};
	unsigned char touchs_tpye[] = "msg2238";
	int vendor = 0;
	int ret = 0;
	int update_major = 0;
	int update_minor = 0;
	const struct firmware *fw = NULL;
	int result = 0;

	if ((true == ts_gesture_wakeup_device_enable) || (1 == ts_resume_suspend_flag)) {
		dprintk("touchscreen don't support to read when power off");
		return sprintf(buf, "touchscreen don't support to read when power off\n");
	}

	/* add vendor */
	g_SwId = msg22xx_DrvFwCtrlMsg22xxGetSwId(EMEM_MAIN);
	if (MSG22XX_SW_ID_DJN == g_SwId) {
		vendor = 0;
	} else if (MSG22XX_SW_ID_BOOYI == g_SwId) {
		vendor = 1;
	}

	/* add fw version */
	msg22xx_DrvFwCtrlGetCustomerFirmwareVersion(&fw_major, &fw_minor, &pVersion);

	result = request_firmware(&fw, &touch_update[vendor][0], dev);
	if (result < 0) {
		dev_err(dev, "Request firmware failed - %s (%d)\n", &touch_update[vendor][0], result);
	} else {
		update_major = fw->data[0xBFF5]<<8 | fw->data[0xBFF4];
		update_minor = fw->data[0xBFF7]<<8 | fw->data[0xBFF6];
	}

	ret = scnprintf(buf, PAGE_SIZE,
			"type:\t%s\n"
			"vendor:\t%s\n"
			"firmware_version:\t%d.%d\n"
			"update_version:\t%d.%d\n"
			"driver_version:\t%s\n",
			touchs_tpye,
			&touch_ic[vendor][0],
			fw_major, fw_minor,
			update_major, update_minor,
			TOUCH_DRIVER_VERSION);

	return ret;
}

static DEVICE_ATTR(hw_info, (S_IRUGO | S_IWUSR), msg22xx_hw_info_show, NULL);

/* probe function is used for matching and initializing input device */
static int msg22xx_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0, i;

	struct input_dev *input_dev;
	struct msg22xx_ts_data *ts_data;
	struct msg22xx_ts_platform_data *pdata;

	dprintk("\n %s \n", __func__);

	if(client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(struct msg22xx_ts_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		ret = msg22xx_parse_dt(&client->dev, pdata);
		if (ret) {
			dev_err(&client->dev, "DT parsing failed\n");
			return ret;
		}
	} else
		pdata = client->dev.platform_data;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C not supported\n");
		return -ENODEV;
	}

	ts_data = devm_kzalloc(&client->dev, sizeof(struct msg22xx_ts_data), GFP_KERNEL);
	if (!ts_data) {
		dev_err(&client->dev, "Not enough memory\n");
		return -ENOMEM;
	}

	ts_data->client = client;
	ts_data->info.point = devm_kzalloc(&client->dev,
		sizeof(struct touchPoint_t) * pdata->num_max_touches, GFP_KERNEL);
	if (!ts_data->info.point) {
		dev_err(&client->dev, "Not enough memory\n");
		return -ENOMEM;
	}

	/* allocate an input device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "input device allocation failed\n");
		goto err_input_allocate_dev;
	}

	input_dev->name = client->name;
	input_dev->phys = "I2C";
	input_dev->dev.parent = &client->dev;
	input_dev->id.bustype = BUS_I2C;

	ts_data->input_dev = input_dev;
	ts_data->client = client;
	ts_data->pdata = pdata;

	input_set_drvdata(input_dev, ts_data);
	i2c_set_clientdata(client, ts_data);

	ret = msg22xx_ts_power_init(ts_data, true);
	if (ret) {
		dev_err(&client->dev, "Mstar power init failed\n");
		return ret;
	}

	ret = msg22xx_ts_power_on(ts_data, true);
	if (ret) {
		dev_err(&client->dev, "Mstar power on failed\n");
		goto exit_deinit_power;
	}

	ret = msg22xx_pinctrl_init(ts_data);
	if (!ret && ts_data->ts_pinctrl) {
	/*
	* Pinctrl handle is optional. If pinctrl handle is found
	* let pins to be configured in active state. If not
	* found continue further without error.
	*/
        ret = pinctrl_select_state(ts_data->ts_pinctrl, ts_data->pinctrl_state_reset_active);
        if (ret < 0){
            dev_err(&client->dev,
            "Failed to select %s pinatate %d\n", PINCTRL_STATE_RESET_ACTIVE, ret);
        }
        ret = pinctrl_select_state(ts_data->ts_pinctrl, ts_data->pinctrl_state_int_suspend);
        if (ret < 0){
            dev_err(&client->dev,
            "Failed to select %s pinatate %d\n", PINCTRL_STATE_INT_SUSPEND, ret);
        }
	}

	ret = msg22xx_ts_gpio_configure(ts_data, true);
	if (ret) {
		dev_err(&client->dev, "Failed to configure gpio %d\n", ret);
		goto exit_gpio_config;
	}

	if (msg22xx_get_ic_type(ts_data) == 0) {
		dev_err(&client->dev, "The current IC is not Mstar\n");
		ret = -1;
		goto err_wrong_ic_type;
	}

	/*
	* Pull up gpio_int after reset
	*/
	if (ts_data->ts_pinctrl) {
        ret = pinctrl_select_state(ts_data->ts_pinctrl, ts_data->pinctrl_state_int_active);
        if (ret < 0){
            dev_err(&client->dev,
            "Failed to select %s pinatate %d\n", PINCTRL_STATE_INT_ACTIVE, ret);
        }
	}

	mutex_init(&msg22xx_mutex);
	mutex_init(&ts_data->ts_mutex);

	/* set the supported event type for input device */
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	for (i = 0; i < pdata->num_buttons; i++)
		input_set_capability(input_dev, EV_KEY, pdata->button_map[i]);

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_UP);
	input_set_capability(input_dev, EV_KEY, KEY_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_LEFT);
	input_set_capability(input_dev, EV_KEY, KEY_RIGHT);
	input_set_capability(input_dev, EV_KEY, KEY_W);
	input_set_capability(input_dev, EV_KEY, KEY_Z);
	input_set_capability(input_dev, EV_KEY, KEY_V);
	input_set_capability(input_dev, EV_KEY, KEY_O);
	input_set_capability(input_dev, EV_KEY, KEY_M);
	input_set_capability(input_dev, EV_KEY, KEY_C);
	input_set_capability(input_dev, EV_KEY, KEY_E);
	input_set_capability(input_dev, EV_KEY, KEY_S);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 2, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, pdata->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, pdata->y_max, 0, 0);
	ret = input_mt_init_slots(input_dev,  pdata->num_max_touches, 0);
	if (ret) {
		dev_err(&client->dev, "Error %d initialising slots\n", ret);
		goto err_free_mem;
	}

	/* register the input device to input sub-system */
	ret = input_register_device(input_dev);
	if (ret < 0) {
		dev_err(&client->dev, "Unable to register ms-touchscreen input device\n");
		goto err_input_reg_dev;
	}

	/* gesture wakeup  */
	if (device_create_file(&client->dev, &dev_attr_gesture) < 0) {
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
				dev_attr_gesture.attr.name);
		goto err_create_gesture_file;
	}

	/* fw update name  */
	if (device_create_file(&client->dev, &dev_attr_fw_name) < 0) {
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
				dev_attr_fw_name.attr.name);
		goto err_create_name_file;
	}

	/* fw update  */
	g_FwUpdateState = false;
	if (device_create_file(&client->dev, &dev_attr_fw_update) < 0) {
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
				dev_attr_fw_update.attr.name);
		goto err_create_update_file;
	}

	/* hw_info  */
	if (device_create_file(&client->dev, &dev_attr_hw_info) < 0) {
		dev_err(&client->dev, "Failed to create device file(%s)!\n",
				dev_attr_hw_info.attr.name);
		goto err_create_hw_file;
	}

#ifdef TP_PRINT
	tp_print_create_entry(ts_data);
#endif
	ret = request_threaded_irq(client->irq, NULL, msg22xx_ts_interrupt,
			pdata->irq_gpio_flags | IRQF_ONESHOT | IRQF_TRIGGER_RISING,
			"msg22xx", ts_data);
	if (ret)
		goto err_req_irq;

	spin_lock_init(&msg22xx_irqlock);

	msg22xx_disable_irq(ts_data);

#if defined(CONFIG_FB)
	ts_data->fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&ts_data->fb_notif);
#endif

#ifdef CONFIG_TOUCHSCREEN_PROXIMITY_SENSOR
	tsps_assist_register_callback("msg22xx", &tsps_msg22xx_enable, &tsps_msg22xx_data);
#endif
	dprintk("\n mstar touch screen registered \n");

	/* be a wake source */
	device_init_wakeup(&ts_data->client->dev, 1);

	msg22xx_data = ts_data;
	msg22xx_enable_irq(ts_data);

	return 0;

err_req_irq:
	free_irq(client->irq, ts_data);
err_create_hw_file:
	device_remove_file(&client->dev, &dev_attr_hw_info);
err_create_update_file:
	device_remove_file(&client->dev, &dev_attr_fw_update);
err_create_name_file:
	device_remove_file(&client->dev, &dev_attr_fw_name);
err_create_gesture_file:
	device_remove_file(&client->dev, &dev_attr_gesture);
err_input_reg_dev:
	input_free_device(input_dev);
	input_dev = NULL;
err_input_allocate_dev:
	mutex_destroy(&msg22xx_mutex);
	mutex_destroy(&ts_data->ts_mutex);
err_wrong_ic_type:
	msg22xx_ts_gpio_configure(ts_data, false);
exit_gpio_config:
	if (ts_data->ts_pinctrl) {
		if (IS_ERR_OR_NULL(ts_data->pinctrl_state_release)) {
			devm_pinctrl_put(ts_data->ts_pinctrl);
			ts_data->ts_pinctrl = NULL;
		} else {
			ret = pinctrl_select_state(ts_data->ts_pinctrl,
				ts_data->pinctrl_state_release);
			if (ret < 0)
				dev_err(&ts_data->client->dev,
				"Cannot get release pinctrl state\n");
		}
	}
	msg22xx_ts_power_on(ts_data, false);
exit_deinit_power:
	msg22xx_ts_power_init(ts_data, false);
err_free_mem:
	input_free_device(input_dev);

	return ret;
}

/* remove function is triggered when the input device is removed
from input sub-system */
static int touch_driver_remove(struct i2c_client *client)
{
	int retval = 0;
	struct msg22xx_ts_data *ts_data = i2c_get_clientdata(client);

	free_irq(ts_data->client->irq, ts_data);
	gpio_free(ts_data->pdata->irq_gpio);
	gpio_free(ts_data->pdata->reset_gpio);
	if (gpio_is_valid(ts_data->pdata->cam_avdd_gpio)) {
		gpio_free(ts_data->pdata->cam_avdd_gpio);
	}

	if (ts_data->ts_pinctrl) {
		if (IS_ERR_OR_NULL(ts_data->pinctrl_state_release)) {
			devm_pinctrl_put(ts_data->ts_pinctrl);
			ts_data->ts_pinctrl = NULL;
		} else {
			retval = pinctrl_select_state(ts_data->ts_pinctrl,
				ts_data->pinctrl_state_release);
			if (retval < 0)
				dev_err(&ts_data->client->dev,
				"Cannot get release pinctrl state\n");
		}
	}

	input_unregister_device(ts_data->input_dev);
	mutex_destroy(&msg22xx_mutex);
	mutex_destroy(&ts_data->ts_mutex);

	return retval;
}

/* The I2C device list is used for matching I2C device
and I2C device driver. */
static const struct i2c_device_id touch_device_id[] = {
	{"msg22xx", 0},
	{}, /* should not omitted */
};

static struct of_device_id msg22xx_match_table[] = {
	{ .compatible = "mstar,msg22xx", },
	{ },
};

MODULE_DEVICE_TABLE(i2c, touch_device_id);

static struct i2c_driver touch_device_driver = {
	.driver = {
		.name = "ms-msg22xx",
		.owner = THIS_MODULE,
		.of_match_table = msg22xx_match_table,
	},
	.probe = msg22xx_ts_probe,
	.remove = touch_driver_remove,
	.id_table = touch_device_id,
};

module_i2c_driver(touch_device_driver);

#ifdef TP_PRINT
#include <linux/proc_fs.h>

static unsigned short InfoAddr = 0x0F, PoolAddr = 0x10, TransLen = 256;
static unsigned char row, units, cnt;

static int tp_print_proc_read(struct msg22xx_ts_data *ts_data)
{
	unsigned short i, j;
	unsigned short left, offset = 0;
	unsigned char dbbus_tx_data[3] = {0};
	unsigned char u8Data;
	signed short s16Data;
	int s32Data;
	char *buf = NULL;

	left = cnt*row*units;
	if ((ts_data->suspended == 0) && (InfoAddr != 0x0F) && (PoolAddr != 0x10) &&
		(left > 0)) {
			buf = kmalloc(left, GFP_KERNEL);
			if (buf != NULL) {

				while (left > 0) {
					dbbus_tx_data[0] = 0x53;
					dbbus_tx_data[1] = ((PoolAddr + offset) >> 8)
						& 0xFF;
					dbbus_tx_data[2] = (PoolAddr + offset) & 0xFF;
					mutex_lock(&msg22xx_mutex);
					write_i2c_seq(ts_data, ts_data->client->addr,
						&dbbus_tx_data[0], 3);
					read_i2c_seq(ts_data, ts_data->client->addr,
						&buf[offset],
						left > TransLen ? TransLen : left);
					mutex_unlock(&msg22xx_mutex);

					if (left > TransLen) {
						left -= TransLen;
						offset += TransLen;
					} else {
						left = 0;
					}
				}

				for (i = 0; i < cnt; i++) {
					for (j = 0; j < row; j++) {
						if (units == 1) {
							u8Data = buf[i * row * units +
								j * units];
						} else if (units == 2) {
							s16Data = buf[i * row * units +
								j * units] +
								(buf[i * row * units +
								j * units + 1] << 8);
						} else if (units == 4) {
							s32Data = buf[i * row * units +
								j * units] +
								(buf[i * row * units +
								j * units + 1] << 8) +
								(buf[i * row * units +
								j * units + 2] << 16) +
								(buf[i * row * units +
								j * units + 3] << 24);
						}
					}
				}

				kfree(buf);
			}
	}

	return 0;
}

static void tp_print_create_entry(struct msg22xx_ts_data *ts_data)
{
	unsigned char dbbus_tx_data[3] = {0};
	unsigned char dbbus_rx_data[8] = {0};

	dbbus_tx_data[0] = 0x53;
	dbbus_tx_data[1] = 0x00;
	dbbus_tx_data[2] = 0x58;
	mutex_lock(&msg22xx_mutex);
	write_i2c_seq(ts_data, ts_data->client->addr, &dbbus_tx_data[0], 3);
	read_i2c_seq(ts_data, ts_data->client->addr, &dbbus_rx_data[0], 4);
	mutex_unlock(&msg22xx_mutex);
	InfoAddr = (dbbus_rx_data[1]<<8) + dbbus_rx_data[0];
	PoolAddr = (dbbus_rx_data[3]<<8) + dbbus_rx_data[2];

	if ((InfoAddr != 0x0F) && (PoolAddr != 0x10)) {
		msleep(20);
		dbbus_tx_data[0] = 0x53;
		dbbus_tx_data[1] = (InfoAddr >> 8) & 0xFF;
		dbbus_tx_data[2] = InfoAddr & 0xFF;
		mutex_lock(&msg22xx_mutex);
		write_i2c_seq(ts_data, ts_data->client->addr,
			&dbbus_tx_data[0], 3);
		read_i2c_seq(ts_data, ts_data->client->addr,
			&dbbus_rx_data[0], 8);
		mutex_unlock(&msg22xx_mutex);

		units = dbbus_rx_data[0];
		row = dbbus_rx_data[1];
		cnt = dbbus_rx_data[2];
		TransLen = (dbbus_rx_data[7]<<8) + dbbus_rx_data[6];

		if (device_create_file(&ts_data->client->dev,
			&dev_attr_tpp) < 0)
			dev_err(&ts_data->client->dev, "Failed to create device file(%s)!\n",
			dev_attr_tpp.attr.name);
	}
}
#endif

MODULE_AUTHOR("TPLINK");
MODULE_LICENSE("GPL v2");
