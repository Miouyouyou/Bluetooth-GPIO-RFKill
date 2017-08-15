WARNING
-------

** !! DO NOT USE THIS DRIVER IF YOU CAN'T READ THE CODE !! **

About
-----

A test driver to enable or disable the right GPIO pins of Bluetooth
chips, using the RFKill system, in order to use them with BlueZ
afterwards.

Preparation
-----------

This is not required, but you can export the following values to
influentitate the [Makefile](./Makefile).
```bash
export ARCH=arm
export CROSS_COMPILE=armv7a-hardfloat-linux-gnueabi-
export MYY_KERNEL_DIR=/path/to/your/arm/linux
```

Or you can edit the [Makefile](./Makefile).

Compilation
-----------

`make`

Clean
-----

`make clean`

