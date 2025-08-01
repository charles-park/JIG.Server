//------------------------------------------------------------------------------
/**
 * @file setup.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID-C4 server config parse.
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
//------------------------------------------------------------------------------
// return 1 : find success, 0 : not found
//------------------------------------------------------------------------------
static int find_file_path (const char *fname, char *file_path)
{
    FILE *fp;
    char cmd_line[STR_PATH_LENGTH * 2];

    memset (cmd_line, 0, sizeof(cmd_line));
    sprintf(cmd_line, "%s\n", "pwd");

    if (NULL != (fp = popen(cmd_line, "r"))) {
        memset (cmd_line, 0, sizeof(cmd_line));
        fgets  (cmd_line, STR_PATH_LENGTH, fp);
        pclose (fp);

        strncpy (file_path, cmd_line, strlen(cmd_line)-1);

        memset (cmd_line, 0, sizeof(cmd_line));
        sprintf(cmd_line, "find -name %s\n", fname);
        if (NULL != (fp = popen(cmd_line, "r"))) {
            memset (cmd_line, 0, sizeof(cmd_line));
            fgets  (cmd_line, STR_PATH_LENGTH, fp);
            pclose (fp);
            if (strlen(cmd_line)) {
                strncpy (&file_path[strlen(file_path)], &cmd_line[1], strlen(cmd_line)-1);
                file_path[strlen(file_path)-1] = 0;
                return 1;
            }
            return 0;
        }
    }
    pclose(fp);
    return 0;
}

//------------------------------------------------------------------------------
static int find_ts_event (const char *f_str)
{
    FILE *fp;
    char cmd  [STR_PATH_LENGTH];
    int i = 0;

    /*
        find /dev/input -name event*        udevadm info -a -n /dev/input/event3 | grep 0705
    */
    while (1) {
        memset  (cmd, 0, sizeof(cmd));
        sprintf (cmd, "/dev/input/event%d", i);
        if (access  (cmd, F_OK))    return -1;

        memset  (cmd, 0, sizeof(cmd));
        sprintf (cmd, "udevadm info -a -n /dev/input/event%d | grep %s", i, f_str);
        if ((fp = popen(cmd, "r")) != NULL) {
            memset (cmd, 0x00, sizeof(cmd));
            while (fgets (cmd, sizeof(cmd), fp) != NULL) {
                if (strstr (cmd, f_str) != NULL) {
                    pclose (fp);
                    return i;
                }
            }
            pclose (fp);
        }
        i++;
    }
}

//------------------------------------------------------------------------------
static int find_uart_port (const char *path)
{
    FILE *fp;
    char rdata[256], *ptr;

    memset  (rdata, 0x00, sizeof(rdata));
    sprintf (rdata, "find %s -name ttyUSB* 2<&1", path);

    // UART (ttyUSB) find...
    if ((fp = popen(rdata, "r")) != NULL) {
        memset (rdata, 0x00, sizeof(rdata));
        while (fgets (rdata, sizeof(rdata), fp) != NULL) {

            if ((ptr = strstr (rdata, "ttyUSB")) != NULL) {
                char c_port = *(ptr +6);
                pclose(fp);
                return (c_port - '0');
            }
            memset (rdata, 0x00, sizeof(rdata));
        }
        pclose(fp);
    }
    return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void parse_S_cmd (server_t *p, char *cfg)
{
    char *tok;

    if (strtok (cfg, ",") != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL)
            strncpy (p->fb_path, tok, strlen(tok));

        if ((tok = strtok (NULL, ",")) != NULL)
            p->ch_cnt = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->usblp_mode = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            find_file_path (tok, p->ui_path);
    }
}

//------------------------------------------------------------------------------
static void parse_C_cmd (server_t *p, char *cfg)
{
    char *tok;
    int ch;

    if (strtok (cfg, ",") != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL)
            ch = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            strncpy (p->ch[ch].i2c_path, tok, strlen(tok));

        if ((tok = strtok (NULL, ",")) != NULL)
            strncpy (p->ch[ch].uart_path, tok, strlen(tok));

        if ((tok = strtok (NULL, ",")) != NULL)
            p->ch[ch].uart_baud = atoi (tok);
    }
}

//------------------------------------------------------------------------------
static void parse_U_cmd (server_t *p, char *cfg)
{
    char *tok;
    int cnt = 0;

    if (strtok (cfg, ",") != NULL) {
        while ((tok = strtok (NULL, ",")) != NULL)
            p->u_item[cnt++] = atoi (tok);
    }
}

//------------------------------------------------------------------------------
static void parse_P_cmd (server_t *p, char *cfg)
{
    char *tok;
    int ch = 0;

    if (strtok (cfg, ",") != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL)
            ch = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            strncpy (p->ch[ch].pw_item[p->ch[ch].pw_item_cnt].cname, tok, strlen(tok));

        if ((tok = strtok (NULL, ",")) != NULL)
            p->ch[ch].pw_item[p->ch[ch].pw_item_cnt].check_mV = atoi (tok);

        p->ch[ch].pw_item_cnt ++;
    }
}

//------------------------------------------------------------------------------
static void parse_H_cmd (server_t *p, char *cfg)
{
    char *tok;

    if (strtok (cfg, ",") != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL)
            p->h_item[p->h_item_cnt].did = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->h_item[p->h_item_cnt].pin = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->h_item[p->h_item_cnt].max = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->h_item[p->h_item_cnt].min = atoi (tok);

        p->h_item_cnt++;
    }
}

//------------------------------------------------------------------------------
static void parse_T_cmd (server_t *p, char *cfg)
{
    char *tok;

    if (strtok (cfg, ",") != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL)
            strncpy (p->ts_vid, tok, strlen(tok));

        if ((tok = strtok (NULL, ",")) != NULL)
            p->ts_reset_gpio = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->ts_reset_level = atoi (tok);
    }
}

//------------------------------------------------------------------------------
static void parse_D_cmd (server_t *p, char *cfg)
{
    char *tok;

    if (strtok (cfg, ",") != NULL) {
        if ((tok = strtok (NULL, ",")) != NULL)
            p->d_item[p->d_item_cnt].gid = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->d_item[p->d_item_cnt].did = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->d_item[p->d_item_cnt].uid_l = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->d_item[p->d_item_cnt].uid_r = atoi (tok);

        if ((tok = strtok (NULL, ",")) != NULL)
            p->d_item[p->d_item_cnt].is_str = atoi (tok);

        p->d_item_cnt ++;
    }
}

//------------------------------------------------------------------------------
static int channel_setup (channel_t *pch)
{
    char uart_path[STR_PATH_LENGTH];
    // i2c init
    pch->i2c_fd = adc_board_init (pch->i2c_path);

    memset (uart_path, 0, sizeof(uart_path));
    // find uart & protocol init
    sprintf (uart_path, "/dev/ttyUSB%d", find_uart_port(pch->uart_path));

    if ((pch->puart = uart_init (uart_path, pch->uart_baud)) != NULL) {
        if (ptc_grp_init (pch->puart, 1)) {
            if (!ptc_func_init (pch->puart, 0, SERIAL_RESP_SIZE, protocol_check, protocol_catch)) {
                printf ("%s : protocol install error.", __func__);
                exit(1);
            }
        }
        return 1;
    }
    pch->status = eSTATUS_ERR;
    printf ("%s : Error... Protocol not installed!\n", __func__);
    return 0;
}

//------------------------------------------------------------------------------
static int server_config (server_t *p, const char *cfg_fname)
{
    FILE *pfd;
    char buf[STR_PATH_LENGTH] = {0,};
    int check_cfg = 0;

    memset (buf, 0, sizeof(buf));

    if (!find_file_path (cfg_fname, buf)) {
        printf ("%s : %s file not found!\n", __func__, cfg_fname);
        return 0;
    }

    if ((pfd = fopen(buf, "r")) == NULL) {
        printf ("%s : %s file open error!\n", __func__, buf);
        return 0;
    }

    while (fgets(buf, sizeof(buf), pfd) != NULL) {

        if (buf[0] == '#' || buf[0] == '\n')  continue;

        if (!check_cfg) {
            if (strstr(buf, "ODROID-SERVER-CONFIG") != NULL)    check_cfg = 1;
            continue;
        }

        switch (buf[0]) {
            case 'S':   parse_S_cmd (p, buf);  break;
            case 'C':   parse_C_cmd (p, buf);  break;
            case 'U':   parse_U_cmd (p, buf);  break;
            case 'P':   parse_P_cmd (p, buf);  break;
            case 'T':   parse_T_cmd (p, buf);  break;
            case 'D':   parse_D_cmd (p, buf);  break;
            case 'H':   parse_H_cmd (p, buf);  break;
            default :
                break;
        }
    }
    fclose (pfd);

    return check_cfg;
}

//------------------------------------------------------------------------------
void ts_reinit (server_t *p)
{
    // find ts event...
    int event_no;
    char ts_event[STR_PATH_LENGTH];

    if (p->pts) {
        ts_deinit (p->pts); p->pts = NULL;
    }

    // Vu12 (222a:0001)
    if      ((p->pfb->w == 1920) && (p->pfb->h == 720))
        event_no = find_ts_event ("222a");
    // Vu7  (16b4:000?)
    else if ((p->pfb->w == 1920) && (p->pfb->h == 1080))
        event_no = find_ts_event ("16b4");
    // ??? display
    else
        event_no = find_ts_event (p->ts_vid);

    if (event_no != -1) {

        memset  (ts_event, 0, sizeof(ts_event));
        sprintf (ts_event, "/dev/input/event%d", event_no);
        p->pts = ts_init (ts_event);
        printf ("%s : ts_event path = %s\n", __func__, ts_event);

        // ts reset button define
        if (p->ts_reset_gpio != -1) {
            gpio_export    (p->ts_reset_gpio);
            gpio_direction (p->ts_reset_gpio, 0);   // input
            printf ("%s : ts reset button = %d\n", __func__, p->ts_reset_gpio);
        }
    }
}

//------------------------------------------------------------------------------
int server_setup (server_t *p, const char *cfg_fname)
{
    if (server_config (p, cfg_fname)) {
        if ((p->pfb = fb_init (p->fb_path)) == NULL)            exit(1);
        if ((p->pui = ui_init (p->pfb, p->ui_path)) == NULL)    exit(1);

        // touch init
        ts_reinit (p);

        // usb label printer setting
        p->usblp_status = usblp_config ();

        // left, right channel init
        {
            int i;
            for (i = 0; i < p->ch_cnt; i++)  channel_setup (&p->ch[i]);
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
