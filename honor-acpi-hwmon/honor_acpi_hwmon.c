// SPDX-License-Identifier: GPL-2.0-only
/*
 * Read-only EC telemetry for HONOR NBLK-WAX9X, BIOS 3.07.
 *
 * GTMP/GFNS are the getters behind Huawei WMI commands 0x0202/0x0802.
 * Calling the getters directly avoids sharing WMAA's global two-call
 * counter with huawei-wmi's battery, keyboard and LED operations.
 * The audited getters read the firmware's EC OperationRegion; they do
 * not invoke SMI or write EC control registers. No raw MMIO is used here.
 */

#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/hwmon.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/slab.h>

#define DRV_NAME "honor_acpi_hwmon"
#define UPDATE_INTERVAL_MS 1000
#define TEMP_COUNT 6
#define FAN_INDEX TEMP_COUNT
#define SENSOR_COUNT (TEMP_COUNT + 1)

static const u8 temp_ids[TEMP_COUNT] = { 0x00, 0x05, 0x07, 0x08, 0x0b, 0x0e };
static const char * const temp_labels[TEMP_COUNT] = {
	"CPU (EC)", "TSLO (EC)", "TNTC (EC)", "CNTC (EC)", "DNTC (EC)",
	"Battery (EC)",
};

struct honor_sample {
	long value;
	unsigned long updated;
	int error;
	bool valid;
};

struct honor_data {
	acpi_handle temp_method;
	acpi_handle fan_method;
	/* Protects cached samples and the system-sleep state. */
	struct mutex lock;
	struct honor_sample samples[SENSOR_COUNT];
	bool suspended;
};

static struct platform_device *honor_device;

static const struct dmi_system_id honor_models[] = {
	{
		.ident = "HONOR NBLK-WAX9X BIOS 3.07",
		.matches = {
			DMI_EXACT_MATCH(DMI_SYS_VENDOR, "HONOR"),
			DMI_EXACT_MATCH(DMI_PRODUCT_NAME, "NBLK-WAX9X"),
			DMI_EXACT_MATCH(DMI_BOARD_NAME, "NBLK-WAX9X-PCB"),
			DMI_EXACT_MATCH(DMI_BIOS_VERSION, "3.07"),
		},
	},
	{ }
};
MODULE_DEVICE_TABLE(dmi, honor_models);

/* Caller holds data->lock. Errors are cached too, to bound firmware traffic. */
static int honor_sample(struct honor_data *data, int index, long *value)
{
	struct honor_sample *sample = &data->samples[index];
	bool fan = index == FAN_INDEX;
	u8 request[8] = { 0x02, fan ? 0x08 : 0x02, 0 };
	union acpi_object arg = {
		.buffer = {
			.type = ACPI_TYPE_BUFFER,
			.length = sizeof(request),
			.pointer = request,
		},
	};
	struct acpi_object_list args = { .count = 1, .pointer = &arg };
	struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object *obj;
	acpi_status status;
	u8 *response;
	int err;

	if (data->suspended)
		return -EAGAIN;

	if (sample->valid && time_before(jiffies, sample->updated +
					msecs_to_jiffies(UPDATE_INTERVAL_MS))) {
		*value = sample->value;
		return sample->error;
	}

	request[2] = fan ? 0 : temp_ids[index];
	status = acpi_evaluate_object(fan ? data->fan_method : data->temp_method,
				      NULL, &args, &output);
	obj = output.pointer;
	if (ACPI_FAILURE(status)) {
		err = -EIO;
		goto out;
	}
	if (!obj || obj->type != ACPI_TYPE_BUFFER || obj->buffer.length < 3) {
		err = -EPROTO;
		goto out;
	}

	response = obj->buffer.pointer;
	if (response[0] || (!fan && response[2] == 0xff)) {
		err = -ENODATA;
		goto out;
	}

	if (fan) {
		sample->value = response[1] | (response[2] << 8);
		if (sample->value == 0xffff) {
			err = -ENODATA;
			goto out;
		}
	} else {
		sample->value = response[2] * 1000L;
	}
	err = 0;
out:
	kfree(output.pointer);
	sample->updated = jiffies;
	sample->valid = true;
	sample->error = err;
	*value = sample->value;
	return err;
}

static umode_t honor_is_visible(const void *drvdata,
				enum hwmon_sensor_types type, u32 attr, int channel)
{
	switch (type) {
	case hwmon_chip:
		return attr == hwmon_chip_update_interval ? 0444 : 0;
	case hwmon_temp:
		if (channel < 0 || channel >= TEMP_COUNT)
			return 0;
		return attr == hwmon_temp_input || attr == hwmon_temp_label ? 0444 : 0;
	case hwmon_fan:
		if (channel != 0)
			return 0;
		return attr == hwmon_fan_input || attr == hwmon_fan_label ? 0444 : 0;
	default:
		return 0;
	}
}

static int honor_read(struct device *dev, enum hwmon_sensor_types type,
		      u32 attr, int channel, long *value)
{
	struct honor_data *data = dev_get_drvdata(dev);
	int index, err;

	if (type == hwmon_chip && attr == hwmon_chip_update_interval) {
		*value = UPDATE_INTERVAL_MS;
		return 0;
	}
	if (type == hwmon_temp && attr == hwmon_temp_input &&
	    channel >= 0 && channel < TEMP_COUNT)
		index = channel;
	else if (type == hwmon_fan && attr == hwmon_fan_input && channel == 0)
		index = FAN_INDEX;
	else
		return -EOPNOTSUPP;

	mutex_lock(&data->lock);
	err = honor_sample(data, index, value);
	mutex_unlock(&data->lock);
	return err;
}

static int honor_read_string(struct device *dev, enum hwmon_sensor_types type,
			     u32 attr, int channel, const char **str)
{
	if (type == hwmon_temp && attr == hwmon_temp_label &&
	    channel >= 0 && channel < TEMP_COUNT) {
		*str = temp_labels[channel];
		return 0;
	}
	if (type == hwmon_fan && attr == hwmon_fan_label && channel == 0) {
		*str = "CPU fan (EC)";
		return 0;
	}
	return -EOPNOTSUPP;
}

static const struct hwmon_ops honor_ops = {
	.is_visible = honor_is_visible,
	.read = honor_read,
	.read_string = honor_read_string,
};

static const struct hwmon_channel_info * const honor_info[] = {
	HWMON_CHANNEL_INFO(chip, HWMON_C_UPDATE_INTERVAL),
	HWMON_CHANNEL_INFO(temp,
			   HWMON_T_INPUT | HWMON_T_LABEL, HWMON_T_INPUT | HWMON_T_LABEL,
			   HWMON_T_INPUT | HWMON_T_LABEL, HWMON_T_INPUT | HWMON_T_LABEL,
			   HWMON_T_INPUT | HWMON_T_LABEL, HWMON_T_INPUT | HWMON_T_LABEL),
	HWMON_CHANNEL_INFO(fan, HWMON_F_INPUT | HWMON_F_LABEL),
	NULL
};

static const struct hwmon_chip_info honor_chip_info = {
	.ops = &honor_ops,
	.info = honor_info,
};

static int honor_suspend(struct device *dev)
{
	struct honor_data *data = dev_get_drvdata(dev);

	mutex_lock(&data->lock);
	data->suspended = true;
	mutex_unlock(&data->lock);
	return 0;
}

static int honor_resume(struct device *dev)
{
	struct honor_data *data = dev_get_drvdata(dev);
	int i;

	mutex_lock(&data->lock);
	for (i = 0; i < SENSOR_COUNT; i++)
		data->samples[i].valid = false;
	data->suspended = false;
	mutex_unlock(&data->lock);
	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(honor_pm_ops, honor_suspend, honor_resume);

static int honor_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct honor_data *data;
	struct device *hwmon;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	if (ACPI_FAILURE(acpi_get_handle(NULL, "\\GTMP", &data->temp_method)) ||
	    ACPI_FAILURE(acpi_get_handle(NULL, "\\GFNS", &data->fan_method)))
		return -ENODEV;

	mutex_init(&data->lock);
	platform_set_drvdata(pdev, data);

	hwmon = devm_hwmon_device_register_with_info(dev, DRV_NAME, data,
						     &honor_chip_info, NULL);
	if (IS_ERR(hwmon))
		return PTR_ERR(hwmon);

	dev_info(dev, "Registered %d read-only EC sensors\n", SENSOR_COUNT);
	return 0;
}

static struct platform_driver honor_driver = {
	.driver = {
		.name = DRV_NAME,
		.pm = pm_sleep_ptr(&honor_pm_ops),
	},
};

static int __init honor_init(void)
{
	if (acpi_disabled || !dmi_check_system(honor_models))
		return -ENODEV;

	honor_device = platform_create_bundle(&honor_driver, honor_probe,
					      NULL, 0, NULL, 0);
	return PTR_ERR_OR_ZERO(honor_device);
}

static void __exit honor_exit(void)
{
	platform_device_unregister(honor_device);
	platform_driver_unregister(&honor_driver);
}

module_init(honor_init);
module_exit(honor_exit);

MODULE_DESCRIPTION("Read-only ACPI EC telemetry for HONOR NBLK-WAX9X");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1.1");
