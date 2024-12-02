//------------------------------------------------------------------------------
/**
 * @file device_c4.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID-C4 Device resp control.
 * @version 2.0
 * @date 2024-11-25
 *
 * @package apt install iperf3, nmap, ethtool, usbutils, alsa-utils
 *
 * @copyright Copyright (c) 2022
 *
 */
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>
#include <pthread.h>

//------------------------------------------------------------------------------
#include "device_c4.h"

//------------------------------------------------------------------------------
int device_resp_parse (const char *resp_msg, parse_resp_data_t *pdata)
{
    int msg_size = (int)strlen(resp_msg);
    char *ptr, resp[SERIAL_RESP_SIZE+1];

    if ((msg_size != SERIAL_RESP_SIZE) && (msg_size != DEVICE_RESP_SIZE)) {
        printf ("%s : unknown resp size = %d, resp = %s\n", __func__, msg_size, resp_msg);
        return 0;
    }

    memset (resp,   0, sizeof(resp));
    memset (pdata,  0, sizeof(parse_resp_data_t));

    // copy org msg
    memcpy (resp, resp_msg, msg_size);

    if ((ptr = strtok (resp, ",")) != NULL) {
        if (msg_size == SERIAL_RESP_SIZE) {
            // cmd
            if ((ptr = strtok (NULL, ",")) != NULL) pdata->cmd = *ptr;
            // gid
            if ((ptr = strtok (NULL, ",")) != NULL) pdata->gid = atoi(ptr);
            // did
            if ((ptr = strtok (NULL, ",")) != NULL) pdata->did = atoi(ptr);

            ptr = strtok (NULL, ",");
        }

        // status
        if (ptr != NULL) {
            pdata->status_c =  *ptr;
            pdata->status_i = (*ptr == 'P') ? 1 : 0;
        }
        // resp str
        if ((ptr = strtok (NULL, ",")) != NULL) {
            {
                int i, pos;
                for (i = 0, pos = 0; i < DEVICE_RESP_SIZE -2; i++)
                {
                    if ((*(ptr + i) != 0x20) && (*(ptr + i) != 0x00))
                        pdata->resp_s[pos++] = *(ptr + i);
                }
            }
            pdata->resp_i = atoi(ptr);
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
