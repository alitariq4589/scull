# scull
Linux device driver for Simple Character Utility for Loading Localities (SCULL).

Use following command to build and load the driver (open a dmesg window alongside `dmesg -w` to see the output).

```
sudo apt install linux-headers-$(uname -r) build-essential # Install the kernel headers and gcc for compiling the code with kernel libraries
cd scull
make
sudo sh scull_load.sh # This will load the driver in kernel

sudo su # Change to root as /dev/scull0 cannot be accessed using other users (even sudo)

# Testing:

echo "Hello" > /dev/scull0
cat /dev/scull0 # This should print the string written above as scull is a memory device
```
