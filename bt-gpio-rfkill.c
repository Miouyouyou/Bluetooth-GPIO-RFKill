/**
 * This a test driver to setup various Bluetooth chips GPIO pins.
 * 
 * Most of the "communication" part of the Bluetooth driver part is set
 * up through BlueZ, using the HCI protocol through UART to configure
 * the device, generate appropriate /dev/ nodes with associate an
 * additional rfkill system to these nodes.
 * 
 * This driver part only ensure that the Bluetooth chip is waken up
 * correctly, so that it can communicate through the UART channels.
 * 
 * Released under GPLv2 license.
 * Author : Miouyouyou (Myy)
 */

/* If you got a dumb IDE that does not support parsing defines from
 * a .h file, but support defining special macro values, then define
 * DUMBY_THE_EDITOR to 1 in your IDE to parse all of the macros
 * generated during the kernel compilation, based on the kernel
 * configuration.
 */
#ifdef DUMBY_THE_EDITOR
#include <generated/autoconf.h>
#endif

// THIS_MODULE and others things
#include <linux/module.h>

// platform_driver, platform_device, module_platform_driver
// Also include mod_devicetable.h
// of_device_id
#include <linux/platform_device.h>

// of_match_ptr
#include <linux/of.h>

// devm_gpiod related functions
#include <linux/gpio/consumer.h>

// rfkill related functions
#include <linux/rfkill.h>

// mdelay
#include <linux/delay.h>

#include "analysis_helpers.h"

struct myy_driver_private_data;

// --- DATA STRUCTURES

/* There's actually an IRQ setup on Rockchip kernels, but I really
 * don't see its point at the moment, so I don't handle it
 * for the moment.
 */
enum myy_gpio_names_indices {
	myy_gpio_power_index,
	myy_gpio_reset_index,
	myy_gpio_wakeup_index,
};

/* In the DTS, its the same property name without the -gpios prefix.
 * So bluetooth-power is actually named bluetooth-power-gpios in the DTS
 */
static char const * __restrict myy_gpio_names[] = {
	[myy_gpio_power_index]  = "bluetooth-power",
	[myy_gpio_reset_index]  = "bluetooth-reset",
	[myy_gpio_wakeup_index] = "bluetooth-wakeup",
};

static char const myy_rfkill_name[] = "bt-gpio";

/* Our driver's private data... Only store the power and reset GPIO
 * identifiers.
 */
struct myy_driver_private_data {
	struct {
		struct gpio_desc * power, * reset, * wakeup;
	} gpios;
	struct rfkill * __restrict rfkill_system;
};

static const struct of_device_id myy_compatible_ids[] = {
	{
		/* This must match your Bluetooth chip "compatible" node, in the
		 * DTS file.
		 * If you want to use this driver with Bluetooth chips that
		 * harbour a different "compatible" property, you better append a
		 * new structure to this array and setup the compatible member
		 * accordingly. */
		.compatible = "rockchip,vpu_service",
		/* You can init a custom data structure before and set its address
		 * to the .data member (.data = &myy_initialized_data_structure).
		 */
		.data = NULL
	},
	{ /* This empty structure serves as a sentinel. Do not remove */ }
};

// --- RFKILL FUNCTIONS

/* This methodology is copied from Toshiba Bluetooth drivers.
 * Might want to adapt it...
 * Only called explicitly.
 */
static void myy_rfkill_bt_on
(struct myy_driver_private_data * driver_private_data)
{
	gpiod_set_value(driver_private_data->gpios.reset, 0);
	gpiod_set_value(driver_private_data->gpios.power, 1);
	gpiod_set_value(driver_private_data->gpios.reset, 1);
	mdelay(20);
	gpiod_set_value(driver_private_data->gpios.reset, 0);
}

/* This methodology is copied from Toshiba Bluetooth drivers.
 * Might want to adapt it...
 * Only called explicitly.
 */
static void myy_rfkill_bt_off
(struct myy_driver_private_data * driver_private_data)
{
	gpiod_set_value(driver_private_data->gpios.reset, 1);
	mdelay(10);
	gpiod_set_value(driver_private_data->gpios.power, 0);
	gpiod_set_value(driver_private_data->gpios.reset, 0);
}

/* Called implicitly, since we set up this function address to the
 * .set_block member of the rfkill structure passed to rfkill_alloc.
 */
static int myy_rfkill_bt_set_block
(void * driver_private_data, bool blocked)
{
	pr_info(
		"Simple BT RFKILL : blocked =  %s ?\n", myy_bool_str(blocked)
	);

	if (!blocked) {
		myy_rfkill_bt_on(driver_private_data);
		pr_info("Simple BT : I'm alive ! RFKILL Unblocked !\n");
	} else {
		myy_rfkill_bt_off(driver_private_data);
		pr_info("Simple BT : Blargh ! I'm being RFKilled !\n");
	}

	return 0;
}

/* The RFKill structure, passed to rfkill_alloc, that define the
 * functions to call for various operations.
 * Currently, only the "Set Block" function is set. This function
 * is called when blocking or unblocking the device with RFKill user
 * programs.
 */
static const struct rfkill_ops myy_rfkill_ops = {
	.set_block = myy_rfkill_bt_set_block,
};

// --- DRIVER SETUP UTILITIES FUNCTIONS

/**
 * Find and setup the Bluetooth chip GPIOS
 * 
 * @param bluetooth_dev       The bluetooth device structure address
 * @param driver_private_data The driver's private data address
 * 
 * @return
 * 0       on success
 * -ENOSYS on failure
 */
static int find_the_bluetooth_gpios
(struct device * __restrict const bluetooth_dev,
 struct myy_driver_private_data * __restrict const driver_private_data)
{
	struct gpio_desc * power_gpio, * reset_gpio, * wakeup_gpio;
	int ret = 0;

	/* Try to set everyting up at once */
	power_gpio = devm_gpiod_get(
		bluetooth_dev, myy_gpio_names[myy_gpio_power_index], GPIOD_OUT_HIGH
	);
	reset_gpio = devm_gpiod_get(
		bluetooth_dev, myy_gpio_names[myy_gpio_reset_index], GPIOD_OUT_HIGH
	);
	wakeup_gpio = devm_gpiod_get(
		bluetooth_dev, myy_gpio_names[myy_gpio_wakeup_index], GPIOD_OUT_HIGH
	);

	driver_private_data->gpios.power  = power_gpio;
	driver_private_data->gpios.reset  = reset_gpio;
	driver_private_data->gpios.wakeup = wakeup_gpio;

	if (IS_ERR(power_gpio) || IS_ERR(reset_gpio) || IS_ERR(wakeup_gpio)) {
		/* Let's try to provide a useful message if the detection failed. */
		dev_err(bluetooth_dev,
			"An error occured while trying to find the right GPIO.\n"
			"  Power GPIO   [%s-gpios] - Problem when parsing ? %s\n"
			"  Reset GPIO   [%s-gpios] - Problem when parsing ? %s\n"
			"  Wake up GPIO [%s-gpios] - Problem when parsing ? %s\n",
			myy_gpio_names[myy_gpio_power_index],
			myy_bool_str(IS_ERR(power_gpio)),
			myy_gpio_names[myy_gpio_reset_index],
			myy_bool_str(IS_ERR(reset_gpio)),
			myy_gpio_names[myy_gpio_wakeup_index],
			myy_bool_str(IS_ERR(wakeup_gpio))
		);
		ret = -ENOSYS;
	}

	return ret;
}

/**
 * 
 * Setup the RFKill system on the Bluetooth device.
 * 
 * @param bluetooth_dev       The Bluetooth device structure address
 * @param driver_private_data This driver private data
 * 
 * @return
 * 0 on success
 * -ERRNO on failure.
 *   Can be anything returned by rfkill_alloc or rfkill_register.
 */
int setup_the_rfkill_system
(struct device * __restrict const bluetooth_dev,
 struct myy_driver_private_data * __restrict const driver_private_data)
{
	int ret = 0;

	/* The last argument is actually the address of the data structure
	 * that is passed to the RFKILL ".set_block" function.
	 */
	struct rfkill * __restrict rfkill_system =
		rfkill_alloc(
			myy_rfkill_name, bluetooth_dev, RFKILL_TYPE_BLUETOOTH,
			&myy_rfkill_ops, driver_private_data);
	if (!rfkill_system) {
		ret = -ENOMEM;
		goto err_rfk_alloc;
	}

	// This will actually set up a /dev/rfkillX node, it seems...
	ret = rfkill_register(rfkill_system);
	if (ret)
		goto err_rfkill;

	driver_private_data->rfkill_system = rfkill_system;
	return ret;

err_rfk_alloc:
	rfkill_destroy(rfkill_system);
err_rfkill:
	myy_rfkill_bt_off(driver_private_data);

	return ret;
}

// --- DEVICE PROBING AND INIT

/* Should return 0 on success and a negative errno on failure. */
static int myy_bluetooth_probe(struct platform_device * pdev)
{
	/* The device associated with the platform_device. Identifier
	 * used for convenience.
	 * (Writing &pdev->dev every time is a waste of keyboard presses and
	 *  instructions.) */
	struct device * __restrict const bluetooth_dev = &pdev->dev;

	/* Store our own structure containing our own driver's private data.
	 * devm_kzalloc allocated memory will be deallocated on device's
	 * removal (Which will only happen with rmmod or probing failure in
	 * the case of SDIO/GPIO Bluetooth chips ...).
	 */
	struct myy_driver_private_data * __restrict const driver_data =
		devm_kzalloc(
			&pdev->dev, sizeof(struct myy_driver_private_data *), GFP_KERNEL
		);

	/* Used to check various return codes for errors */
	int ret;

	platform_set_drvdata(pdev, driver_data);

	ret = find_the_bluetooth_gpios(bluetooth_dev, driver_data);

	if (ret)
		goto err;

	ret = setup_the_rfkill_system(bluetooth_dev, driver_data);
	if (ret)
		goto err;
err:
	return ret;
}

/* Should return 0 on success and a negative errno on failure. */
static int myy_bluetooth_remove(struct platform_device * pdev)
{
	struct myy_driver_private_data * __restrict const driver_data =
		(struct myy_driver_private_data * __restrict)
			platform_get_drvdata(pdev);

	/* driver_data was allocated through devm_kzalloc and will be
	 * deallocated automatically.
	 */

	if (!IS_ERR(driver_data->gpios.power))
		gpiod_put(driver_data->gpios.power);

	if (!IS_ERR(driver_data->gpios.reset))
		gpiod_put(driver_data->gpios.power);

	if (!IS_ERR(driver_data->gpios.wakeup))
		gpiod_put(driver_data->gpios.wakeup);
		
	if (driver_data->rfkill_system) {
		rfkill_destroy(driver_data->rfkill_system);
		myy_rfkill_bt_off(driver_data);
	}
	return 0;
}

static void myy_bluetooth_shutdown(struct platform_device *pdev)
{
	printk(KERN_INFO "Shutting down...\n");
}

static struct platform_driver myy_bt_gpio_rfkill_driver = {
	.probe = myy_bluetooth_probe,
	.remove = myy_bluetooth_remove,
	.shutdown = myy_bluetooth_shutdown,
	.driver = {
		.name = "myy-bt-gpio",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(myy_compatible_ids),
	},
};

module_platform_driver(myy_bt_gpio_rfkill_driver);
MODULE_AUTHOR("Miouyouyou (Myy)");
MODULE_DESCRIPTION(
	"Test driver enabling an RFKILL system which enable or disable\n"
	"appropriate Bluetooth related GPIO pins, in order to use the\n"
	"associated Bluetooth device with BlueZ afterwards.\n"
);
MODULE_LICENSE("GPL v2");
