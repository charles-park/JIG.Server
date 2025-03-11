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
#include "server.h"

//------------------------------------------------------------------------------
static int iperf3_client_func (const char *server_ip)
{
    FILE *fp;
    char cmd_line [STR_PATH_LENGTH], *pstr = NULL;
    int speed;

    memset  (cmd_line, 0, sizeof(cmd_line));
    sprintf (cmd_line, "iperf3 -c %s -t 1", server_ip);

    if ((fp = popen(cmd_line, "r")) != NULL) {
        memset (cmd_line, 0, sizeof(cmd_line));
        while (fgets(cmd_line, sizeof(cmd_line), fp)) {
            if (strstr (cmd_line, "receiver") != NULL) {
                if ((pstr = strstr (cmd_line, "MBytes")) != NULL) {
                    while (*pstr != ' ')    pstr++;
                    speed = atoi (pstr);
                    pclose (fp);
                    return speed;
                }
            }
            memset (cmd_line, 0, sizeof(cmd_line));
        }
        pclose(fp);
    }
    return 0;
}

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
                for (i = 0; i < DEVICE_RESP_SIZE -2; i++) {
                    if ((*(ptr + i) != 0x20) && (*(ptr + i) != 0x00)) break;
                }

                for (pos = 0; i < DEVICE_RESP_SIZE -2; i++)
                    pdata->resp_s[pos++] = *(ptr + i);
            }
            pdata->resp_i = atoi(ptr);
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
int device_resp_check (server_t *p, int fd, parse_resp_data_t *pdata)
{
    /* Device request I2C ADC Check */
    switch (pdata->gid) {
        /* IR Thread running */
        case eGID_IR:
            return 0;
         case eGID_ETHERNET:
            if (pdata->did == 2) {  // iperf did
                int iperf_speed = 0;
                iperf_speed = iperf3_client_func (pdata->resp_s);
                printf ("%s : iperf = %d\n", __func__, iperf_speed);
                memset (pdata->resp_s, 0, sizeof(pdata->resp_s));
                sprintf(pdata->resp_s, "%d", iperf_speed);
            }
            break;
        case eGID_LED:
            {
                int value = 0, pin, i, adc_read;
                for (i = 0; i < 10; i++) {
                    adc_board_read (fd, pdata->resp_s, &adc_read, &pin);
                    if (value < adc_read)
                        value = adc_read;

                    usleep (10000);
                }
                printf ("%s : led value = %d\n", __func__, value);
                memset (pdata->resp_s, 0, sizeof(pdata->resp_s));
                sprintf(pdata->resp_s, "%d", value);
            }
            break;
        case eGID_HEADER:
#define HEADER_PIN_MAX  40
#define GPIO_LOW_mV     100
#define GPIO_HIGH_mV    3000
            {
                int header[HEADER_PIN_MAX +1];
                int pin, i, max_mv = GPIO_HIGH_mV, min_mv = GPIO_LOW_mV;

                // Header max min config
                for (i = 0; i < p->h_item_cnt; i++) {
                    if (p->h_item[i].pin)   continue;
                    if ((DEVICE_ID(pdata->did) == p->h_item[i].did)) {
                        max_mv = p->h_item[i].max;  min_mv = p->h_item[i].min;
                        break;
                    }
                }
                usleep (100 * 1000);    // gpio setup stable delay

                memset (header, 0, sizeof(header));
                //int adc_board_read (int fd, const char *h_name, int *read_value, int *cnt)
                adc_board_read (fd, pdata->resp_s, &header[0], &pin);

                // Header pin config
                for (i = 0; i < p->h_item_cnt; i++) {
                    if (!p->h_item[i].pin)  continue;
                    if ((DEVICE_ID(pdata->did) == p->h_item[i].did)) {
                        if      (header[p->h_item[i].pin -1] >= p->h_item[i].max)
                            header[p->h_item[i].pin -1] = max_mv;
                        else if (header[p->h_item[i].pin -1] <= p->h_item[i].min)
                            header[p->h_item[i].pin -1] = min_mv;
                    }
                }

                memset (pdata->resp_s, 0, sizeof(pdata->resp_s));
                for (i = 0, pin = 0; i < DEVICE_RESP_SIZE -2; i ++) {
                    pin = (i * 2);
                    if      ((header[pin] <= min_mv) && (header[pin + 1] <= min_mv))
                        pdata->resp_s[i] = '0';
                    else if ((header[pin] >= max_mv) && (header[pin + 1] <= min_mv))
                        pdata->resp_s[i] = '1';
                    else if ((header[pin] <= min_mv) && (header[pin + 1] >= max_mv))
                        pdata->resp_s[i] = '2';
                    else if ((header[pin] >= max_mv) && (header[pin + 1] >= max_mv))
                        pdata->resp_s[i] = '3';
                    else
                        pdata->resp_s[i] = '-';
                }
                printf ("%s : %s\n", __func__, pdata->resp_s);
            }
            break;
/* not implement */
        case eGID_PWM: case eGID_GPIO: case eGID_AUDIO:
        default:
            pdata->status_i = 0;
            break;
    }
    // device_data_check ok
    return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
