# Linux kernel drivers tutorial

Repository containing a collection of driver examples for the DE10-Nano development kit. I wrote this as a beginner myself so it is likely that I've made some mistakes here and there. If you find any, feel free to point them out and correct them!


## Examples

The repo contains 5 examples:

- `hello-mod`: Hello world kernel module. Prints to kernel when module is loaded and unloaded.
- `my-led-driver-mod`: Controls a single LED on the DE10-Nano. Provides a filesystem to read and write the state of the LED.
- `my-btn-driver-mod`: Runs an ISR whenever the button is pushed down. Also set up to run a work task and enable wakeup when the DE10-Nano is put to sleep.
- `my-imu-driver-mod`: Prints out the readings of the DE10-Nano accelerometer.
- `imu-regmap-mod`: Same as above, but uses the `regmap` API.