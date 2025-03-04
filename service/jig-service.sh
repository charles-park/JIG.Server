#!/bin/bash

# device stable delay
# sleep 10 && sync

#--------------------------
# ODROID-C4 Server enable
#--------------------------
/usr/bin/sync && /root/JIG.Server/JIG.Server -c server.c4.cfg > /dev/null 2>&1
