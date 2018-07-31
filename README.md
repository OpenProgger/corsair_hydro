# Kernel Driver for Corsair H110i Devices
This Kernel driver supports the HID-Protocol of some Corsair Watercooler Devices like H110i.
It creates an hwmon device for the fans and temp sensors which were readable by lm_sensors.
It's still possible to use user-space tools for the hidraw interface (e. g. OpenCorsairLink).

It was only for personal use and the code is not in best state but feel free to bugreport or PR.

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

## External Sources
For the HID Protocoll the [OpenCorsairLink](https://github.com/audiohacked/OpenCorsairLink) and this [Corsair Forum Thread](http://forum.corsair.com/v3/showthread.php?t=120092)
 where the sources.
