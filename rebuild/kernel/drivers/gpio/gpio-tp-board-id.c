
/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/list.h>
#include <linux/pinctrl/consumer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#define BOARD_ID_GPIO_DRIVER_NAME	"qcom,tp_board_id"
#define BOARD_ID_MAX_GPIO		2

static int board_id_gpio[BOARD_ID_MAX_GPIO];
static struct kobject *board_info_kobj;

static ssize_t board_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        return scnprintf(buf, PAGE_SIZE, "%d%d", gpio_get_value(board_id_gpio[0]), gpio_get_value(board_id_gpio[1]));
}

static DEVICE_ATTR(board_id, S_IRUGO, board_id_show, NULL);

static struct attribute *board_id_attrs[] = {
	&dev_attr_board_id.attr,
	NULL,
};

static struct attribute_group board_info_attr_group = {
	.attrs = board_id_attrs,
};

static struct of_device_id tp_board_id_gpio_of_match[] = {
	{.compatible = BOARD_ID_GPIO_DRIVER_NAME,},
	{},
};

int tp_board_id_gpio_probe(struct platform_device *pdev)
{
	int i, ret;
	struct device_node *node = pdev->dev.of_node;
#if 0
	struct pinctrl *board_id_pinctrl;
	struct pinctrl_state * gpio_state_default;

	board_id_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(board_id_pinctrl)) {
		pr_err("%s:failed to get pinctrl\n", __func__);
		return PTR_ERR(board_id_pinctrl);
	}

	gpio_state_default = pinctrl_lookup_state(board_id_pinctrl, "board_id_default");
        if (IS_ERR(gpio_state_default)) {
		pr_err("%s:can not get default pinstate\n", __func__);
		return -EINVAL;
	}

	ret = pinctrl_select_state(board_id_pinctrl, gpio_state_default);
        if (ret) {
		pr_err("%s:set state failed!\n", __func__);
		return -EINVAL;
	}
#endif
	for (i = 0; i < BOARD_ID_MAX_GPIO; i++) {
		board_id_gpio[i] = of_get_gpio(node, i);
		if (board_id_gpio[i] < 0) {
			pr_err("%s: Fail to get board id gpio: %d\n", __func__, i);
			return -EINVAL;
		}

		ret = gpio_request_one(board_id_gpio[i], GPIOF_DIR_IN, NULL);
		if (ret) {
			pr_err("%s: request failed for gpio:%d\n", __func__, board_id_gpio[i]);
			return -EINVAL;
		}
	}

	board_info_kobj = kobject_create_and_add("tp_board_info", NULL);
	if (!board_info_kobj ) {
		pr_err("%s: Fail to create board_info_kobj\n",  __func__);
		return -ENOMEM;
	}

	/* Create the files associated with this kobject */
	ret = sysfs_create_group(board_info_kobj, &board_info_attr_group);
	if (ret) {
		kobject_put(board_info_kobj);
	}

	return ret;
}

int tp_board_id_gpio_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < BOARD_ID_MAX_GPIO; i++) {
		if (gpio_is_valid(board_id_gpio[i]))
			gpio_free(board_id_gpio[i]);
	}

	if (board_info_kobj) {
		kobject_put(board_info_kobj);
	}

	return 0;
}

static struct platform_driver tp_board_id_gpio_driver = {
	.probe = tp_board_id_gpio_probe,
	.remove = tp_board_id_gpio_remove,
	.driver = {
		   .name = BOARD_ID_GPIO_DRIVER_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = tp_board_id_gpio_of_match,
		   }
};

static int __init tp_board_id_gpio_init(void)
{
	return platform_driver_register(&tp_board_id_gpio_driver);
}

static void __exit tp_board_id_gpio_exit(void)
{
	return platform_driver_unregister(&tp_board_id_gpio_driver);
}

module_init(tp_board_id_gpio_init);
module_exit(tp_board_id_gpio_exit);

MODULE_DESCRIPTION("TP BOARD ID GPIO driver");
MODULE_LICENSE("GPL v2");
