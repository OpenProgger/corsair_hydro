/*
 * HID driver for Corsair Hydro devices
 *
 * Supported devices:
 *	- H110i Extreme
 *
 * Copyright (c) 2018 Lukas Kahnert
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include <linux/hid.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define USB_VENDOR_ID_CORSAIR		0x1b1c
#define USB_DEVICE_ID_CORSAIR_HYDRO	0x0c04

struct corsair_hydro_data {
	struct usb_device *usbdev;
	char *buf;
	const struct attribute_group* corsair_hydro_sensor_groups[2];
	struct sensor_device_attribute* corsair_hydro_sensor_attributes;
};

static int corsair_hydro_device_ids[] = {
	0x3b, /* H80i */
	0x3c, /* H100i */
	0x41, /* H110i */
	0x42  /* H110i Extreme*/
};

static int send_msg(struct corsair_hydro_data* data) {
	int ret, actual_length;
	ret = usb_control_msg(data->usbdev, usb_sndctrlpipe(data->usbdev, 0),
				HID_REQ_SET_REPORT, USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, (0x02<<8)|0x00, 0,
				data->buf, 64, USB_CTRL_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(&data->usbdev->dev, "Failed to send HID Request(error %d)\n", ret);
		return ret;
	}
	memset(data->buf, 0, 64);
	ret = usb_interrupt_msg(data->usbdev, usb_rcvintpipe(data->usbdev, 0x81),
				data->buf, 64, &actual_length,
				USB_CTRL_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(&data->usbdev->dev, "Failed to get HID Response(error: %d).\n", ret);
		return ret;
	}
	return 0;
}

static ssize_t custom_profile_show(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	memcpy(data->buf, (char[]){ 0x08, 0x0e, 0x0b, 0x19, 0x0a, 0x0f, 0x0b, 0x1a, 0x0a }, 9 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	ret = sprintf(buf, "%x%x %x%x %x%x %x%x %x%x\n%x%x %x%x %x%x %x%x %x%x\n", data->buf[3], data->buf[4], data->buf[5], data->buf[6], data->buf[7], data->buf[8], data->buf[9], data->buf[10], data->buf[11], data->buf[12], data->buf[16], data->buf[17], data->buf[18], data->buf[19], data->buf[20], data->buf[21], data->buf[22], data->buf[23], data->buf[24], data->buf[25]);
	return ret;
}

static ssize_t custom_profile_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	int table[10];
	ret = sscanf(buf, "%d %d %d %d %d %d %d %d %d %d", &table[0], &table[1], &table[2], &table[3], &table[4], &table[5], &table[6], &table[7], &table[8], &table[9]);
	if(ret < 9)
		return -EINVAL;
	memcpy(data->buf, (char[]){ 0x1c, 0x10, 0x0a, 0x19, 0x0a, table[1] & 0xff, table[1] << 8, table[3] & 0xff, table[3] >> 8, table[5] & 0xff, table[5] >> 8, table[7] & 0xff, table[7] >> 8, table[9] & 0xff, table[9] >> 8, 0x11, 0x0a, 0x1a, 0x0a, 0x00, table[0], 0x00, table[2], 0x00, table[4], 0x00, table[6], 0x00, table[8] }, 29 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	return count;
}

static ssize_t led_mode_show(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	memcpy(data->buf, (char[]){0x03, 0x13, 0x07, 0x06}, 4 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	ret = sprintf(buf, "%x\n", data->buf[2]);
	return ret;
}
static ssize_t led_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	unsigned mode = 0;
	if(kstrtouint(buf, 0, &mode) || mode > 0xcf)
				return -EINVAL;
	memcpy(data->buf, (char[]){0x04, 0x14, 0x06, 0x06, mode}, 5 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	return count;
}

static ssize_t led_colors_show(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	memcpy(data->buf, (char[]){0x04, 0x15, 0x0b, 0x0b, 0x0c}, 5 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	ret = sprintf(buf, "%x%x%x %x%x%x %x%x%x %x%x%x\n", data->buf[3], data->buf[4], data->buf[5], data->buf[6], data->buf[7], data->buf[8], data->buf[9], data->buf[10], data->buf[11], data->buf[12], data->buf[16], data->buf[17]);
	return ret;
}
static ssize_t led_colors_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret;
    struct corsair_hydro_data *data = dev_get_drvdata(dev);
    unsigned color[4];
    ret = sscanf(buf, "%x %x %x %x", &color[0], &color[1], &color[2], &color[3]);
    if(ret < 4)
        return -EINVAL;
    memcpy(data->buf, (char[]){0x10, 0x30, 0x0a, 0x0b, 0x0c, color[0] >> 16, (color[0] >> 8) & 0xff, color[0] & 0xff, color[1] >> 16, (color[1] >> 8) & 0xff, color[1] & 0xff, color[2] >> 16, (color[2] >> 8) & 0xff, color[2] & 0xff, color[3] >> 16, (color[3] >> 8) & 0xff, color[3] & 0xff}, 17 * sizeof data->buf[0]); 
    ret = send_msg(data);
    return ret ? ret : count;
}

static ssize_t led_temp_profile_show(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	memcpy(data->buf, (char[]){0x08, 0x18, 0x0b, 0x09, 0x06, 0x19, 0x0b, 0x0a, 0x09}, 9 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	ret = sprintf(buf, "%x %x %x\n%x%x%x %x%x%x %x%x%x\n", data->buf[4], data->buf[6], data->buf[8], data->buf[9], data->buf[10], data->buf[11], data->buf[12], data->buf[13], data->buf[14], data->buf[15], data->buf[16], data->buf[17]);
	return ret;

}
static ssize_t led_temp_profile_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	int temp[3];
	unsigned color[3];
	ret = sscanf(buf, "%d %x %d %x %d %x", &temp[0], &color[0], &temp[1], &color[1], &temp[2], &color[2]);
	if(ret < 6)
		return -EINVAL;
	memcpy(data->buf, (char[]){0x17, 0x20, 0x0a, 0x09, 0x06, 0x00, temp[0], 0x00, temp[1], 0x00, temp[2], 0x21, 0x0a, 0x0a, 0x09, color[0] >> 16, (color[0] >> 8) & 0xff, color[0] & 0xff, color[1] >> 16, (color[1] >> 8) & 0xff, color[1] & 0xff, color[2] >> 16, (color[2] >> 8) & 0xff, color[2] & 0xff}, 24 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	return count;
}

static ssize_t led_ext_temp_show(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	memcpy(data->buf, (char[]){0x03, 0x16, 0x09, 0x08}, 4 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	ret = sprintf(buf, "%x%x\n", data->buf[2], data->buf[3]);
	return ret;
}
static ssize_t led_ext_temp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret;
		struct corsair_hydro_data *data = dev_get_drvdata(dev);
		unsigned temp = 0;
		if(kstrtouint(buf, 10, &temp))
			return -EINVAL;
		memcpy(data->buf, (char[]){0x05, 0x17, 0x08, 0x08, 0x00, temp}, 6 * sizeof data->buf[0]);
		ret = send_msg(data);
		if(ret)
			return ret;
	return count;
}

static DEVICE_ATTR_RW(custom_profile);
static DEVICE_ATTR_RW(led_mode);
static DEVICE_ATTR_RW(led_colors);
static DEVICE_ATTR_RW(led_temp_profile);
static DEVICE_ATTR_RW(led_ext_temp);

static struct attribute *corsair_hydro_attributes[] = {
	&dev_attr_custom_profile.attr,
	&dev_attr_led_mode.attr,
	&dev_attr_led_colors.attr,
	&dev_attr_led_temp_profile.attr,
	&dev_attr_led_ext_temp.attr,
	NULL
};

static const struct attribute_group corsair_hydro_attribute_group = {
	.attrs = corsair_hydro_attributes
};

static ssize_t show_temp_input(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret, i;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *dev_attr = to_sensor_dev_attr(attr);
	unsigned temp = 0;
	memcpy(data->buf, (char[]){ 0x07, 0x06, 0x06, 0x0c, dev_attr->index, 0x07, 0x09, 0x0e }, 8 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret)
		return ret;
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x06:
				if(data->buf[i] != 0x06) {
					dev_err(&data->usbdev->dev, "Invalid HID Request ID(got id %d)\n", data->buf[i]);
					return -EINVAL;
				}
				break;
			case 0x07:
				temp = ((((unsigned char)data->buf[i + 2] << 8) + (unsigned char)data->buf[i + 1]) * 1000) / 256;
				i += 2;
				break;
		}
	}
	return sprintf(buf, "%u\n", temp);
}

static ssize_t show_fan_input(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret, i;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *dev_attr = to_sensor_dev_attr(attr);
	unsigned rpm = 0;
	memcpy(data->buf, (char[]){ 0x07, 0x08, 0x06, 0x10, dev_attr->index, 0x09, 0x09, 0x16 }, 8 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret) {
		return ret;
	}
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x08:
				if(data->buf[i] != 0x06) {
					dev_err(&data->usbdev->dev, "Invalid HID Request ID(got id %d)\n", data->buf[i]);
					return -EINVAL;
				}
				break;
			case 0x09:
				rpm = ((unsigned char)data->buf[i + 2] << 8) + (unsigned char)data->buf[i + 1];
				i += 2;
				break;
		}
	}
	return sprintf(buf, "%u\n", rpm);
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret, i;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *dev_attr = to_sensor_dev_attr(attr);
	unsigned pwm = 0;
	memcpy(data->buf, (char[]){ 0x07, 0x08, 0x06, 0x10, dev_attr->index, 0x0a, 0x07, 0x13 }, 8 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret) {
		return ret;
	}
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x08:
				if(data->buf[i] != 0x06) {
					dev_err(&data->usbdev->dev, "Invalid HID Request ID(got id %d)\n", data->buf[i]);
					return -EINVAL;
				}
				break;
			case 0x0a:
				pwm = (unsigned char)data->buf[++i];
				break;
		}
	}
	return sprintf(buf, "%u\n", pwm);
}

static ssize_t store_pwm(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret, i;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *dev_attr = to_sensor_dev_attr(attr);
	unsigned pwm = 0;
	
	if(kstrtouint(buf, 10, &pwm) || pwm > 255)
		return -EINVAL;
	memcpy(data->buf, (char[]){ 0x08, 0x08, 0x06, 0x10, dev_attr->index, 0x0b, 0x06, 0x13, pwm }, 9 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret) {
		return ret;
	}
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x08:
			case 0x0b:
				if(data->buf[i] != 0x06) {
					dev_err(&data->usbdev->dev, "Invalid HID Request ID(got id %d)\n", data->buf[i]);
					return -EINVAL;
				}
				break;
		}
	}
	return count;
}

static ssize_t show_pwm_enable(struct device *dev, struct device_attribute *attr, char *buf) {
	int ret, i;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *dev_attr = to_sensor_dev_attr(attr);
	unsigned pwm_mode = 0;
	memcpy(data->buf, (char[]){ 0x07, 0x08, 0x06, 0x10, dev_attr->index, 0x0c, 0x07, 0x12 }, 8 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret) {
		return ret;
	}
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x08:
				if(data->buf[i] != 0x06) {
					dev_err(&data->usbdev->dev, "Invalid HID Request ID(got id %d)\n", data->buf[i]);
					return -EINVAL;
				}
				break;
			case 0x0c:
				pwm_mode = (data->buf[++i] & 0x0f) - 1;
				break;
		}
	}
	switch(pwm_mode) {
		case 0x02:
		case 0x04:
			pwm_mode = 1;
			break;
		case 0x06:
			pwm_mode = 2;
			break;
		case 0x08:
			pwm_mode = 3;
			break;
		case 0x0a:
			pwm_mode = 4;
			break;
		case 0x0c:
			pwm_mode = 5;
			break;
		case 0x0e:
			pwm_mode = 6;
			break;
	}
	return sprintf(buf, "%u\n", pwm_mode);
}

static ssize_t store_pwm_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
	int ret, i;
	struct corsair_hydro_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute *dev_attr = to_sensor_dev_attr(attr);
	unsigned pwm_mode = 0;
	
	if(kstrtouint(buf, 10, &pwm_mode) || pwm_mode > 255)
		return -EINVAL;
	switch(pwm_mode) {
		case 1:
			pwm_mode = 0x02;
			break;
		case 2:
			pwm_mode = 0x06;
			break;
		case 3:
			pwm_mode = 0x08;
			break;
		case 4:
			pwm_mode = 0x0a;
			break;
		case 5:
			pwm_mode = 0x0c;
			break;
		case 6:
			pwm_mode = 0x0e;
			break;
	}
	memcpy(data->buf, (char[]){ 0x08, 0x08, 0x06, 0x10, dev_attr->index, 0x0d, 0x06, 0x12, pwm_mode }, 9 * sizeof data->buf[0]);
	ret = send_msg(data);
	if(ret) {
		return ret;
	}
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x08:
			case 0x0d:
				if(data->buf[i] != 0x06) {
					dev_err(&data->usbdev->dev, "Invalid HID Request ID(got id %d)\n", data->buf[i]);
					return -EINVAL;
				}
				break;
		}
	}
	return count;
}

static int corsair_probe(struct hid_device *dev, const struct hid_device_id *id) {
	int ret, version[3] = {0}, dev_id = 0, i, temp_sensors = 0, fans = 0, leds = 0;
	char name[8];
	struct usb_interface *usbif = to_usb_interface(dev->dev.parent);
	struct corsair_hydro_data *data;
	struct attribute_group* corsair_hydro_group;

	ret = hid_parse(dev);
	if (ret != 0) {
		hid_err(dev, "parse failed\n");
		return ret;
	}
	ret = hid_hw_start(dev, HID_CONNECT_HIDRAW);
	if (ret != 0) {
		hid_err(dev, "hw start failed\n");
		return ret;
	}

	data = devm_kzalloc(&dev->dev, sizeof(*data), GFP_KERNEL);
	data->usbdev = interface_to_usbdev(usbif);
	data->buf = devm_kzalloc(&dev->dev, 64, GFP_KERNEL);
	memcpy(data->buf, (char[]){ 0x03, 0x01, 0x07, 0x00}, 4 * sizeof data->buf[0]);
	send_msg(data);
	for(i = 0; i < ARRAY_SIZE(corsair_hydro_device_ids); i++) {
		if(corsair_hydro_device_ids[i] == data->buf[2]) {
			dev_id = data->buf[2];
		}
	}
	if(!dev_id) {
		return -EINVAL;
	}

	memcpy(data->buf, (char[]){ 0x10, 0x02, 0x07, 0x01, 0x03, 0x0B, 0x02, 0x08, 0x04, 0x07, 0x0d, 0x05, 0x07, 0x11, 0x06, 0x07, 0x05 }, 17 * sizeof data->buf[0]);
	send_msg(data);
	for(i = 0; data->buf[i]; i++) {
		switch(data->buf[i++]) {
			case 0x02:
				version[2] = data->buf[++i];
				version[0] = data->buf[++i] / 0x0F;
				version[1] = data->buf[i] % 0x10;
				break;
			case 0x03:
				i++;
				memcpy(name, &data->buf[++i], 8 * sizeof(char));
				i += 7;
				break;
			case 0x04:
				temp_sensors = data->buf[++i];
				printk(KERN_INFO "corsair_hydro: Found %d Temperature Sensors", temp_sensors);
				break;
			case 0x05:
				fans = data->buf[++i];
				printk(KERN_INFO "corsair_hydro: Found %d Fans", fans);
				break;
			case 0x06:
				leds = data->buf[++i];
				printk(KERN_INFO "corsair_hydro: Found %d LEDs", leds);
				break;
		}
	}

	corsair_hydro_group = devm_kzalloc(&dev->dev, sizeof(struct attribute_group), GFP_KERNEL);
	corsair_hydro_group->attrs = devm_kzalloc(&dev->dev, sizeof(struct attribute*) * (temp_sensors + (3 * fans) + 1), GFP_KERNEL);
	data->corsair_hydro_sensor_attributes = devm_kzalloc(&dev->dev, sizeof(struct sensor_device_attribute) * (temp_sensors + (3 * fans)), GFP_KERNEL);
	for(i = 0; i < temp_sensors; i++) {
		struct sensor_device_attribute sensor_dev_attr_template;
		char* label = devm_kzalloc(&dev->dev, sizeof(char[12]), GFP_KERNEL);
		sprintf(label, "temp%d_input", i + 1);
		sensor_dev_attr_template = (struct sensor_device_attribute) {
			.dev_attr = {
				.attr = {
					.name = label,
					.mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO)
				},
				.show = show_temp_input,
				.store = NULL
			},
			.index = i
		};
		data->corsair_hydro_sensor_attributes[i] = sensor_dev_attr_template;
		corsair_hydro_group->attrs[i] = &data->corsair_hydro_sensor_attributes[i].dev_attr.attr;
	}
	for(i = 0; i < fans; i++) {
		struct sensor_device_attribute sensor_dev_attr_template;
		char* label = devm_kzalloc(&dev->dev, sizeof(char[11]), GFP_KERNEL);
		sprintf(label, "fan%d_input", i + 1);
		sensor_dev_attr_template = (struct sensor_device_attribute) {
			.dev_attr = {
				.attr = {
					.name = label,
					.mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO)
				},
				.show = show_fan_input,
				.store = NULL
			},
			.index = i
		};
		data->corsair_hydro_sensor_attributes[i + temp_sensors] = sensor_dev_attr_template;
		corsair_hydro_group->attrs[i + temp_sensors] = &data->corsair_hydro_sensor_attributes[i + temp_sensors].dev_attr.attr;
	}
	for(i = 0; i < fans; i++) {
		struct sensor_device_attribute sensor_dev_attr_template;
		char* label = devm_kzalloc(&dev->dev, sizeof(char[5]), GFP_KERNEL);
		sprintf(label, "pwm%d", i + 1);
		sensor_dev_attr_template = (struct sensor_device_attribute) {
			.dev_attr = {
				.attr = {
					.name = label,
					.mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR)
				},
				.show = show_pwm,
				.store = store_pwm
			},
			.index = i
		};
		data->corsair_hydro_sensor_attributes[i + (temp_sensors + fans)] = sensor_dev_attr_template;
		corsair_hydro_group->attrs[i + (temp_sensors + fans)] = &data->corsair_hydro_sensor_attributes[i + (temp_sensors + fans)].dev_attr.attr;
	}
	for(i = 0; i < fans; i++) {
		struct sensor_device_attribute sensor_dev_attr_template;
		char* label = devm_kzalloc(&dev->dev, sizeof(char[12]), GFP_KERNEL);
		sprintf(label, "pwm%d_enable", i + 1);
		sensor_dev_attr_template = (struct sensor_device_attribute) {
			.dev_attr = {
				.attr = {
					.name = label,
					.mode = VERIFY_OCTAL_PERMISSIONS(S_IRUGO|S_IWUSR)
				},
				.show = show_pwm_enable,
				.store = store_pwm_enable
			},
			.index = i
		};
		data->corsair_hydro_sensor_attributes[i + (temp_sensors + 2 * fans)] = sensor_dev_attr_template;
		corsair_hydro_group->attrs[i + (temp_sensors + 2 * fans)] = &data->corsair_hydro_sensor_attributes[i + (temp_sensors + 2 * fans)].dev_attr.attr;
	}
	data->corsair_hydro_sensor_groups[0] = corsair_hydro_group;
	devm_hwmon_device_register_with_groups(&dev->dev, "corsair_hydro", data, data->corsair_hydro_sensor_groups);

	dev_set_drvdata(&dev->dev, data);
	ret = sysfs_create_group(&dev->dev.kobj, &corsair_hydro_attribute_group);
	if(ret != 0) {
		hid_err(dev, "sysfs registration failed\n");
		return ret;
	}

	printk(KERN_INFO "HID: Started Corsair Watercooler %s (Dev ID 0x%x) with Firmware %d.%d.%d\n", name, dev_id, version[0], version[1], version[2]);
	return 0;
}

static void corsair_remove(struct hid_device *dev) {
	sysfs_remove_group(&dev->dev.kobj, &corsair_hydro_attribute_group);
	hid_hw_stop(dev);
}

static const struct hid_device_id corsair_hydro_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_CORSAIR, USB_DEVICE_ID_CORSAIR_HYDRO) },
	{}
};

MODULE_DEVICE_TABLE(hid, corsair_hydro_devices);

static struct hid_driver corsair_driver = {
	.name = "corsair_hydro",
	.id_table = corsair_hydro_devices,
	.probe = corsair_probe,
	.remove = corsair_remove,
};

module_hid_driver(corsair_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas Kahnert");
MODULE_DESCRIPTION("HID driver for Corsair Watercooling devices");
