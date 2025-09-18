# Server : JIG-C4_C5 (2025.01)

* Document : [ODROID-C5 - ADC Board 연결도(2025)](https://docs.google.com/spreadsheets/d/1DmyNXs4d5W-9Q2ZlV6k4eF86kqg6jsr3/edit?gid=346818897#gid=346818897)
* Client C5 Config : [DEV Config.C5](/configs/c4_c5_server.c5.cfg), [UI Config.C5](/configs/c4_c5_ui.c5.cfg)
* Client C4 Config : [DEV Config.C4](/configs/c4_c5_server.c4.cfg), [UI Config.C4](/configs/c4_c5_ui.c4.cfg)
* Image PATH     : smb://odroidh3.local/sharedfolder/생산관리/jig/odroid_c5_c4/2025.06.19_NewImage/
* Release Image  : jig-c4-c5.server-c5.Jun_20_2025.emmc.img
* Linux OS Image : ubuntu-22.04-factory-odroidc5-odroidc5-20250619.img.xz
```
root@server:~# uname -a
Linux server 5.15.153-odroid-arm64 #1 SMP PREEMPT Wed, 18 Jun 2025 08:31:13 +0000 aarch64 aarch64 aarch64 GNU/Linux
```
#### ⚠️ **Note : `The DDR clock must be verified as 1896Mhz in the bootloader`**

## Test items (ODROID-C5)
```
  HDMI     : FB, EDID, HPD
  STORAGE  : eMMC
  USB      : USB2.0 x 4, OTG
  ETHERNET : IPERF(Server), MAC Write
  HEADER   : H40
  ADC      : Header40 - 37Pin, 40Pin
  LED      : Power(Red), Alive(Blue), Ethernet(Green/Orange)
  IR       : IR Receive
```

## Test items (ODROID-C4)
```
  HDMI     : FB, EDID, HPD
  STORAGE  : eMMC
  USB      : USB3.0 x 4, OTG
  ETHERNET : IPERF(Server), MAC Write
  HEADER   : H40, H07
  ADC      : Header40 - 37Pin, 40Pin
  LED      : Power(Red), Alive(Blue), Ethernet(Green/Orange)
  IR       : IR Receive
  F/W      : USB F/W Write
```
