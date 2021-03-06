/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/* DTS2016040502547 chendong 20160407 begin */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/sensors.h>
#include <linux/pm_wakeup.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <misc/app_info.h>

#define AP3426_I2C_NAME			"ap3426"
#define AP3426_LIGHT_INPUT_NAME		"ap3426-light"
#define AP3426_PROXIMITY_INPUT_NAME	"ap3426-proximity"

/* AP3426 registers */
#define AP3426_REG_CONFIG		0x00
#define AP3426_REG_INT_FLAG		0x01
#define AP3426_REG_INT_CTL		0x02
#define AP3426_REG_WAIT_TIME		0x06
#define AP3426_REG_IR_DATA_LOW		0x0A
#define AP3426_REG_IR_DATA_HIGH		0x0B
#define AP3426_REG_ALS_DATA_LOW		0x0C
#define AP3426_REG_ALS_DATA_HIGH	0x0D
#define AP3426_REG_PS_DATA_LOW		0x0E
#define AP3426_REG_PS_DATA_HIGH		0x0F
#define AP3426_REG_ALS_GAIN		0x10
#define AP3426_REG_ALS_PERSIST		0x14
#define AP3426_REG_ALS_LOW_THRES_0	0x1A
#define AP3426_REG_ALS_LOW_THRES_1	0x1B
#define AP3426_REG_ALS_HIGH_THRES_0	0x1C
#define AP3426_REG_ALS_HIGH_THRES_1	0x1D
#define AP3426_REG_PS_GAIN		0x20
#define AP3426_REG_PS_LED_DRIVER	0x21
#define AP3426_REG_PS_INT_FORM		0x22
#define AP3426_REG_PS_MEAN_TIME		0x23
#define AP3426_REG_PS_SMART_INT		0x24
#define AP3426_REG_PS_INT_TIME		0x25
#define AP3426_REG_PS_PERSIST		0x26
#define AP3426_REG_PS_CAL_L		0x28
#define AP3426_REG_PS_CAL_H		0x29
#define AP3426_REG_PS_LOW_THRES_0	0x2A
#define AP3426_REG_PS_LOW_THRES_1	0x2B
#define AP3426_REG_PS_HIGH_THRES_0	0x2C
#define AP3426_REG_PS_HIGH_THRES_1	0x2D

#define AP3426_REG_MAGIC		0xFF
#define AP3426_REG_COUNT		0x2E

#define AP3426_ALS_INT_MASK		0x01
#define AP3426_PS_INT_MASK		0x02

#define ALS_GAIN_SWITCH_RATIO		80

/* AP3426 ALS data is 16 bit */
#define ALS_DATA_MASK		0xffff
#define ALS_LOW_BYTE(data)	((data) & 0xff)
#define ALS_HIGH_BYTE(data)	(((data) >> 8) & 0xff)

/* AP3426 PS data is 10 bit */
#define PS_DATA_MASK		0x3ff
#define PS_LOW_BYTE(data)	((data) & 0xff)
#define PS_HIGH_BYTE(data)	(((data) >> 8) & 0x3)

/* default als sensitivity in lux */
#define AP3426_ALS_SENSITIVITY		50

/* AP3426 takes at least 10ms to boot up */
#define AP3426_BOOT_TIME_MS		12

#define AP3426_CALIBRATE_SAMPLES	15
/* als and ps interrupt enabled, clear by software */
#define AP3426_INT_CONFIG		0x89

/* Any proximity distance change will wakeup SoC */
#define AP3426_WAKEUP_ANY_CHANGE	0xff

#define CAL_BUF_LEN			16
static int flag=0;
static int cali = 100;
#define	DIST_DIFF_LIGHT 	120
#define	ALS_CW_VALUE	24906/10000
#define	ALS_AD_VALUE 34868/10000
#define	ALS_CW_ANGLE 327/200

#define	ALS_THRES_HIGH 	2000
#define	ALS_THRES_LOW 	150
#define	ALS_LIGHTBAR_VALUE 	1000
/* < DTS2015072505158 wuying/wx221431 20150725 begin */
#define AP3426_LUX_MAX	65535
extern bool power_key_ps ;
static int als_polling_count=0;
static int ap3426_debug_mask= 0;
module_param_named(ap3426_debug, ap3426_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
#define AP3426_ERR(x...) do {\
	if (ap3426_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)
#define AP3426_WARN(x...) do {\
	if (ap3426_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)
#define AP3426_INFO(x...) do {\
	if (ap3426_debug_mask >=1) \
        printk(KERN_ERR x);\
    } while (0)
#define AP3426_DEBUG(x...) do {\
    if (ap3426_debug_mask >=2) \
        printk(KERN_ERR x);\
    } while (0)

enum {
	CMD_WRITE = 0,
	CMD_READ = 1,
};

struct regulator_map {
	struct regulator	*regulator;
	int			min_uv;
	int			max_uv;
	char			*supply;
};

struct pinctrl_config {
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*state[2];
	char			*name[2];
};

struct ap3426_data {
	struct i2c_client	*i2c;
	struct regmap		*regmap;
	struct regulator	*config;
	struct input_dev	*input_light;
	struct input_dev	*input_proximity;
	struct workqueue_struct	*workqueue;

	struct sensors_classdev	als_cdev;
	struct sensors_classdev	ps_cdev;
	struct mutex		ops_lock;
	struct work_struct	report_work;
	struct work_struct	als_enable_work;
	struct work_struct	als_disable_work;
	struct work_struct	ps_enable_work;
	struct work_struct	ps_disable_work;
	struct delayed_work powerkey_work;
	atomic_t		wake_count;

	int			irq_gpio;
	int			irq;
	bool			als_enabled;
	bool			ps_enabled;
	u32			irq_flags;
	unsigned int		als_delay;
	unsigned int		ps_delay;
	int			als_cal;
	int			ps_cal;
	int			als_gain;
	int			als_persist;
	int			ps_gain;
	int			ps_persist;
	int			ps_led_driver;
	int			ps_mean_time;
	int			ps_integrated_time;
	int			ps_wakeup_threshold;

	int			last_als;
	int			last_ps;
	int			report_distance;
	int			flush_count;
	int			power_enabled;

	unsigned int		reg_addr;
	char			calibrate_buf[CAL_BUF_LEN];
	unsigned int		bias;
	unsigned int  crosstalk;
	unsigned int prevalue;
};

static struct regulator_map power_config[] = {
	{.supply = "vdd", .min_uv = 2000000, .max_uv = 3300000, },
	{.supply = "vio", .min_uv = 1750000, .max_uv = 1950000, },
};

static struct pinctrl_config pin_config = {
	.name = { "default", "sleep" },
};

static int gain_table[] = { 32768, 8192, 2048, 512 };
/* within 2% percent of jitter will trigger interrupt */
static int sensitivity_table[] = { 3000, 400, 100, 1 };
static int pmt_table[] = { 5, 10, 14, 19 }; /* 5.0 9.6, 14.1 18.7 */

/* PS distance table */
static int ps_distance_table[] = { 100,50};
static int ps_distance_final_table[] = { 100,50};


static struct sensors_classdev als_cdev = {
	.name = "ap3426-light",
	.vendor = "Dyna Image Corporation",
	.version = 1,
	.handle = SENSORS_LIGHT_HANDLE,
	.type = SENSOR_TYPE_LIGHT,
	.max_range = "655360",
	.resolution = "1.0",
	.sensor_power = "0.35",
	.min_delay = 100000,
	.max_delay = 1375,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 2,
	.enabled = 0,
	.delay_msec = 50,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
	.sensors_calibrate = NULL,
	.sensors_write_cal_params = NULL,
	.params = NULL,
};

static struct sensors_classdev ps_cdev = {
	.name = "ap3426-proximity",
	.vendor = "Dyna Image Corporation",
	.version = 1,
	.handle = SENSORS_PROXIMITY_HANDLE,
	.type = SENSOR_TYPE_PROXIMITY,
	.max_range = "6",
	.resolution = "1.0",
	.sensor_power = "0.35",
	.min_delay = 5000,
	.max_delay = 1280,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 3,
	.enabled = 0,
	.delay_msec = 50,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
	.sensors_calibrate = NULL,
	.sensors_write_cal_params = NULL,
	.params = NULL,
};

static int sensor_power_init(struct device *dev, struct regulator_map *map,
		int size)
{
	int rc;
	int i;

	for (i = 0; i < size; i++) {
		map[i].regulator = devm_regulator_get(dev, map[i].supply);
		if (IS_ERR(map[i].regulator)) {
			rc = PTR_ERR(map[i].regulator);
			AP3426_ERR("%s,line %d:Regualtor get failed vdd rc=%d\n",__func__,__LINE__,rc);
			goto exit;
		}
		if (regulator_count_voltages(map[i].regulator) > 0) {
			rc = regulator_set_voltage(map[i].regulator,
					map[i].min_uv, map[i].max_uv);
			if (rc) {
				AP3426_ERR("%s,line %d: Regulator set failed vdd rc=%d\n",__func__,__LINE__,
						rc);
				goto exit;
			}
		}
	}

	return 0;

exit:
	/* Regulator not set correctly */
	for (i = i - 1; i >= 0; i--) {
		if (regulator_count_voltages(map[i].regulator))
			regulator_set_voltage(map[i].regulator, 0,
					map[i].max_uv);
	}

	return rc;
}

static int sensor_power_deinit(struct device *dev, struct regulator_map *map,
		int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (!IS_ERR_OR_NULL(map[i].regulator)) {
			if (regulator_count_voltages(map[i].regulator) > 0)
				regulator_set_voltage(map[i].regulator, 0,
						map[i].max_uv);
		}
	}

	return 0;
}

static int sensor_power_config(struct device *dev, struct regulator_map *map,
		int size, bool enable)
{
	int i;
	int rc = 0;

	if (enable) {
		for (i = 0; i < size; i++) {
			rc = regulator_enable(map[i].regulator);
			if (rc) {
				AP3426_ERR("%s,line %d:enable %s failed.\n",__func__,__LINE__,
						map[i].supply);
				goto exit_enable;
			}
		}
	} else {
		for (i = 0; i < size; i++) {
			rc = regulator_disable(map[i].regulator);
			if (rc) {
				AP3426_ERR("%s,line %d:disable %s failed.\n",__func__,__LINE__,
						map[i].supply);
				goto exit_disable;
			}
		}
	}

	return 0;

exit_enable:
	for (i = i - 1; i >= 0; i--)
		regulator_disable(map[i].regulator);

	return rc;

exit_disable:
	for (i = i - 1; i >= 0; i--)
		if (regulator_enable(map[i].regulator))
			AP3426_ERR("%s,line %d:enable %s failed\n",__func__,__LINE__, map[i].supply);

	return rc;
}

static int sensor_pinctrl_init(struct device *dev,
		struct pinctrl_config *config)
{
	config->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(config->pinctrl)) {
		AP3426_ERR("%s,line %d:Failed to get pinctrl\n",__func__,__LINE__);
		return PTR_ERR(config->pinctrl);
	}

	config->state[0] =
		pinctrl_lookup_state(config->pinctrl, config->name[0]);
	if (IS_ERR_OR_NULL(config->state[0])) {
		AP3426_ERR("%s,line %d:Failed to look up default state\n",__func__,__LINE__);
		return PTR_ERR(config->state[0]);
	}

	config->state[1] =
		pinctrl_lookup_state(config->pinctrl, config->name[1]);
	if (IS_ERR_OR_NULL(config->state[1])) {
		AP3426_ERR("%s,line %d:Failed to look up default state\n",__func__,__LINE__);
		return PTR_ERR(config->state[1]);
	}

	return 0;
}

static int ap3426_parse_dt(struct device *dev, struct ap3426_data *di)
{
	struct device_node *dp = dev->of_node;
	int rc;
	u32 value;
	int i;

	rc = of_get_named_gpio_flags(dp, "di,irq-gpio", 0,
			&di->irq_flags);
	if (rc < 0) {
		AP3426_ERR("%s,line %d:unable to read irq gpio\n",__func__,__LINE__);
		return rc;
	}

	di->irq_gpio = rc;

	rc = of_property_read_u32(dp, "di,als-cal", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,als-cal failed\n",__func__,__LINE__);
		return rc;
	}
	di->als_cal = value;

	rc = of_property_read_u32(dp, "di,als-gain", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,als-gain failed\n",__func__,__LINE__);
		return rc;
	}
	if (value >= ARRAY_SIZE(gain_table)) {
		AP3426_ERR("%s,line %d:di,als-gain out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->als_gain = value;

	rc = of_property_read_u32(dp, "di,als-persist", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,als-persist failed\n",__func__,__LINE__);
		return rc;
	}
	if (value > 0x3f) { /* The maximum value is 63 conversion time. */
		AP3426_ERR("%s,line %d:di,als-persist out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->als_persist = value;

	rc = of_property_read_u32(dp, "di,ps-gain", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,ps-gain failed\n",__func__,__LINE__);
		return rc;
	}
	if (value > 0x03) { /* The maximum value is 3, stands for 8x gain. */
		AP3426_ERR("%s,line %d:proximity gain out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->ps_gain = value;

	rc = of_property_read_u32(dp, "di,ps-persist", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,ps-persist failed\n",__func__,__LINE__);
		return rc;
	}
	if (value > 0x3f) { /* The maximum value is 63 conversion time. */
		AP3426_ERR("%s,line %d:di,ps-persist out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->ps_persist = value;

	rc = of_property_read_u32(dp, "di,ps-led-driver", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,ps-led-driver failed\n",__func__,__LINE__);
		return rc;
	}
	if (value > 0x03) { /* The maximum value is 3, stands for 100% duty. */
		AP3426_ERR("%s,line %d:led driver out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->ps_led_driver = value;

	rc = of_property_read_u32(dp, "di,ps-mean-time", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:di,ps-mean-time incorrect\n",__func__,__LINE__);
		return rc;
	}
	if (value >= ARRAY_SIZE(pmt_table)) {
		AP3426_ERR("%s,line %d:ps mean time out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->ps_mean_time = value;

	rc = of_property_read_u32(dp, "di,ps-integrated-time", &value);
	if (rc) {
		AP3426_ERR("%s,line %d:read di,ps-intergrated-time failed\n",__func__,__LINE__);
		return rc;
	}
	if (value > 0x3f) { /* The maximum value is 63. */
		AP3426_ERR("%s,line %d:ps integrated time out of range\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->ps_integrated_time = value;

	rc = of_property_read_u32(dp, "di,wakeup-threshold", &value);
	if (rc) {
		AP3426_INFO("%s,line %d:di,wakeup-threshold incorrect, drop to default\n",__func__,__LINE__);
		value = AP3426_WAKEUP_ANY_CHANGE;
	}
	if ((value >= ARRAY_SIZE(ps_distance_table)) &&
			(value != AP3426_WAKEUP_ANY_CHANGE)) {
		AP3426_ERR("%s,line %d:wakeup threshold too big\n",__func__,__LINE__);
		return -EINVAL;
	}
	di->ps_wakeup_threshold = value;

	rc = of_property_read_u32_array(dp, "di,als-sensitivity",
			sensitivity_table, ARRAY_SIZE(sensitivity_table));
	if (rc)
		AP3426_INFO("%s,line %d:read di,als-sensitivity failed. Drop to default\n",__func__,__LINE__);

	rc = of_property_read_u32_array(dp, "di,ps-distance-table",
			ps_distance_table, ARRAY_SIZE(ps_distance_table));
	if ((rc == -ENODATA) || (rc == -EOVERFLOW)) {
		AP3426_ERR("%s,line %d:di,ps-distance-table is not correctly set\n",__func__,__LINE__);
		return rc;
	}

	for (i = 1; i < ARRAY_SIZE(ps_distance_table); i++) {
		if (ps_distance_table[i - 1] < ps_distance_table[i]) {
			AP3426_ERR("%s,line %d:ps distance table should in descend order\n",__func__,__LINE__);
			return -EINVAL;
		}
	}

	if (ps_distance_table[0] > PS_DATA_MASK) {
		AP3426_ERR("%s,line %d:distance table out of range\n",__func__,__LINE__);
		return -EINVAL;
	}

	return 0;
}

static int ap3426_check_device(struct ap3426_data *di)
{
	unsigned int part_id;
	int rc;

	/* AP3426 don't have part id registers */
	rc = regmap_read(di->regmap, AP3426_REG_CONFIG, &part_id);
	if (rc) {
		AP3426_ERR("%s,line %d:read reg %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
		return rc;
	}

	AP3426_DEBUG("%s,line %d:register %d:%d\n",__func__,__LINE__, AP3426_REG_CONFIG,
			part_id);
	return 0;
}

static int ap3426_init_input(struct ap3426_data *di)
{
	struct input_dev *input;
	int status;

	input = devm_input_allocate_device(&di->i2c->dev);
	if (!input) {
		AP3426_ERR("%s,line %d:allocate light input device failed\n",__func__,__LINE__);
		return PTR_ERR(input);
	}

	input->name = AP3426_LIGHT_INPUT_NAME;
	input->phys = "ap3426/input0";
	input->id.bustype = BUS_I2C;

	__set_bit(EV_ABS, input->evbit);
	input_set_abs_params(input, ABS_MISC, 0, 655360, 0, 0);

	status = input_register_device(input);
	if (status) {
		AP3426_ERR("%s,line %d:register light input device failed.\n",__func__,__LINE__);
		return status;
	}

	di->input_light = input;

	input = devm_input_allocate_device(&di->i2c->dev);
	if (!input) {
		AP3426_ERR("%s,line %d:allocate light input device failed\n",__func__,__LINE__);
		return PTR_ERR(input);
	}

	input->name = AP3426_PROXIMITY_INPUT_NAME;
	input->phys = "ap3426/input1";
	input->id.bustype = BUS_I2C;

	__set_bit(EV_ABS, input->evbit);
	input_set_abs_params(input, ABS_DISTANCE, 0, 1023, 0, 0);

	status = input_register_device(input);
	if (status) {
		AP3426_ERR("%s,line %d:register proxmity input device failed.\n",__func__,__LINE__);
		return status;
	}

	di->input_proximity = input;

	return 0;
}

static int ap3426_init_device(struct ap3426_data *di)
{
	int rc;

	/* Enable als/ps interrupt and clear interrupt by software */
	rc = regmap_write(di->regmap, AP3426_REG_INT_CTL, AP3426_INT_CONFIG);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_INT_CTL);
		return rc;
	}

	/* Set als gain */
	rc = regmap_write(di->regmap, AP3426_REG_ALS_GAIN, di->als_gain << 4);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_ALS_GAIN);
		return rc;
	}

	/* Set als persistense */
	rc = regmap_write(di->regmap, AP3426_REG_ALS_PERSIST, di->als_persist);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_ALS_PERSIST);
		return rc;
	}

	/* Set ps interrupt form */
	rc = regmap_write(di->regmap, AP3426_REG_PS_INT_FORM, 0);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_INT_FORM);
		return rc;
	}

	/* Set ps gain */
	rc = regmap_write(di->regmap, AP3426_REG_PS_GAIN, di->ps_gain << 2);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_GAIN);
		return rc;
	}

	/* Set ps persist */
	rc = regmap_write(di->regmap, AP3426_REG_PS_PERSIST, di->ps_persist);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_PERSIST);
		return rc;
	}

	/* Set PS LED driver strength */
	rc = regmap_write(di->regmap, AP3426_REG_PS_LED_DRIVER,
			di->ps_led_driver);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_LED_DRIVER);
		return rc;
	}

	/* Set PS mean time */
	rc = regmap_write(di->regmap, AP3426_REG_PS_MEAN_TIME,
			di->ps_mean_time);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_MEAN_TIME);
		return rc;
	}

	/* Set PS integrated time */
	rc = regmap_write(di->regmap, AP3426_REG_PS_INT_TIME,
			di->ps_integrated_time);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_INT_TIME);
		return rc;
	}

	/* Set calibration parameter low byte */
	rc = regmap_write(di->regmap, AP3426_REG_PS_CAL_L,
			PS_LOW_BYTE(di->bias));
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_CAL_L);
		return rc;
	}

	/* Set calibration parameter high byte */
	rc = regmap_write(di->regmap, AP3426_REG_PS_CAL_H,
			PS_HIGH_BYTE(di->bias));
	if (rc) {
		AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
				AP3426_REG_PS_CAL_H);
		return rc;
	}

	AP3426_DEBUG("%s,line %d:ap3426 initialize sucessful\n",__func__,__LINE__);

	return 0;
}

static int ap3426_calc_conversion_time(struct ap3426_data *di, int als_enabled,
		int ps_enabled)
{
	int conversion_time = 0;

	/* ALS conversion time is 100ms */
	if (als_enabled)
		conversion_time = 100;

	if (ps_enabled)
		conversion_time += pmt_table[di->ps_mean_time] +
			di->ps_mean_time * di->ps_integrated_time / 16;
	if((als_enabled) && (ps_enabled))
		conversion_time = 100 +pmt_table[di->ps_mean_time] +
			di->ps_mean_time * di->ps_integrated_time / 16;

	return conversion_time;
}

/* update als gain and threshold */
static int ap3426_als_update_setting(struct ap3426_data *di,
		unsigned int raw_value)
{
	int i;
	int rc;
	unsigned int lux_pre;
	unsigned int config;
	unsigned int adc_threshold;
	unsigned int adc_base;
	int gain_index; /* new gain index */
	u8 als_data[4];

	lux_pre = (raw_value * gain_table[di->als_gain]) >> 16;

	for (i = ARRAY_SIZE(gain_table) - 1; i >= 0; i--) {
		if (lux_pre < gain_table[i] *  ALS_GAIN_SWITCH_RATIO / 100)
			break;
	}

	gain_index = i < 0 ? 0 : i;

	/*
	 * Disable als and enable it again to avoid incorrect value.
	 * Updating als gain during als measurement cycle will cause
	 * incorrect light sensor adc value. The logic here is to handle
	 * this scenario.
	 */
	if (di->als_gain != gain_index) {
		/* read the system config register */
		rc = regmap_read(di->regmap, AP3426_REG_CONFIG, &config);
		if (rc) {
			AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			return rc;
		}

		/* disable als_sensor */
		rc = regmap_write(di->regmap, AP3426_REG_CONFIG,
				config & (~0x01));
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			return rc;
		}

		/* set als gain */
		rc = regmap_write(di->regmap, AP3426_REG_ALS_GAIN, i << 4);
		if (rc) {
			AP3426_ERR("%s,line %d:write %d register failed\n",__func__,__LINE__,
					AP3426_REG_ALS_GAIN);
			return rc;
		}
	}

	adc_base = raw_value * gain_table[di->als_gain] / gain_table[i];
	adc_threshold = ((10 * sensitivity_table[i]) << 16) /
		(di->als_cal * gain_table[i]);
	if (adc_threshold < 1)
		adc_threshold = 1;

	AP3426_DEBUG("%s,line %d:adc_base:%d adc_threshold:%d\n",__func__,__LINE__, adc_base,
			adc_threshold);

	/* lower threshold */
	if (adc_base < adc_threshold) {
		als_data[0] = 0x0;
		als_data[1] = 0x0;
	} else {
		als_data[0] = ALS_LOW_BYTE(adc_base - adc_threshold);
		als_data[1] = ALS_HIGH_BYTE(adc_base - adc_threshold);
	}

	/* upper threshold */
	if (adc_base + adc_threshold > ALS_DATA_MASK) {
		if (di->als_gain != 0) { /* trigger interrupt anyway */
			als_data[2] = als_data[0];
			als_data[3] = als_data[1];
		} else {
			als_data[2] = ALS_LOW_BYTE(ALS_DATA_MASK);
			als_data[3] = ALS_HIGH_BYTE(ALS_DATA_MASK);
		}
	} else {
		als_data[2] = ALS_LOW_BYTE(adc_base + adc_threshold);
		als_data[3] = ALS_HIGH_BYTE(adc_base + adc_threshold);
	}

	rc = regmap_bulk_write(di->regmap, AP3426_REG_ALS_LOW_THRES_0,
			als_data, 4);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_ALS_LOW_THRES_0, rc);
		return rc;
	}

	AP3426_DEBUG("%s,line %d:als threshold: 0x%x 0x%x 0x%x 0x%x\n",__func__,__LINE__,
			als_data[0], als_data[1], als_data[2],
			als_data[3]);

	/* Enable als again. */
	if (di->als_gain != gain_index) {
		rc = regmap_write(di->regmap, AP3426_REG_CONFIG, config | 0x01);
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			return rc;
		}

		di->als_gain = i;
	}

	return 0;
}

/* Read raw data, convert it to human readable values, report it and
 * reconfigure the sensor.
 */
static int ap3426_get_irdata(struct ap3426_data *di)
{
	int rc = 0;
	u8 ir_data[2];
	rc = regmap_bulk_read(di->regmap, AP3426_REG_IR_DATA_LOW,
					&ir_data, 2);
	if (rc) {
		AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_ALS_DATA_LOW, rc);
		goto exit;
	}
	rc = ir_data[0] | (ir_data[1] << 8);
exit:
	return rc;
}

static int ap3426_process_data(struct ap3426_data *di, int als_ps)
{
	ktime_t timestamp;
	int rc = 0;
	int ir_data = 0;
	unsigned int tmp;
	unsigned int gain;
	u8 als_data[2];
	int lux;
	u8 ps_data[4];
	int distance;

	timestamp = ktime_get();
	if (als_ps) { /* process als value */
		/* Read data */
		rc = regmap_bulk_read(di->regmap, AP3426_REG_ALS_DATA_LOW,
				als_data, 2);
		if (rc) {
			AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_ALS_DATA_LOW, rc);
			goto exit;
		}
		ir_data = ap3426_get_irdata(di);
		gain = gain_table[di->als_gain];
		/* lower bit */
		lux = (als_data[0] * di->als_cal * gain / 10) >> 16;
		/* higher bit */
		lux += (als_data[1] * di->als_cal * gain / 10) >> 8;
		tmp = als_data[0] | (als_data[1] << 8);
		if(ir_data < DIST_DIFF_LIGHT)
		{
			lux = lux * ALS_CW_VALUE*cali/100*ALS_CW_ANGLE;
		}
		else
		{
			lux = lux * ALS_AD_VALUE;
		}
		if( als_polling_count < 5 )
        {
                if(lux == AP3426_LUX_MAX)
                {
                        lux = lux - als_polling_count%2;
                }
                else
                {
                        lux = lux + als_polling_count%2;
                }
                als_polling_count++;
        }
		if (lux != di->last_als && ((tmp != ALS_DATA_MASK) ||
					((tmp == ALS_DATA_MASK) &&
					 (di->als_gain == 0)))) {
			input_report_abs(di->input_light, ABS_MISC, lux);
			input_sync(di->input_light);
		}

		di->last_als = lux;

		rc = ap3426_als_update_setting(di, tmp);
		if (rc) {
			AP3426_ERR("%s,line %d:update setting failed\n",__func__,__LINE__);
			goto exit;
		}
	} else { /* process ps value*/
		rc = regmap_bulk_read(di->regmap, AP3426_REG_PS_DATA_LOW,
				ps_data, 2);
		if (rc) {
			AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_PS_DATA_LOW, rc);
			goto exit;
		}

		AP3426_DEBUG("%s,line %d:ps data: 0x%x 0x%x\n",__func__,__LINE__,
				ps_data[0], ps_data[1]);
		tmp = ps_data[0] | (ps_data[1] << 8);
		if(tmp > ps_distance_final_table[0] )
		{
			distance = 0;
			ps_data[0] =
			PS_LOW_BYTE(ps_distance_final_table[1]);
			ps_data[1] =
			PS_HIGH_BYTE(ps_distance_final_table[1]);
			ps_data[2] = PS_LOW_BYTE(PS_DATA_MASK);
			ps_data[3] = PS_HIGH_BYTE(PS_DATA_MASK);
			rc = regmap_bulk_write(di->regmap, AP3426_REG_PS_LOW_THRES_0,
					ps_data, 4);
		}
		else if(tmp < ps_distance_final_table[1])
		{
			distance = 1;
			ps_data[0] = PS_LOW_BYTE(0);
			ps_data[1] = PS_LOW_BYTE(0);
			ps_data[2] = PS_LOW_BYTE(ps_distance_final_table[0]);
			ps_data[3] = PS_HIGH_BYTE(ps_distance_final_table[0]);
			rc = regmap_bulk_write(di->regmap, AP3426_REG_PS_LOW_THRES_0,
					ps_data, 4);
		}
		else
		{
			distance = 1;
		}
		AP3426_DEBUG("%s,line %d:reprt work ps_data: tmp =%d value[0]=%d  value[1]=%d \n",__func__,__LINE__,tmp, ps_distance_final_table[0],ps_distance_final_table[1]);
		input_report_abs(di->input_proximity, ABS_DISTANCE,
			distance);
		input_sync(di->input_proximity);
	}

exit:
	return rc;
}

static irqreturn_t ap3426_irq_handler(int irq, void *data)
{
	struct ap3426_data *di = data;
	bool rc;

	rc = queue_work(di->workqueue, &di->report_work);
	/* wake up event should hold a wake lock until reported */
	if (rc && (atomic_inc_return(&di->wake_count) == 1))
		pm_stay_awake(&di->i2c->dev);

	AP3426_DEBUG("%s,line %d:\n",__func__,__LINE__);
	return IRQ_HANDLED;
}

static void ap3426_report_work(struct work_struct *work)
{
	struct ap3426_data *di = container_of(work, struct ap3426_data,
			report_work);
	int rc;
	unsigned int status;

	mutex_lock(&di->ops_lock);

	/* avoid fake interrupt */
	if (!di->power_enabled) {
		AP3426_DEBUG("%s,line %d:fake interrupt triggered\n",__func__,__LINE__);
		goto exit;
	}

	rc = regmap_read(di->regmap, AP3426_REG_INT_FLAG, &status);
	if (rc) {
		AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_INT_FLAG, rc);
		status |= AP3426_PS_INT_MASK;
		goto exit;
	}

	AP3426_DEBUG("%s,line %d:interrupt issued status=0x%x.\n",__func__,__LINE__, status);

	/* als interrupt issueed */
	if ((status & AP3426_ALS_INT_MASK) && (di->als_enabled)) {
		rc = ap3426_process_data(di, 1);
		if (rc)
			goto exit;
		AP3426_DEBUG("%s,line %d:process als data done!\n",__func__,__LINE__);
	}

	if ((status & AP3426_PS_INT_MASK) && (di->ps_enabled)) {
		rc = ap3426_process_data(di, 0);
		if (rc)
			goto exit;
		AP3426_DEBUG("%s,line %d:process ps data done!\n",__func__,__LINE__);
		pm_wakeup_event(&di->input_proximity->dev, 200);
	}

exit:
	if (atomic_dec_and_test(&di->wake_count)) {
		pm_relax(&di->i2c->dev);
		AP3426_DEBUG("%s,line %d:wake lock released\n",__func__,__LINE__);
	}

	/* clear interrupt */
	if (di->power_enabled) {
		if (regmap_write(di->regmap, AP3426_REG_INT_FLAG, 0x0))
			AP3426_ERR("%s,line %d:clear interrupt failed\n",__func__,__LINE__);
	}

	mutex_unlock(&di->ops_lock);
}
static int ap3426_dial_ps_fuction(struct ap3426_data *di)
{
	int rc = 0;
	int value;
	int i=0;
	int tmp=0;
	int pxvalue=0;
	int read_count = 0;
	int sum_value = 0;
	u8 ps_data[4];
	rc = regmap_write(di->regmap, AP3426_REG_CONFIG, 0x02);
	if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			goto exit;
	}
	for(i=0;i<4;i++)
	{
		msleep(ap3426_calc_conversion_time(di, di->als_enabled, 1));
		rc = regmap_bulk_read(di->regmap, AP3426_REG_PS_DATA_LOW,
				ps_data, 2);
		if (rc) {
			AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_PS_DATA_LOW, rc);
			goto exit;
		}

		AP3426_DEBUG("%s,line %d:ps data: 0x%x 0x%x\n",__func__,__LINE__,
				ps_data[0], ps_data[1]);

		tmp = ps_data[0] | (ps_data[1] << 8);
		if(tmp < 0)
		{
			AP3426_ERR("%s %d:i2c read ps data fail. \n",__func__,__LINE__);
			goto exit;
		}
		sum_value += tmp;
		read_count++;
	}
	rc = regmap_write(di->regmap, AP3426_REG_CONFIG, 0x0);
	if(read_count > 0)
	{
		value=sum_value/read_count;
		AP3426_INFO("%s %d:value=%d sum_value=%d read_count=%d. \n",__func__,__LINE__,
			value,sum_value,read_count);
	}
	else
	{
		value=200;
	}
	if(value == 0)
	{
		value = 200;
	}
	pxvalue=value-di->crosstalk;
	AP3426_INFO("%s %d:px_value=%d ps_data->crosstalk=%d \n",__func__,__LINE__,
			pxvalue,di->crosstalk);
	if((pxvalue < 300)&&(pxvalue > 0))
	{
		ps_distance_final_table[0]=value+ps_distance_table[0];
		ps_distance_final_table[1] =value+ps_distance_table[1];
		flag =1;
		di->prevalue=value;
	}
	else if(pxvalue <= 0)
	{
		ps_distance_final_table[0]=di->crosstalk+ps_distance_table[0];
		ps_distance_final_table[1] =di->crosstalk+ps_distance_table[1];
		flag =1;
		di->prevalue=di->crosstalk;
	}
	else
	{
		if(flag == 1)
		{
			ps_distance_final_table[0]=di->prevalue+ps_distance_table[0];
			ps_distance_final_table[1] =di->prevalue+ps_distance_table[1];
		}
		else
		{
			ps_distance_final_table[0]=di->crosstalk+ps_distance_table[0];
			ps_distance_final_table[1] =di->crosstalk+ps_distance_table[1];
		}
	}
	ps_data[0] =
		PS_LOW_BYTE(ps_distance_final_table[1]);
	ps_data[1] =
		PS_HIGH_BYTE(ps_distance_final_table[1]);
	ps_data[2] = PS_LOW_BYTE(ps_distance_final_table[0]);
	ps_data[3] = PS_HIGH_BYTE(ps_distance_final_table[0]);
	AP3426_ERR("%s,line %d:ps threshold: 0x%x 0x%x 0x%x 0x%x\n",__func__,__LINE__,
			ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
	rc = regmap_bulk_write(di->regmap,
			AP3426_REG_PS_LOW_THRES_0, ps_data, 4);
	if (rc) {
		AP3426_ERR("%s,line %d:set up threshold failed\n",__func__,__LINE__);
		goto exit;
	}
exit:
	return rc;

}
static ssize_t ap3426_show_ps_calibration(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *di = input_get_drvdata(input);
	int tmp;
	int i;
	int rc;
	int read_count = 0;
	int sum_value = 0;
	u8 ps_data[4];
	/* < CPM2016020300039 yanfei 20160222 begin */
	int ps_th_limit = 616;
	/* CPM2016020300039 yanfei 20160222 end > */

	for(i=0;i<20;i++){
		rc = regmap_bulk_read(di->regmap, AP3426_REG_PS_DATA_LOW,
				ps_data, 2);
		if (rc) {
			AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_PS_DATA_LOW, rc);
			return rc;
		}

		AP3426_DEBUG("%s,line %d:ps data: 0x%x 0x%x\n",__func__,__LINE__,
				ps_data[0], ps_data[1]);

		tmp = ps_data[0] | (ps_data[1] << 8);
		 if(tmp < 0)
		 {
		 	AP3426_ERR("%s %d:i2c read ps data fail. \n",__func__,__LINE__);
			return tmp;
		 }
		else if(tmp < ps_th_limit){
		sum_value += tmp;
		read_count ++;
		}
		AP3426_ERR("%s %d:  tmp=%d  read_count=%d \n",__func__,__LINE__,tmp,read_count);
	}

	if(read_count == 0){
		di->crosstalk =-1;
		return sprintf(buf, "%d\n",  di->crosstalk);
	}else{
		 di->crosstalk = sum_value/read_count;
		 if(di->crosstalk == 0)
		 {
			di->crosstalk = 5;
		 }
		AP3426_ERR("%s %d:  data->crosstalk=%d,sum_value=%d,read_count=%d \n",__func__,__LINE__,
			di->crosstalk,sum_value,read_count);
		ps_distance_final_table[0] = ps_distance_table[0]+di->crosstalk;
		ps_distance_final_table[1] = ps_distance_table[1]+di->crosstalk;
		ps_data[0] =
			PS_LOW_BYTE(ps_distance_final_table[1]);
		ps_data[1] =
			PS_HIGH_BYTE(ps_distance_final_table[1]);
		ps_data[2] = PS_LOW_BYTE(ps_distance_final_table[0]);
		ps_data[3] = PS_HIGH_BYTE(ps_distance_final_table[0]);
		AP3426_ERR("%s,line %d:ps threshold: 0x%x 0x%x 0x%x 0x%x\n",__func__,__LINE__,
			ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
		rc = regmap_bulk_write(di->regmap,
				AP3426_REG_PS_LOW_THRES_0, ps_data, 4);
		 if(rc  < 0)
		 {

			AP3426_ERR("%s %d: ap3426_set_phthres \n",__func__,__LINE__);
			return sprintf(buf, "%d\n", di->crosstalk);
		 }

	}
	AP3426_ERR("%s %d:data->ps_thres_high=%d data->ps_thres_low=%d \n",__func__,__LINE__,
		ps_distance_final_table[0],ps_distance_final_table[1]);
	return sprintf(buf, "%d\n", di->crosstalk);

}
static ssize_t ap3426_store_ps_crosstalk(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *di = input_get_drvdata(input);
	int ret;
	u8 ps_data[4];
	unsigned long prox_data;

	if (strict_strtoul(buf, 10, &prox_data) < 0)
		return -EINVAL;
	di->crosstalk= prox_data;
	AP3426_ERR("%s %d prox_data=%d \n",__func__,__LINE__,di->crosstalk);

	if(di->crosstalk==0)
	{
		AP3426_ERR("%s %d: ps_thres_high=%d ps_thres_low=%d  ps_high=%d ps_low=%d\n",__func__,__LINE__,
			ps_distance_final_table[0],ps_distance_final_table[1],ps_distance_table[0],ps_distance_table[1]);
		goto exit;
	}
	ps_distance_final_table[0] = ps_distance_table[0]+di->crosstalk;
	ps_distance_final_table[1] = ps_distance_table[1]+di->crosstalk;
	ps_data[0] =
		PS_LOW_BYTE(ps_distance_final_table[1]);
	ps_data[1] =
		PS_HIGH_BYTE(ps_distance_final_table[1]);
	ps_data[2] = PS_LOW_BYTE(ps_distance_final_table[0]);
	ps_data[3] = PS_HIGH_BYTE(ps_distance_final_table[0]);
	AP3426_ERR("%s,line %d:ps threshold: 0x%x 0x%x 0x%x 0x%x\n",__func__,__LINE__,
			ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
	ret = regmap_bulk_write(di->regmap,
			AP3426_REG_PS_LOW_THRES_0, ps_data, 4);
	 if(ret  < 0)
	 {

		AP3426_ERR("%s %d: ap3426_set_phthres \n",__func__,__LINE__);
		goto exit;
	 }
	AP3426_ERR("%s %d: ps_thres_high=%d ps_thres_low=%d  ps_high=%d ps_low=%d\n",__func__,__LINE__,
		ps_distance_final_table[0],ps_distance_final_table[1],ps_distance_table[0],ps_distance_table[1]);

exit:

	return count;
}
static ssize_t ap3426_show_als_cal(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *di = input_get_drvdata(input);
	int  lux;
	int err;
	unsigned int gain;
	u8 als_data[2];
	err = regmap_bulk_read(di->regmap, AP3426_REG_ALS_DATA_LOW,
			als_data, 2);
	if (err) {
		AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_ALS_DATA_LOW, err);
		return err;
	}
	gain = gain_table[di->als_gain];
	/* lower bit */
	lux = (als_data[0] * di->als_cal * gain / 10) >> 16;
	/* higher bit */
	lux += (als_data[1] * di->als_cal * gain / 10) >> 8;
	/*< CQ_hw00003902 yanfei 20151225 begin*/
	AP3426_ERR("%s %d:lux=%d  \n",__func__,__LINE__,lux);
	if((lux>ALS_THRES_LOW )&&(lux< ALS_THRES_HIGH))
	{
		cali = ALS_LIGHTBAR_VALUE * 100 / (lux*ALS_CW_VALUE);
	}
	/*  CQ_hw00003902 yanfei 20151225 end >*/
	else
	{
		err=-1;
		AP3426_ERR("%s %d:lux=%d cali=%d \n",__func__,__LINE__,lux,cali);
		return sprintf(buf, "%d\n", err);
	}
	/* CPM2016011900083 yanfei 20160127 end*/
	AP3426_DEBUG("%s %d:lux=%d cali=%d \n",__func__,__LINE__,lux,cali);
	return sprintf(buf, "%d\n", cali);
}
static ssize_t ap3426_store_als_cali(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long als_data;

	if (strict_strtoul(buf, 10, &als_data) < 0)
		return -EINVAL;
	cali= als_data;
	AP3426_ERR("%s %d:cali=%d \n",__func__,__LINE__,cali);
	return count;
}
/* CPM2016032900051 yanfei 20160405 end > */
/*< CQ_hw00004042 yanfei 20160109 begin*/
/*< CQ_hw00004049 yanfei 20160115 begin*/
static DEVICE_ATTR(als_calibration, 0664,NULL,ap3426_store_als_cali);
static DEVICE_ATTR(als_cal, S_IWUSR |S_IRUGO,ap3426_show_als_cal,NULL);
static DEVICE_ATTR(ps_crosstalk, 0664,NULL,ap3426_store_ps_crosstalk);
/* CQ_hw00004042 yanfei 20160109 end >*/
/* CQ_hw00004049 yanfei 20160115 end >*/
static int ap3426_enable_ps(struct ap3426_data *di, int enable)
{
	unsigned int config;
	int rc = 0;

	rc = regmap_read(di->regmap, AP3426_REG_CONFIG, &config);
	if (rc) {
		AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
		goto exit;
	}

	/* avoid operate sensor in different executing context */
	if (enable) {
		/* Enable ps sensor */
		rc=ap3426_dial_ps_fuction(di);
		if(rc)
		{
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
			goto exit;
		}
		rc = regmap_write(di->regmap, AP3426_REG_CONFIG, config | 0x02);
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			goto exit;
		}

		/* wait the data ready */
		msleep(ap3426_calc_conversion_time(di, di->als_enabled, 1));

		/* Clear last value and report it even not change. */
		di->last_ps = -1;
		rc = ap3426_process_data(di, 0);
		if (rc) {
			AP3426_ERR("%s,line %d:process ps data failed\n",__func__,__LINE__);
			goto exit;
		}

		/* clear interrupt */
		rc = regmap_write(di->regmap, AP3426_REG_INT_FLAG, 0x0);
		if (rc) {
			AP3426_ERR("%s,line %d:clear interrupt failed\n",__func__,__LINE__);
			goto exit;
		}

		di->ps_enabled = true;
	} else {
		/* disable the ps_sensor */
		rc = regmap_write(di->regmap, AP3426_REG_CONFIG,
				config & (~0x2));
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			goto exit;
		}
		di->ps_enabled = false;
	}
exit:
	return rc;
}

static int ap3426_enable_als(struct ap3426_data *di, int enable)
{
	unsigned int config;
	int rc = 0;

	/* Read the system config register */
	rc = regmap_read(di->regmap, AP3426_REG_CONFIG, &config);
	if (rc) {
		AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
		goto exit;
	}

	if (enable) {
		/* enable als_sensor */
		rc = regmap_write(di->regmap, AP3426_REG_CONFIG, config | 0x03);
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			goto exit;
		}

		/* wait data ready */
		msleep(ap3426_calc_conversion_time(di, 1, di->ps_enabled));

		/* Clear last value and report even not change. */
		di->last_als = -1;

		rc = ap3426_process_data(di, 1);
		if (rc) {
			AP3426_ERR("%s,line %d:process als data failed\n",__func__,__LINE__);
			goto exit;
		}

		/* clear interrupt */
		rc = regmap_write(di->regmap, AP3426_REG_INT_FLAG, 0x0);
		if (rc) {
			AP3426_ERR("%s,line %d:clear interrupt failed\n",__func__,__LINE__);
			goto exit;
		}

		di->als_enabled = 1;

	} else {
		/* disable the als_sensor */
		rc = regmap_write(di->regmap, AP3426_REG_CONFIG,
				config & (~0x1));
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
					AP3426_REG_CONFIG, rc);
			goto exit;
		}
		di->als_enabled = 0;
	}
exit:
	return rc;
}

/* Sync delay to hardware according configurations
 * Note one of the sensor may report faster than expected.
 */
static int ap3426_sync_delay(struct ap3426_data *di, int als_enabled,
		int ps_enabled, unsigned int als_delay, unsigned int ps_delay)
{
	unsigned int convert_msec;
	unsigned int delay;
	int rc;

	/* ignore delay synchonization while power not enabled */
	if (!di->power_enabled) {
		AP3426_DEBUG("%s,line %d:power is not enabled\n",__func__,__LINE__);
		return 0;
	}
	convert_msec = ap3426_calc_conversion_time(di, als_enabled, ps_enabled);

	if (als_enabled && ps_enabled)
		delay = min(als_delay, ps_delay);
	else if (als_enabled)
		delay = als_delay;
	else if (ps_enabled)
		delay = ps_delay;
	else
		return 0;

	if (delay < convert_msec)
		delay = 0;
	else
		delay -= convert_msec;

	/* Insert delay_msec into wait slots. The maximum is 255 * 5ms */
	AP3426_DEBUG("%s,line %d:wait time: %lu\n",__func__,__LINE__, min(delay / 5UL, 255UL));
	rc = regmap_write(di->regmap, AP3426_REG_WAIT_TIME,
			min(delay / 5UL, 255UL));
	if (rc) {
		AP3426_ERR("%s,line %d:write %d failed\n",__func__,__LINE__,
				AP3426_REG_WAIT_TIME);
		return rc;
	}

	return 0;
}

static void ap3426_als_enable_work(struct work_struct *work)
{
	struct ap3426_data *di = container_of(work, struct ap3426_data,
			als_enable_work);

	mutex_lock(&di->ops_lock);
	if (!di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}

		msleep(AP3426_BOOT_TIME_MS);
		di->power_enabled = true;
		als_polling_count=0;
		if (ap3426_init_device(di)) {
			AP3426_ERR("%s,line %d:init device failed\n",__func__,__LINE__);
			goto exit_power_off;
		}
	}

	/* Old HAL: Sync to last delay. New HAL: Sync to current delay */
	if (ap3426_sync_delay(di, 1, di->ps_enabled, di->als_delay,
				di->ps_delay))
		goto exit_power_off;

	if (ap3426_enable_als(di, 1)) {
		AP3426_ERR("%s,line %d:enable als failed\n",__func__,__LINE__);
		goto exit_power_off;
	}

exit_power_off:
	if ((!di->als_enabled) && (!di->ps_enabled) &&
			di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}
		di->power_enabled = false;
	}

exit:
	mutex_unlock(&di->ops_lock);
	return;
}


static void ap3426_als_disable_work(struct work_struct *work)
{
	struct ap3426_data *di = container_of(work, struct ap3426_data,
			als_disable_work);

	mutex_lock(&di->ops_lock);

	if (ap3426_enable_als(di, 0)) {
		AP3426_ERR("%s,line %d:disable als failed\n",__func__,__LINE__);
		goto exit;
	}

	if (ap3426_sync_delay(di, 0, di->ps_enabled, di->als_delay,
			di->ps_delay))
		goto exit;

	if ((!di->als_enabled) && (!di->ps_enabled) && di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}

		di->power_enabled = false;
	}

exit:
	mutex_unlock(&di->ops_lock);
}

static void ap3426_ps_enable_work(struct work_struct *work)
{
	struct ap3426_data *di = container_of(work, struct ap3426_data,
			ps_enable_work);

	mutex_lock(&di->ops_lock);
	if (!di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}

		msleep(AP3426_BOOT_TIME_MS);
		di->power_enabled = true;

		if (ap3426_init_device(di)) {
			AP3426_ERR("%s,line %d:init device failed\n",__func__,__LINE__);
			goto exit_power_off;
		}
	}

	/* Old HAL: Sync to last delay. New HAL: Sync to current delay */
	if (ap3426_sync_delay(di, di->als_enabled, 1, di->als_delay,
				di->ps_delay))
		goto exit_power_off;

	if (ap3426_enable_ps(di, 1)) {
		AP3426_ERR("%s,line %d:enable ps failed\n",__func__,__LINE__);
		goto exit_power_off;
	}

exit_power_off:
	if ((!di->als_enabled) && (!di->ps_enabled) &&
			di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}
		di->power_enabled = false;
	}

exit:
	mutex_unlock(&di->ops_lock);
	return;
}

void ap3426_powerkey_screen_handler(struct work_struct *work)
{
        struct ap3426_data *ps_data = container_of((struct delayed_work *)work, struct ap3426_data, powerkey_work);
        if(power_key_ps)
        {
                AP3426_ERR("%s : power_key_ps (%d) press\n",__func__, power_key_ps);
                power_key_ps=false;
                input_report_abs(ps_data->input_proximity, ABS_DISTANCE, 1);
                input_sync(ps_data->input_proximity);
        }
        schedule_delayed_work(&ps_data->powerkey_work, msecs_to_jiffies(500));
}
static void ap3426_ps_disable_work(struct work_struct *work)
{
	struct ap3426_data *di = container_of(work, struct ap3426_data,
			ps_disable_work);

	mutex_lock(&di->ops_lock);

	if (ap3426_enable_ps(di, 0)) {
		AP3426_ERR("%s,line %d:disable ps failed\n",__func__,__LINE__);
		goto exit;
	}

	if (ap3426_sync_delay(di, di->als_enabled, 0, di->als_delay,
			di->ps_delay))
		goto exit;

	if ((!di->als_enabled) && (!di->ps_enabled) && di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}

		di->power_enabled = false;
	}
exit:
	mutex_unlock(&di->ops_lock);
}

static struct regmap_config ap3426_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int ap3426_cdev_enable_als(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	int res = 0;
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, als_cdev);

	mutex_lock(&di->ops_lock);

	if (enable)
		queue_work(di->workqueue, &di->als_enable_work);
	else
		queue_work(di->workqueue, &di->als_disable_work);

	mutex_unlock(&di->ops_lock);

	return res;
}

static int ap3426_cdev_enable_ps(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, ps_cdev);

	mutex_lock(&di->ops_lock);

	if (enable)
	{
		queue_work(di->workqueue, &di->ps_enable_work);
		power_key_ps = false;
		schedule_delayed_work(&di->powerkey_work, msecs_to_jiffies(100));
	}
	else
	{
		queue_work(di->workqueue, &di->ps_disable_work);
		cancel_delayed_work(&di->powerkey_work);
	}
	mutex_unlock(&di->ops_lock);

	return 0;
}

static int ap3426_cdev_set_als_delay(struct sensors_classdev *sensors_cdev,
		unsigned int delay_msec)
{
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, als_cdev);

	mutex_lock(&di->ops_lock);

	di->als_delay = delay_msec;
	ap3426_sync_delay(di, di->als_enabled, di->ps_enabled,
			di->als_delay, di->ps_delay);

	mutex_unlock(&di->ops_lock);

	return 0;
}

static int ap3426_cdev_set_ps_delay(struct sensors_classdev *sensors_cdev,
		unsigned int delay_msec)
{
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, ps_cdev);

	mutex_lock(&di->ops_lock);

	di->ps_delay = delay_msec;
	ap3426_sync_delay(di, di->als_enabled, di->ps_enabled,
			di->als_delay, di->ps_delay);

	mutex_unlock(&di->ops_lock);

	return 0;
}

static int ap3426_cdev_ps_flush(struct sensors_classdev *sensors_cdev)
{
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, ps_cdev);

	input_event(di->input_proximity, EV_SYN, SYN_CONFIG,
			di->flush_count++);
	input_sync(di->input_proximity);

	return 0;
}

static int ap3426_cdev_als_flush(struct sensors_classdev *sensors_cdev)
{
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, als_cdev);

	input_event(di->input_light, EV_SYN, SYN_CONFIG, di->flush_count++);
	input_sync(di->input_light);

	return 0;
}

/* This function should be called when sensor is disabled */
static int ap3426_cdev_ps_calibrate(struct sensors_classdev *sensors_cdev,
		int axis, int apply_now)
{
	int rc;
	int power;
	unsigned int config;
	unsigned int interrupt;
	u16 min = PS_DATA_MASK;
	u8 ps_data[2];
	int count = AP3426_CALIBRATE_SAMPLES;
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, ps_cdev);


	if (axis != AXIS_BIAS)
		return 0;

	mutex_lock(&di->ops_lock);

	/* Ensure only be called when sensors in standy mode */
	if (di->als_enabled || di->ps_enabled) {
		rc = -EPERM;
		goto exit;
	}

	power = di->power_enabled;
	if (!power) {
		rc = sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true);
		if (rc) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}

		msleep(AP3426_BOOT_TIME_MS);

		rc = ap3426_init_device(di);
		if (rc) {
			AP3426_ERR("%s,line %d:init ap3426 failed\n",__func__,__LINE__);
			goto exit;
		}
	}

	rc = regmap_read(di->regmap, AP3426_REG_INT_CTL, &interrupt);
	if (rc) {
		AP3426_ERR("%s,line %d:read interrupt configuration failed\n",__func__,__LINE__);
		goto exit_power_off;
	}

	/* disable interrupt */
	rc = regmap_write(di->regmap, AP3426_REG_INT_CTL, 0x0);
	if (rc) {
		AP3426_ERR("%s,line %d:disable interrupt failed\n",__func__,__LINE__);
		goto exit_power_off;
	}

	rc = regmap_read(di->regmap, AP3426_REG_CONFIG, &config);
	if (rc) {
		AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
		goto exit_enable_interrupt;
	}

	/* clear wait time */
	rc = regmap_write(di->regmap, AP3426_REG_WAIT_TIME, 0x0);
	if (rc) {
		AP3426_ERR("%s,line %d:clear wait time failed\n",__func__,__LINE__);
		goto exit_enable_interrupt;
	}

	/* clear offset */
	ps_data[0] = 0;
	ps_data[1] = 0;
	rc = regmap_bulk_write(di->regmap, AP3426_REG_PS_CAL_L,
			ps_data, 2);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_PS_CAL_L, rc);
		goto exit_enable_interrupt;
	}

	/* enable ps sensor */
	rc = regmap_write(di->regmap, AP3426_REG_CONFIG, config | 0x02);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
		goto exit_disable_ps;
	}

	while (--count) {
		/*
		 * This function is expected to be executed only 1 time in
		 * factory and never be executed again during the device's
		 * life time. It's fine to busy wait for data ready.
		 */
		usleep_range(ap3426_calc_conversion_time(di, 0, 1) * 1000,
			(ap3426_calc_conversion_time(di, 0, 1) + 1) * 1000);
		rc = regmap_bulk_read(di->regmap, AP3426_REG_PS_DATA_LOW,
				ps_data, 2);
		if (rc) {
			AP3426_ERR("%s,line %d:read PS data failed\n",__func__,__LINE__);
			break;
		}
		if (min > ((ps_data[1] << 8) | ps_data[0]))
			min = (ps_data[1] << 8) | ps_data[0];
	}

	if (!count) {
		if (min > (PS_DATA_MASK >> 1)) {
			AP3426_ERR("%s,line %d:ps data out of range, check if shield\n",__func__,__LINE__);
			rc = -EINVAL;
			goto exit_disable_ps;
		}

		if (apply_now) {
			ps_data[0] = PS_LOW_BYTE(min);
			ps_data[1] = PS_HIGH_BYTE(min);
			rc = regmap_bulk_write(di->regmap, AP3426_REG_PS_CAL_L,
					ps_data, 2);
			if (rc) {
				AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
						AP3426_REG_PS_CAL_L, rc);
				goto exit_disable_ps;
			}
			di->bias = min;
		}

		snprintf(di->calibrate_buf, sizeof(di->calibrate_buf), "0,0,%d",
				min);
		AP3426_DEBUG("%s,line %d:result: %s\n",__func__,__LINE__, di->calibrate_buf);
	} else {
		AP3426_ERR("%s,line %d:calibration failed\n",__func__,__LINE__);
		rc = -EINVAL;
	}

exit_disable_ps:
	rc = regmap_write(di->regmap, AP3426_REG_CONFIG, config & (~0x02));
	if (rc) {
		AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_CONFIG, rc);
		goto exit_enable_interrupt;
	}

exit_enable_interrupt:
	if (regmap_write(di->regmap, AP3426_REG_INT_CTL, interrupt)) {
		AP3426_ERR("%s,line %d:enable interrupt failed\n",__func__,__LINE__);
		goto exit_power_off;
	}

exit_power_off:
	if (!power) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power off sensor failed.\n",__func__,__LINE__);
			goto exit;
		}
	}
exit:
	mutex_unlock(&di->ops_lock);
	return rc;
}

static int ap3426_cdev_ps_write_cal(struct sensors_classdev *sensors_cdev,
		struct cal_result_t *cal_result)
{
	int power;
	u8 ps_data[2];
	int rc;
	struct ap3426_data *di = container_of(sensors_cdev,
			struct ap3426_data, ps_cdev);

	mutex_lock(&di->ops_lock);
	power = di->power_enabled;
	if (!power) {
		rc = sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), true);
		if (rc) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}
	}

	di->bias = cal_result->bias;
	ps_data[0] = PS_LOW_BYTE(cal_result->bias);
	ps_data[1] = PS_HIGH_BYTE(cal_result->bias);
	rc = regmap_bulk_write(di->regmap, AP3426_REG_PS_CAL_L,
			ps_data, 2);
	if (rc) {
		AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
				AP3426_REG_PS_CAL_L, rc);
		goto exit_power_off;
	}

	snprintf(di->calibrate_buf, 10, "0,0,%d", di->bias);

exit_power_off:
	if (!power) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power off sensor failed.\n",__func__,__LINE__);
			goto exit;
		}
	}
exit:

	mutex_unlock(&di->ops_lock);
	return 0;
};

static ssize_t ap3426_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ap3426_data *di = dev_get_drvdata(dev);
	unsigned int val;
	int rc;
	ssize_t count = 0;
	int i;

	if (di->reg_addr == AP3426_REG_MAGIC) {
		for (i = 0; i < AP3426_REG_COUNT; i++) {
			rc = regmap_read(di->regmap, AP3426_REG_CONFIG + i,
					&val);
			if (rc) {
				AP3426_ERR("%s,line %d:read %d failed\n",__func__,__LINE__,
						AP3426_REG_CONFIG + i);
				break;
			}
			count += snprintf(&buf[count], PAGE_SIZE,
					"0x%x: 0x%x\n", AP3426_REG_CONFIG + i,
					val);
		}
	} else {
		rc = regmap_read(di->regmap, di->reg_addr, &val);
		if (rc) {
			AP3426_ERR("%s,line %d:read %d failed\n",__func__,__LINE__,
					di->reg_addr);
			return rc;
		}
		count += snprintf(&buf[count], PAGE_SIZE, "0x%x:0x%x\n",
				di->reg_addr, val);
	}

	return count;
}

static ssize_t ap3426_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct ap3426_data *di = dev_get_drvdata(dev);
	unsigned int reg;
	unsigned int val;
	unsigned int cmd;
	int rc;

	if (sscanf(buf, "%u %u %u\n", &cmd, &reg, &val) < 2) {
		AP3426_ERR("%s,line %d:argument error\n",__func__,__LINE__);
		return -EINVAL;
	}

	if (cmd == CMD_WRITE) {
		rc = regmap_write(di->regmap, reg, val);
		if (rc) {
			AP3426_ERR("%s,line %d:write %d failed\n", __func__,__LINE__,reg);
			return rc;
		}
	} else if (cmd == CMD_READ) {
		di->reg_addr = reg;
		AP3426_DEBUG("%s,line %d:register address set to 0x%x\n",__func__,__LINE__, reg);
	}

	return size;
}
static DEVICE_ATTR(ps_cal, 0664, ap3426_show_ps_calibration, NULL);

static DEVICE_ATTR(register, S_IWUSR | S_IRUGO,
		ap3426_register_show,
		ap3426_register_store);

static struct attribute *ap3426_attr[] = {
	&dev_attr_register.attr,
	&dev_attr_ps_cal.attr,
	&dev_attr_ps_crosstalk.attr,
	&dev_attr_als_cal.attr,
	&dev_attr_als_calibration.attr,
	NULL
};

static const struct attribute_group ap3426_attr_group = {
	.attrs = ap3426_attr,
};

static int ap3426_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct ap3426_data *di;
	int res = 0;

	AP3426_DEBUG("%s,line %d:probing ap3426...\n",__func__,__LINE__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		AP3426_ERR("%s,line %d:ap3426 i2c check failed.\n",__func__,__LINE__);
		return -ENODEV;
	}

	di = devm_kzalloc(&client->dev, sizeof(struct ap3426_data),
			GFP_KERNEL);
	if (!di) {
		AP3426_ERR("%s,line %d:memory allocation failed,\n",__func__,__LINE__);
		return -ENOMEM;
	}

	di->i2c = client;

	if (client->dev.of_node) {
		res = ap3426_parse_dt(&client->dev, di);
		if (res) {
			AP3426_ERR("%s,line %d:unable to parse device tree.(%d)\n",__func__,__LINE__, res);
			goto out;
		}
	} else {
		AP3426_ERR("%s,line %d:device tree not found.\n",__func__,__LINE__);
		res = -ENODEV;
		goto out;
	}

	dev_set_drvdata(&client->dev, di);
	mutex_init(&di->ops_lock);

	di->regmap = devm_regmap_init_i2c(client, &ap3426_regmap_config);
	if (IS_ERR(di->regmap)) {
		AP3426_ERR("%s,line %d:init regmap failed.(%ld)\n",__func__,__LINE__,
				PTR_ERR(di->regmap));
		res = PTR_ERR(di->regmap);
		goto out;
	}

	res = sensor_power_init(&client->dev, power_config,
			ARRAY_SIZE(power_config));
	if (res) {
		AP3426_ERR("%s,line %d:init power failed.\n",__func__,__LINE__);
		goto out;
	}

	res = sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), true);
	if (res) {
		AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
		goto err_power_config;
	}

	res = sensor_pinctrl_init(&client->dev, &pin_config);
	if (res) {
		AP3426_ERR("%s,line %d:init pinctrl failed.\n",__func__,__LINE__);
		goto err_pinctrl_init;
	}

	/* wait the device to boot up */
	msleep(AP3426_BOOT_TIME_MS);

	res = ap3426_check_device(di);
	if (res) {
		AP3426_ERR("%s,line %d:check device failed.\n",__func__,__LINE__);
		goto err_check_device;
	}

	res = ap3426_init_device(di);
	if (res) {
		AP3426_ERR("%s,line %d:check device failed.\n",__func__,__LINE__);
		goto err_init_device;
	}

	/* configure interrupt */
	if (gpio_is_valid(di->irq_gpio)) {
		res = gpio_request(di->irq_gpio, "ap3426_interrupt");
		if (res) {
			AP3426_ERR("%s,line %d:unable to request interrupt gpio %d\n",__func__,__LINE__,
				di->irq_gpio);
			goto err_request_gpio;
		}

		res = gpio_direction_input(di->irq_gpio);
		if (res) {
			AP3426_ERR("%s,line %d:unable to set direction for gpio %d\n",__func__,__LINE__,
				di->irq_gpio);
			goto err_set_direction;
		}

		di->irq = gpio_to_irq(di->irq_gpio);

		res = devm_request_irq(&client->dev, di->irq,
				ap3426_irq_handler,
				di->irq_flags | IRQF_ONESHOT,
				"ap3426", di);

		if (res) {
			AP3426_ERR("%s,line %d:request irq %d failed(%d),\n",__func__,__LINE__,
					di->irq, res);
			goto err_request_irq;
		}

		/* device wakeup initialization */
		device_init_wakeup(&client->dev, 1);

		di->workqueue = alloc_workqueue("ap3426_workqueue",
				WQ_NON_REENTRANT | WQ_FREEZABLE, 0);
		INIT_WORK(&di->report_work, ap3426_report_work);
		INIT_WORK(&di->als_enable_work, ap3426_als_enable_work);
		INIT_WORK(&di->als_disable_work, ap3426_als_disable_work);
		INIT_WORK(&di->ps_enable_work, ap3426_ps_enable_work);
		INIT_WORK(&di->ps_disable_work, ap3426_ps_disable_work);
		INIT_DELAYED_WORK(&di->powerkey_work, ap3426_powerkey_screen_handler);

	} else {
		res = -ENODEV;
		goto err_init_device;
	}

	res = sysfs_create_group(&client->dev.kobj, &ap3426_attr_group);
	if (res) {
		AP3426_ERR("%s,line %d:sysfs create group failed\n",__func__,__LINE__);
		goto err_create_group;
	}

	res = ap3426_init_input(di);
	if (res) {
		AP3426_ERR("%s,line %d:init input failed.\n",__func__,__LINE__);
		goto err_init_input;
	}

	/* input device should hold a 200ms wake lock */
	device_init_wakeup(&di->input_proximity->dev, 1);

	di->als_cdev = als_cdev;
	di->als_cdev.sensors_enable = ap3426_cdev_enable_als;
	di->als_cdev.sensors_poll_delay = ap3426_cdev_set_als_delay;
	di->als_cdev.sensors_flush = ap3426_cdev_als_flush;
	res = sensors_classdev_register(&client->dev, &di->als_cdev);
	if (res) {
		AP3426_ERR("%s,line %d:sensors class register failed.\n",__func__,__LINE__);
		goto err_register_als_cdev;
	}

	di->ps_cdev = ps_cdev;
	di->ps_cdev.sensors_enable = ap3426_cdev_enable_ps;
	di->ps_cdev.sensors_poll_delay = ap3426_cdev_set_ps_delay;
	di->ps_cdev.sensors_flush = ap3426_cdev_ps_flush;
	di->ps_cdev.sensors_calibrate = ap3426_cdev_ps_calibrate;
	di->ps_cdev.sensors_write_cal_params = ap3426_cdev_ps_write_cal;
	di->ps_cdev.params = di->calibrate_buf;
	res = sensors_classdev_register(&client->dev, &di->ps_cdev);
	if (res) {
		AP3426_ERR("%s,line %d:sensors class register failed.\n",__func__,__LINE__);
		goto err_register_ps_cdev;
	}

	sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), false);
	di->power_enabled = false;
	res = app_info_set("P-Sensor", "AP3426");
	res += app_info_set("L-Sensor", "AP3426");
	if (res < 0)/*failed to add app_info*/
	{
		AP3426_ERR("%s %d:failed to add app_info\n", __func__, __LINE__);
    }
	AP3426_INFO("%s,line %d:ap3426 successfully probed!\n",__func__,__LINE__);

	return 0;

err_register_ps_cdev:
	sensors_classdev_unregister(&di->als_cdev);
err_register_als_cdev:
	device_init_wakeup(&di->input_proximity->dev, 0);
err_init_input:
	sysfs_remove_group(&client->dev.kobj, &ap3426_attr_group);
err_create_group:
err_request_irq:
err_set_direction:
	gpio_free(di->irq_gpio);
err_request_gpio:
err_init_device:
	device_init_wakeup(&client->dev, 0);
err_check_device:
err_pinctrl_init:
	sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), false);
err_power_config:
	sensor_power_deinit(&client->dev, power_config,
			ARRAY_SIZE(power_config));
out:
	return res;
}

static int ap3426_remove(struct i2c_client *client)
{
	struct ap3426_data *di = dev_get_drvdata(&client->dev);

	sensors_classdev_unregister(&di->ps_cdev);
	sensors_classdev_unregister(&di->als_cdev);

	destroy_workqueue(di->workqueue);
	device_init_wakeup(&di->i2c->dev, 0);
	device_init_wakeup(&di->input_proximity->dev, 0);

	sensor_power_config(&client->dev, power_config,
			ARRAY_SIZE(power_config), false);
	sensor_power_deinit(&client->dev, power_config,
			ARRAY_SIZE(power_config));

	return 0;
}

static int ap3426_suspend(struct device *dev)
{
	int res = 0;
	struct ap3426_data *di = dev_get_drvdata(dev);
	u8 ps_data[4];
	unsigned int config;
	int idx = di->ps_wakeup_threshold;

	AP3426_DEBUG("%s,line %d:suspending ap3426...",__func__,__LINE__);

	mutex_lock(&di->ops_lock);

	/* proximity is enabled */
	if (di->ps_enabled) {
		/* disable als sensor to avoid wake up by als interrupt */
		if (di->als_enabled) {
			res = regmap_read(di->regmap, AP3426_REG_CONFIG,
					&config);
			if (res) {
				AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
						AP3426_REG_CONFIG, res);
				goto exit;
			}


			res = regmap_write(di->regmap, AP3426_REG_CONFIG,
					config & (~0x1));
			if (res) {
				AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
						AP3426_REG_CONFIG, res);
				goto exit;
			}

			ap3426_sync_delay(di, 0, 1, 0, di->ps_delay);
		}

		/* Don't power off sensor because proximity is a
		 * wake up sensor.
		 */
		if (device_may_wakeup(&di->i2c->dev)) {
			AP3426_DEBUG("%s,line %d:enable irq wake\n",__func__,__LINE__);
			enable_irq_wake(di->irq);
		}

		/* Setup threshold to avoid frequent wakeup */
		if (device_may_wakeup(&di->i2c->dev) &&
				(idx != AP3426_WAKEUP_ANY_CHANGE)) {
			AP3426_DEBUG("%s,line %d:last ps: %d\n",__func__,__LINE__, di->last_ps);
			if (di->last_ps > idx) {
				ps_data[0] = 0x0;
				ps_data[1] = 0x0;
				ps_data[2] =
					PS_LOW_BYTE(ps_distance_final_table[idx]);
				ps_data[3] =
					PS_HIGH_BYTE(ps_distance_final_table[idx]);
			} else {
				ps_data[0] =
					PS_LOW_BYTE(ps_distance_final_table[idx]);
				ps_data[1] =
					PS_HIGH_BYTE(ps_distance_final_table[idx]);
				ps_data[2] = PS_LOW_BYTE(PS_DATA_MASK);
				ps_data[3] = PS_HIGH_BYTE(PS_DATA_MASK);
			}
			AP3426_DEBUG("%s,line %d:ps threshold: 0x%x 0x%x 0x%x 0x%x\n",__func__,__LINE__,
					ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
			res = regmap_bulk_write(di->regmap,
					AP3426_REG_PS_LOW_THRES_0, ps_data, 4);
			if (res) {
				AP3426_ERR("%s,line %d:set up threshold failed\n",__func__,__LINE__);
				goto exit;
			}
		}
	} else {
		/* power off */
		disable_irq(di->irq);
		if (di->power_enabled) {
			res = sensor_power_config(dev, power_config,
					ARRAY_SIZE(power_config), false);
			if (res) {
				AP3426_ERR("%s,line %d:failed to suspend ap3426\n",__func__,__LINE__);
				enable_irq(di->irq);
				goto exit;
			}
		}
		pinctrl_select_state(pin_config.pinctrl, pin_config.state[1]);
	}
exit:
	mutex_unlock(&di->ops_lock);
	return res;
}

static int ap3426_resume(struct device *dev)
{
	int res = 0;
	struct ap3426_data *di = dev_get_drvdata(dev);
	unsigned int config;

	AP3426_DEBUG("%s,line %d:resuming ap3426...",__func__,__LINE__);
	if (di->ps_enabled) {
		if (device_may_wakeup(&di->i2c->dev)) {
			AP3426_DEBUG("%s,line %d:disable irq wake\n",__func__,__LINE__);
			disable_irq_wake(di->irq);
		}

		if (di->als_enabled) {
			res = regmap_read(di->regmap, AP3426_REG_CONFIG,
					&config);
			if (res) {
				AP3426_ERR("%s,line %d:read %d failed.(%d)\n",__func__,__LINE__,
						AP3426_REG_CONFIG, res);
				goto exit;
			}

			res = regmap_write(di->regmap, AP3426_REG_CONFIG,
					config | 0x1);
			if (res) {
				AP3426_ERR("%s,line %d:write %d failed.(%d)\n",__func__,__LINE__,
						AP3426_REG_CONFIG, res);
				goto exit;
			}

			ap3426_sync_delay(di, 1, 1, di->als_delay,
					di->ps_delay);
		}

	} else {
		pinctrl_select_state(pin_config.pinctrl, pin_config.state[0]);
		/* Power up sensor */
		if (di->power_enabled) {
			res = sensor_power_config(dev, power_config,
					ARRAY_SIZE(power_config), true);
			if (res) {
				AP3426_ERR("%s,line %d:failed to power up ap3426\n",__func__,__LINE__);
				goto exit;
			}
			msleep(AP3426_BOOT_TIME_MS);

			res = ap3426_init_device(di);
			if (res) {
				AP3426_ERR("%s,line %d:failed to init ap3426\n",__func__,__LINE__);
				goto exit_power_off;
			}

			ap3426_sync_delay(di, di->als_enabled, 0, di->als_delay,
					di->ps_delay);
		}

		if (di->als_enabled) {
			res = ap3426_enable_als(di, di->als_enabled);
			if (res) {
				AP3426_ERR("%s,line %d:failed to enable ap3426\n",__func__,__LINE__);
				goto exit_power_off;
			}
		}

		enable_irq(di->irq);
	}

	return res;

exit_power_off:
	if ((!di->als_enabled) && (!di->ps_enabled) &&
			di->power_enabled) {
		if (sensor_power_config(&di->i2c->dev, power_config,
					ARRAY_SIZE(power_config), false)) {
			AP3426_ERR("%s,line %d:power up sensor failed.\n",__func__,__LINE__);
			goto exit;
		}
		di->power_enabled = false;
	}
/* DTS2015072505158 wuying/wx221431 20150725 end > */
exit:
	return res;
}

static const struct i2c_device_id ap3426_id[] = {
	{ AP3426_I2C_NAME, 0 },
	{ }
};

static struct of_device_id ap3426_match_table[] = {
	{ .compatible = "di,ap3426", },
	{ },
};

static const struct dev_pm_ops ap3426_pm_ops = {
	.suspend = ap3426_suspend,
	.resume = ap3426_resume,
};

static struct i2c_driver ap3426_driver = {
	.probe		= ap3426_probe,
	.remove	= ap3426_remove,
	.id_table	= ap3426_id,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= AP3426_I2C_NAME,
		.of_match_table = ap3426_match_table,
		.pm = &ap3426_pm_ops,
	},
};

module_i2c_driver(ap3426_driver);

MODULE_DESCRIPTION("AP3426 ALPS Driver");
MODULE_LICENSE("GPLv2");
/* DTS2016040502547 chendong 20160407 end> */
