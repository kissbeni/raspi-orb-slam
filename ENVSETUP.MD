
## Downloading Raspberry Pi OS

Currently all the things here tested with Raspberry Pi OS Lite for armhf
architectrue, due to issues with libraries not being ported to arm64 yet, you
can get it from the official Raspberry Pi website's downloads page:
https://www.raspberrypi.com/software/operating-systems/

This will give us a `yyyy-mm-dd-raspios-xxxxx-armhf-lite.img` file,
I'll refer to it as `raspios.img` from now on.

## Creating the system SD card

You will need a preferably fast, at least 8GB mcroSD card to which you can write
the previously downloaded system image like this:

**If you run this command on the wrong device you WILL LOSE DATA!**

```bash
sudo dd if=raspios.img of=/dev/your-sdcard bs=64M && sudo sync
```

This creates a 1.5GB system partition, besides that you'll have a buch of free
space. Resize the second partition from the original 1.5GB to at least 4GB and
create a new partition on the remaining space with an ext4 filesystem, which
will be your `/home`. (Use your favourite partitioning tool, I used GParted)


Find out the PARTUUID for it, the simplest way to do it is to list
/dev/disk/by-uuid with `ls -l /dev/disk/by-uuid`, which outputs something like
this:

```
[user@host ~]$ ls -l /dev/disk/by-partuuid
total 0
lrwxrwxrwx 1 root root 15 Sep 23 14:58 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx -> ../../nvme0n1p2
lrwxrwxrwx 1 root root 15 Sep 23 14:58 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx -> ../../nvme0n1p3
lrwxrwxrwx 1 root root 15 Sep 23 14:58 xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx -> ../../nvme0n1p1
lrwxrwxrwx 1 root root 10 Sep 30 20:15 9730496b-01 -> ../../sda1
lrwxrwxrwx 1 root root 10 Sep 30 20:15 9730496b-02 -> ../../sda2
lrwxrwxrwx 1 root root 10 Sep 30 20:17 9730496b-03 -> ../../sda3

```

Then mount the second partition of the SD card to a directory of your choice:

```bash
sudo mount /dev/sdX2 /mnt
```

Then remove the home directory given in the image:

```bash
sudo rm -r /mnt/home/pi
```

Add the new mount point to `fstab`:

```bash
echo 'PARTUUID=9730496b-03 /home ext4 defaults,noatime 0 1' | sudo tee -a /mnt/etc/fstab
```

If you want to do some system configuration changes before your first boot
(like changing your ssh port), you can do them here.

Unmount `/mnt`
```
sudo umount /mnt
```

If you don't have a display cable to configure your Pi graphically, this is how
to set up headless ssh:

```bash
sudo mount /dev/sdX1 /mnt # Mount the first partition of the SD card
sudo touch /mnt/shh
sudo umount /mnt
```

If this doesn't work for you, as in my case it didn't, mount the second
partition again to `/mnt` then add `service ssh start` before the `exit 0` to
`/mnt/etc/rc.local`. When the Pi is booted enable the service with
`sudo systemctl enable sshd` and remove the line from `/etc/rc.local`.

## Hardware setup

Connect your camera to the CSI interface on the Pi (the connector is about in
the midle of the board).

| Pin Name      | Pin number (on header) | Connect to           |
|:--------------|:-----------------------|:---------------------|
| 5V Power      | 2 and/or 4             | 5V buck converter    |
| GND           | 6                      | Common ground        |
| GPIO 14 (TXD) | 8                      | Motor controller RXD |
| GPIO 14 (RXD) | 10                     | Motor controller TXD |
| GPIO 26       | 37                     | Fan MOSFET           |

If you are having trouble identifying which pin is where, there are many
resources available online, just search for it like:
*raspberry pi 4B gpio pinout*.

## Boot your Pi and find it on your network

The easiest way to find it is checking your DHCP server's leases or using
`nmap -sn` to scan your network. Then SSH into it using `ssh pi@your-raspi-ip`,
if you get an `Too many authentication failures` error, try
`ssh -o PubkeyAuthentication=no pi@your-raspi-ip`.

Create your home directory because it will be missing if you followed the
instructions in the begining you end up with an empty `/home` folder.

```bash
sudo mkdir /home/pi
sudo chown pi:pi /home/pi
```

Open `sudo raspi-config` then set your WLAN locale to your country in
Localisation options and enable the camera and the serial port under Interface
options. When asked do **NOT** enable login shell on the serial port because
that will mess up things. You can skip rebooting if you updating the system
anyway like described below.

It's a good practice to update the system first, before you start doing stuff.

```bash
sudo -i
apt update && apt upgrade && reboot
```

If you need a swapfile, [here's a guide setting it up]
(https://linuxize.com/post/create-a-linux-swap-file/). It is not mandatory but a
good thing if you want to compile with all 4 threads, because if you run out of
memory the kernel will start killing processes, thus you compiler as well.

While you download all this stuff I recommend staying on Ethernet but after you
got everything working set up WiFi access by adding your network(s) to
`/etc/wpa_supplicant/wpa_supplicant.conf`, each described like this:

```
network={
   ssid="My Awesome SSID"
   psk="My Awesome Password"
}
```
If you have hidden your SSID you will have to add `scan_ssid=1` as well.

*Rebooting might be needed to apply the new network settings.*

## Installing and building dependencies

Install dependencies:

```bash
sudo apt install gcc g++ clang make cmake git flex bison linux-tools raspberrypi-kernel-headers libeigen3-dev minicom libdw-dev libunwind-dev systemtap-sdt-dev libaudit-dev binutils-dev
```

This project requies OpenCV 3.x, Eigen3.1 and raspicam, you can
download the source and compile it like this:

```bash
cd ~
git clone https://github.com/opencv/opencv.git -b 3.4 opencv3
cd opencv3
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE ../
make -j4
sudo make install

cd ~
git clone https://gitlab.com/libeigen/eigen/ -b 3.1
cd eigen
mkdir build
cd build
cmake -D CMAKE_BUILD_TYPE=RELEASE ../
make -j4
sudo make install

cd ~
git clone https://github.com/cedricve/raspicam
cd raspicam
cmake .
make -j
sudo make install
```

## ORB_SLAM2 and raspi-orb-slam

Clone my repository recursively:

```bash
cd ~
git clone --recursive https://github.com/kissbeni/raspi-orb-slam
cd raspi-orb-slam
```

Build and install ORB_SLAM2:

```bash
cd ORB_SLAM2
./build.sh
sudo cp lib/libORB_SLAM2.so /usr/lib/
```

Building MiniJson:

```bash
cd ~/raspi-orb-slam
cd MiniJson
cmake .
make -j
```

Building raspi-orb-slam itself:

```bash
cd ~/raspi-orb-slam/
make
```

For running raspi-orb-slam there is a shell script that gives the necessarry parameters to the program called `start.sh`

## Profiling

If it is still broken on the current Raspberry Pi OS, perf will not work when you install the `linux-tools` package. You can get it by downloading the Raspberry Pi kernel and compiling perf from source (of course you'll need the same source version as your kernel version, which you can see by typing in `uname -r`).

```bash
cd ~
git clone https://github.com/raspberrypi/linux -b rpi-5.10.y raspikernel # replace rpi-5.10.y with the branch for your kernel
cd raspikernel/tools/perf
make
sudo cp perf /usr/bin/perf_5.10 # or whatever perf needs you to name it
```

To generate flame graph follow the instructions described in [this](https://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html) guide's 4.1 segment.
