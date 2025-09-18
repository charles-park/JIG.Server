# Server : JIG-M1 (2025-07)

* Document : [ODROID-M1 - ADC Board 연결도(2025)](https://docs.google.com/spreadsheets/d/1mUUWAhZeI7kd9SqFgP_7Fea8CZK7xyCqQDq9VtFoWFI/edit?gid=0#gid=0)
* Config   : [DEV Config](/configs/m1_server.c5.cfg), [UI Config](/configs/m1_ui.c5.cfg)
* Image PATH     : smb://odroidh3.local/sharedfolder/생산관리/jig/odroid-m1/2025.09.16-M1_JIG/
* Release Image  : m1_jig_server.Sep_10_2025.c5_emmc.img.xz
* Linux OS Image : ubuntu-22.04-factory-odroidc5-odroidc5-20250619.img.xz
```
root@server:~# uname -a
Linux server 5.15.153-odroid-arm64 #1 SMP PREEMPT Wed, 18 Jun 2025 08:31:13 +0000 aarch64 aarch64 aarch64 GNU/Linux
```
#### ⚠️ **Note : `The DDR clock must be verified as 1896Mhz in the bootloader`**

## Test items
```
  HDMI     : FB, EDID, HPD
  STORAGE  : eMMC, SATA, NVME
  USB      : USB3.0 x 2, USB2.0 x 2
  ETHERNET : IPERF(Server), IPERF(Client), MAC Write
  HEADER   : H40
  ADC      : Header40 - 37Pin, 40Pin
  LED      : Power(Red), Alive(Blue), Ethernet(Green/Orange), NVME(Green)
  AUDIO    : HP L/R, SPEAKER L/R
  IR       : IR Receive
  MISC     : SPI Button, HP Detect
```
