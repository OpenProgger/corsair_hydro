# Kernel Driver for Corsair HID Devices
This Kernel driver supports the HID-Protocol of some Corsair Watercooler Devices like H110i.
It creates an hwmon device for the fans and temp sensors which were readable by lm_sensors.
It's still possible to use user-space tools for the hidraw interface (e. g. OpenCorsairLink).

This driver is still very experimental and could panic your system but for monitoring only it works solid(for me:)).
Contributions are welcome.

## Build and Install
```
 make
 make install
```

## Test
Once installed it should autostart when a matching device is detected but for manual loading just use
globally
```
modprobe corsair_hydro
```
or from source directory
```
insmod corsair_hydro.ko
```

## Usage
# Sensors
The count of Fans and temperature sensors were detected automatically, only leds are hardcoded to one.
The driver registers an hwmon device so after initialization you can see fan speed and temperature in lm_sensors.

# Set Fan Profile
```
echo 1 > /path/to/hwmon/of/device/pwm1_enable
```
Available Profiles:
1: Fixed PWM
2: Default
3: Quiet
4: Balanced
5: Performance
6: Custom

# Set PWM for Fan
```
echo 255 > /path/to/hwmon/of/device/pwm1
```
Valid values were from 0 - 255

# Set Custom Profile
A Profile is defined as array of 5 temperatures and 5 RPM values
They need to be submitted as pairs
```
echo 25 500 30 1000 33 1500 36 1900 40 2200 > /path/to/sysfs/of/device/custom_profile
```

# Set LED Mode
```
echo 0x00 > /path/to/sysfs/of/device/led_mode
```
Avalable Modes:
0x00: Static
0x40: 2-color cycle
0x80: 4-color cycle
0xc0: temperature mode(internal sensor)
0xc7: temperature mode(external sensor)

# Set LED Colors
LED Colors are defined as array of 4 values
```
echo 00ff00 ff0000 0000ff ffffff > /path/to/sysfs/of/device/led_colors
```

# Set Values for Temperatute LED Mode
A Profile is defined as array of 3 pairs
```
echo 25 00ff00 33 ffff00 40 ff0000 > /path/to/sysfs/of/device/led_temp_profile
```
If you use the external sensor, you can set a custom temperature
```
echo 30 > /path/to/sysfs/of/device/led_ext_temp
```
In this case it's possible to write a shell script which pipes the temperature from any other device(e.g. CPU) to set the LED Color

## External Sources
For the HID Protocoll the [OpenCorsairLink](https://github.com/audiohacked/OpenCorsairLink) and this [Corsair Forum Thread](http://forum.corsair.com/v3/showthread.php?t=120092)
 are the sources.
