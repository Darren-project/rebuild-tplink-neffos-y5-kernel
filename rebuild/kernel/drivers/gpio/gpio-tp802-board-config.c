/* Copyright (C) 1996-2016, TP-LINK TECHNOLOGIES CO., LTD.
 *
 * File name: gpio-tp802-board-config.c
 *
 * Description: This drvier reads the value of GPIO-35. TP802 has two
 *              configuration: A and B. GPIO-35's output can be used to judge
 *              the configuration that TP802 uses.
 *
 * Author: Yubin Yang
 *
 * Email: yangyubin@tp-link.com.cn
 */

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>

#define TP802_BOARD_CONFIG_GPIO_DRIVER_NAME  "qcom,tp802_board_config"

static int tp802_board_config_gpio;
static struct kobject *tp802_board_config_kobj;

static ssize_t tp802_board_config_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d",
			gpio_get_value(tp802_board_config_gpio));
}

static DEVICE_ATTR(tp802_board_config, S_IRUGO, tp802_board_config_show, NULL);

static struct attribute *tp802_board_config_attrs[] = {
	&dev_attr_tp802_board_config.attr,
	NULL,
};

static struct attribute_group tp802_board_config_attr_group = {
	.attrs = tp802_board_config_attrs,
};

static struct of_device_id tp802_board_config_gpio_of_match[] = {
	{.compatible = TP802_BOARD_CONFIG_GPIO_DRIVER_NAME,},
	{},
};

int tp802_board_config_gpio_probe(struct platform_device *pdev)
{
	int ret;
	struct device_node *node = pdev->dev.of_node;

	struct pinctrl *tp802_board_config_pinctrl;
	struct pinctrl_state *gpio_state_default;

	tp802_board_config_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(tp802_board_config_pinctrl)) {
		pr_err("%s: failed to get pinctrl\n", __func__);
		return PTR_ERR(tp802_board_config_pinctrl);
	}

	gpio_state_default = pinctrl_lookup_state(tp802_board_config_pinctrl,
			"tp802_board_config_default");
	if (IS_ERR(gpio_state_default)) {
		pr_err("%s: can not get default pinstate\n", __func__);
		return -EINVAL;
	}

	ret = pinctrl_select_state(tp802_board_config_pinctrl,
			gpio_state_default);
	if (ret) {
		pr_err("%s: set state failed!\n", __func__);
		return -EINVAL;
	}

	tp802_board_config_gpio = of_get_gpio(node, 0);
	if (tp802_board_config_gpio < 0) {
		pr_err("%s: Fail to get tp802 board config gpio\n", __func__);
		return -EINVAL;
	}

	ret = gpio_request(tp802_board_config_gpio, "tp802_board_config");
	if (ret) {
		pr_err("%s: Request failed for gpio\n", __func__);
		return -EINVAL;
	}

	ret = gpio_direction_input(tp802_board_config_gpio);
	if (ret) {
		pr_err("%s: set direction error!\n", __func__);
		return -EINVAL;
	}

	tp802_board_config_kobj = kobject_create_and_add("tp802_board_config",
			NULL);
	if (!tp802_board_config_kobj) {
		pr_err("%s: Fail to create tp802_board_config_kobj\n",
				__func__);
		return -ENOMEM;
	}

	ret = sysfs_create_group(tp802_board_config_kobj,
			&tp802_board_config_attr_group);
	if (ret) {
		kobject_put(tp802_board_config_kobj);
	}

	return ret;
}

int tp802_board_config_gpio_remove(struct platform_device *pdev)
{
	if (gpio_is_valid(tp802_board_config_gpio)) {
		gpio_free(tp802_board_config_gpio);
	}
	if (tp802_board_config_kobj) {
		kobject_put(tp802_board_config_kobj);
	}

	return 0;
}

static struct platform_driver tp802_board_config_gpio_driver = {
	.probe = tp802_board_config_gpio_probe,
	.remove = tp802_board_config_gpio_remove,
	.driver = {
		.name = TP802_BOARD_CONFIG_GPIO_DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = tp802_board_config_gpio_of_match,
	}
};

static int __init tp802_board_config_gpio_init(void)
{
	return platform_driver_register(&tp802_board_config_gpio_driver);
}

static void __exit tp802_board_config_gpio_exit(void)
{
	return platform_driver_unregister(&tp802_board_config_gpio_driver);
}

module_init(tp802_board_config_gpio_init);
module_exit(tp802_board_config_gpio_exit);

MODULE_DESCRIPTION("TP802 BOARD CONFIG GPIO DRIVER");
MODULE_LICENSE("GPL v2");
