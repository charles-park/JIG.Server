#!/bin/bash

# device stable delay
# sleep 10 && sync

#--------------------------
# ODROID Server enable
#--------------------------
/usr/bin/sync && /root/JIG.Server/JIG.Server > /dev/null 2>&1
