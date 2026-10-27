# Server : JIG-R1 (2026.10)

* Document : [ODROID-R1 JIG연결도(2026.10)](https://docs.google.com/spreadsheets/d/1uO0tWpi965akbPAEyKgAVrds5GoxAPgPy4x0gtpPmIw/edit?gid=0#gid=0)
* Config   : [DEV Config](/configs/r1_server.c5.cfg), [UI Config](/configs/r1_ui.c5.cfg)
* Image PATH     : smb://odroidh3.local/sharedfolder/생산관리/jig/odroid-r1/
* Release Image  : jig_r1_server_20261027.img
* Linux OS Image : ubuntu-22.04-factory-odroidc5-odroidc5-20250619.img.xz
```
root@server:~# uname -a
Linux server 5.15.153-odroid-arm64 #1 SMP PREEMPT Thu, 01 Oct 2026 11:53:24 +0000 aarch64 aarch64 aarch64 GNU/Linux
```
#### ⚠️ **Note : `The DDR clock must be verified as 1896Mhz in the bootloader`**

## Test items
```
  HDMI     : FB, EDID, HPD
  STORAGE  : eMMC, SATA, NVME
  USB      : USB3.0 x 2, USB2.0 x 2
  ETHERNET : IPERF(Server), IPERF(Client), MAC Write
  HEADER   : H40, H14
  ADC      : Header40 - 37Pin, 40Pin
  LED      : Power(Red), Alive(Blue), Ethernet(Green/Orange), NVME(Green)
  MISC     : FAN Speed
```
