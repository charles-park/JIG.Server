# JIG.Server
#### ⚠️ **Note : `All settings must be performed as root.`**
## List of configurable boards (Detailed settings)
| MODEL     | DATE    | SERVER BOARD                                  | SERVER-LCD      |
|:---------:|:-------:|:---------------------------------------------:|:---------------:|
| JIG_C4_C5 | 2025.01 | [ODROID-C5 (2025.06)](docs/server.c4_c5.md)   | VU12 (1920x720) |
| JIG_M1    | 2025.07 | [ODROID-C5 (2025.09)](docs/server.m1.md)      | VU12 (1920x720) |

## Reference Documents
* [Protocol](https://docs.google.com/spreadsheets/d/1F-HGwMx3569bgrLBSw_cTt5DpECXvL3MvAWMEkRWaL4/edit?gid=0#gid=0)
* [GPIO Map](https://docs.google.com/spreadsheets/d/18cRWfgj9xmlr1JQb91fNN7SQxrBZxkHoxOEJN6Yy4SI/edit?gid=0#gid=0)
* [MAC Address info](https://docs.google.com/spreadsheets/d/1vIC5tHQ0rEVEjXHcP8fZPeVYMpLD3hWRf8UIYwtjrpw/edit?gid=0#gid=0)

## Required packages
  * System upgrade
```
apt update && apt upgrade -y
apt update --fix-missing
```
  * Common packages
```
apt install build-essential vim ssh git python3 python3-pip ethtool net-tools usbutils i2c-tools overlayroot nmap evtest htop cups cups-bsd iperf3 alsa samba lirc evtest minicom
```
  
  * Ubuntu version (>= 24.xx) : python3 package install
```
apt install python3-aiohttp python3-async-timeout
```

  * Ubuntu version ( < 24.xx) : python3 package install
```
pip install aiohttp asyncio
```

## Clone the reopsitory with submodule
```
root@odroid:~# git clone --recursive https://github.com/charles-park/JIG.m2.self

or

root@odroid:~# git clone https://github.com/charles-park/JIG.Client
root@odroid:~# cd JIG.Client
root@odroid:~/JIG.Client# git submodule update --init --recursive

// app build
root@odroid:~/JIG.Client# make clean && make

// app install
root@odroid:~/JIG.Client# cd service
root@odroid:~/JIG.Client/service# ./install.sh

```

### SSH root login
```
root@server:~# passwd root
New password: 
Retype new password: 
passwd: password updated successfully


root@server:~# vi /etc/ssh/sshd_config
...
#LoginGraceTime 2m

PermitRootLogin yes
StrictModes yes

#MaxAuthTries 6
#MaxSessions 10

PubkeyAuthentication yes

# Expect .ssh/authorized_keys2 to be disregarded by default in future.
#AuthorizedKeysFile     .ssh/authorized_keys .ssh/authorized_keys2
...

root@server:~# service sshd restart
```

### Auto login
```
root@server:~# systemctl edit getty@tty1.service
```
```
[Service]
ExecStart=
ExecStart=-/sbin/agetty --noissue --autologin root %I $TERM
Type=idle
```
* edit tool save
  save exit [Ctrl+ k, Ctrl + q]

### Disable screen off
```
root@server:~# vi ~/.bashrc
...
setterm -blank 0 -powerdown 0 -powersave off 2>/dev/null
echo 0 > /sys/class/graphics/fb0/blank
...
```

### Static ip settings (For Debugging)
```
root@server:~# vi /etc/netplan/01-netcfg.yaml
```
```
network:
    version: 2
    renderer: networkd
    ethernets:
        eth0:
            dhcp4: no
            # static ip address
            addresses:
                - 192.168.20.162/24
            gateway4: 192.168.20.1
            nameservers:
              addresses: [8.8.8.8,168.126.63.1]

```
```
root@server:~# netplan apply
root@server:~# ifconfig
```

### Samba config
```
root@server:~# smbpasswd -a root
root@server:~# vi /etc/samba/smb.conf
```
```
[odroid]
   comment = odroid client root
   path = /root
   guest ok = no
   browseable = no
   writable = yes
   create mask = 0755
   directory mask = 0755
```
```
root@server:~# service smbd restart
```

### Overlay root
* overlayroot enable
```
root@server:~# update-initramfs -c -k $(uname -r)
update-initramfs: Generating /boot/initrd.img-4.9.337-17

root@server:~# mkimage -A arm64 -O linux -T ramdisk -C none -a 0 -e 0 -n uInitrd -d /boot/initrd.img-$(uname -r) /boot/uInitrd 
Image Name:   uInitrd
Created:      Fri Oct 27 04:27:58 2023
Image Type:   AArch64 Linux RAMDisk Image (uncompressed)
Data Size:    7805996 Bytes = 7623.04 KiB = 7.44 MiB
Load Address: 00000000
Entry Point:  00000000

// Change overlayroot value "" to "tmpfs" for overlayroot enable
root@server:~# vi /etc/overlayroot.conf
...
overlayroot_cfgdisk="disabled"

# root partition only overlay fs
# overlayroot="tmpfs:recurse=0"

# All partition overlay fs
overlayroot="tmpfs"
```
* overlayroot disable
```
// get write permission
root@server:~# overlayroot-chroot 
INFO: Chrooting into [/media/root-ro]
root@server:~# 

// Change overlayroot value "tmpfs" to "" for overlayroot disable
root@server:~# vi /etc/overlayroot.conf
...
overlayroot_cfgdisk="disabled"
overlayroot=""
```
