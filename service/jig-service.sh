#!/bin/bash

# device stable delay
# sleep 10 && sync

#--------------------------
# ODROID-C4 Server enable
#--------------------------
/usr/bin/sync && /root/JIG.Server/JIG.Server -g 479 > /dev/null 2>&1
