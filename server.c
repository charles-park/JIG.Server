//------------------------------------------------------------------------------
/**
 * @file client.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID-C4 JIG Client App.
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
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <linux/fb.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

//------------------------------------------------------------------------------
#include "server.h"

//------------------------------------------------------------------------------
// device_check.c
//------------------------------------------------------------------------------
extern int  device_resp_parse   (const char *resp, parse_resp_data_t *pdata);
extern int  device_resp_check   (server_t *p, int fd, parse_resp_data_t *pdata);

//------------------------------------------------------------------------------
static int  get_board_ip        (char *ip_addr);
static int  channel_power_status(channel_t *pch);
static void channel_ui_update   (server_t *p);
static void *thread_ui_func     (void *arg);
static int  find_ditem_uid      (server_t *p, int ui_id, int *pos);
static int  find_ditem_pos      (server_t *p, int gid, int did);
static void protocol_parse      (server_t *p, int nch);
static void ts_event_check      (server_t *p, int ui_id);

//------------------------------------------------------------------------------
volatile int SystemCheckReady = 0, RunningTime = DEFAULT_RUNING_TIME;
volatile int UIStatus = eSTATUS_WAIT;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_ui;
pthread_t thread_check;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int get_board_ip (char *ip_addr)
{
    int fd, retry_cnt = 100;
    struct ifreq ifr;

retry:
    usleep (100 * 1000);    // 100ms delay
    /* this entire function is almost copied from ethtool source code */
    /* Open control socket. */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        fprintf (stdout, "Cannot get control socket\n");
        if (retry_cnt--)    goto retry;
        return 0;
    }
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        fprintf (stdout, "SIOCGIFADDR ioctl Error!!\n");
        close(fd);
        if (retry_cnt--)    goto retry;
        return 0;
    }
    inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, ip_addr, sizeof(struct sockaddr));
    printf ("%s : ip_address = %s\n", __func__, ip_addr);

    return 1;
}

//------------------------------------------------------------------------------
static int channel_power_status (channel_t *pch)
{
    int pin, i, retry = 2;

retry:
    for (i = 0; i < pch->pw_item_cnt; i++) {
        pthread_mutex_lock   (&mutex);
        adc_board_read (pch->i2c_fd,
            pch->pw_item[i].cname, &pch->pw_item[i].read_mV, &pin);
        pthread_mutex_unlock (&mutex);
        if (pch->pw_item[i].read_mV < pch->pw_item[i].check_mV)   {
            if (retry--) { usleep (FUNC_LOOP_DELAY);   goto retry;  }

            pch->ready = 0;
            return 0;
        }
    }
    return 1;
}
//------------------------------------------------------------------------------
static void channel_ui_update (server_t *p)
{
    channel_t *pch;
    int nch, uid;
    static int onoff = 0;

    onoff = !onoff;
    for (nch = 0; nch < p->ch_cnt; nch ++) {
        pch = &p->ch[nch];
        uid = nch ? p->u_item[eUID_STATUS_R] : p->u_item[eUID_STATUS_L];

        /* system i2c, uart error check */
        if ((pch->i2c_fd == -1) || (pch->puart == NULL)) {
            char err_item[STR_NAME_LENGTH];
            int cnt = 0;

            memset (err_item, 0, sizeof(err_item));
            cnt = sprintf (err_item, "%s", "E: ");
            if (pch->i2c_fd == -1)
                cnt += sprintf (&err_item[cnt], "%s", "I2C ");
            if (pch->puart == NULL)
                cnt += sprintf (&err_item[cnt], "%s", "UART(S) ");

            ui_set_ritem (p->pfb, p->pui, uid, onoff ? COLOR_RED : p->pui->bc.uint, -1);
            ui_set_sitem (p->pfb, p->pui, uid, -1, -1, err_item);
            pch->status = eSTATUS_ERR;
            continue;
        }

        if (channel_power_status (pch)) {
            // channel power ui
            ui_set_ritem (p->pfb, p->pui,
                nch ? p->u_item[eUID_CH_R] : p->u_item[eUID_CH_L],
                COLOR_GREEN, -1);
            if (pch->status == eSTATUS_STOP)
                pch->status = eSTATUS_WAIT;
        }
        else {
            // channel power ui
            ui_set_ritem (p->pfb, p->pui,
                nch ? p->u_item[eUID_CH_R] : p->u_item[eUID_CH_L],
                COLOR_DIM_GRAY, -1);

            if (pch->status != eSTATUS_STOP) {
                ui_set_ritem (p->pfb, p->pui, uid, p->pui->bc.uint, -1);
                ui_set_sitem (p->pfb, p->pui, uid, -1, -1, "WAIT");
            }
            pch->status = eSTATUS_STOP;
        }

        switch (pch->status) {
            case eSTATUS_STOP:
                break;
            case eSTATUS_WAIT:
                pch->status = eSTATUS_RUN;
                pch->err_cnt = 0;
                pch->ready_wait = UART_WAIT_TIME;
                ui_update_group (p->pfb, p->pui, nch +1);
                ui_set_sitem (p->pfb, p->pui, uid, -1, -1, "WAIT");
                break;
            case eSTATUS_RUN:
                if (!pch->ready)    pch->ready_wait--;
                if (pch->ready_wait) {
                    ui_set_ritem (p->pfb, p->pui, uid,
                        onoff ? RUN_BOX_ON : RUN_BOX_OFF, -1);
                    ui_set_sitem (p->pfb, p->pui, uid, -1, -1, "RUNNING");
                } else {
                    pch->status = eSTATUS_ERR;
                    if (p->usblp_status)
                        usblp_print_err ("uart", "", "", nch);
                }
                break;
            case eSTATUS_PRINT:
                ui_set_ritem (p->pfb, p->pui, uid,
                            pch->err_cnt ? COLOR_RED : COLOR_GREEN, -1);
                ui_set_sitem (p->pfb, p->pui, uid, -1, -1, "FINISH");
                break;
            case eSTATUS_ERR:
                ui_set_ritem (p->pfb, p->pui, uid, onoff ? COLOR_RED : p->pui->bc.uint, -1);
                ui_set_sitem (p->pfb, p->pui, uid, -1, -1, "E: UART(C)");
                break;
        }
    }
}

//------------------------------------------------------------------------------
static void *thread_ui_func (void *arg)
{
    static int onoff = 0;
    server_t *p = (server_t *)arg;

    memset (p->ip_addr, 0, sizeof(p->ip_addr));
    get_board_ip(p->ip_addr);

    while (1) {
        onoff = !onoff;
        ui_set_ritem (p->pfb, p->pui, p->u_item[eUID_ALIVE],
                    onoff ? COLOR_GREEN : p->pui->bc.uint, -1);
        ui_set_sitem (p->pfb, p->pui, p->u_item[eUID_ALIVE],
                    -1, -1, onoff ? p->pui->b_item[0].s_dfl : __DATE__);

        ui_set_sitem (p->pfb, p->pui, p->u_item[eUID_IPADDR], -1, -1, p->ip_addr);

        if (onoff)  ui_update (p->pfb, p->pui, -1);

        channel_ui_update (p);
        {
            if (p->ts_reset_gpio != -1) {
                int bt_status = 0;
                if (gpio_get_value(p->ts_reset_gpio, &bt_status))
                if (bt_status == p->ts_reset_level)  {
                    ui_set_ritem (p->pfb, p->pui, p->u_item[eUID_ALIVE],
                                onoff ? COLOR_PINK : p->pui->bc.uint, -1);

                    ts_reinit (p);
                }
            }
            ui_set_ritem (p->pfb, p->pui, p->u_item[eUID_USBLP],
                p->usblp_status ? COLOR_GREEN : COLOR_DIM_GRAY, -1);
        }
        usleep (UPDATE_UI_DELAY);
    }
    return arg;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int find_ditem_uid (server_t *p, int ui_id, int *pos)
{
    int i = 0;
    for (i = 0; i < p->d_item_cnt; i++) {
        *pos = i;
        if (p->d_item[i].uid_l == ui_id)    return 0;
        if (p->d_item[i].uid_r == ui_id)    return 1;
    }
    return -1;
}

//------------------------------------------------------------------------------
static int find_ditem_pos (server_t *p, int gid, int did)
{
    int i;

    for (i = 0; i < p->d_item_cnt; i++) {
        if ((p->d_item[i].gid == gid) && (p->d_item[i].did == did))
            return i;
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void protocol_parse (server_t *p, int nch)
{
    parse_resp_data_t pitem;
    channel_t *pch = &p->ch[nch];

    char *rx_msg = (char *)pch->rx_msg;
    char serial_resp[SERIAL_RESP_SIZE], resp [DEVICE_RESP_SIZE];

    if (!device_resp_parse (rx_msg, &pitem))    return;

    if (pch->status == eSTATUS_ERR)             return;

    switch (pitem.cmd) {
        /* Device Ready received */
        case 'R':
            /* Server System Ready send */
            SERIAL_RESP_FORM(serial_resp, 'O', -1, -1, NULL);
            ui_update_group (p->pfb, p->pui, nch +1);

            memset (pch->err_msg, 0, sizeof(pch->err_msg));
            pch->err_cnt = 0;
            if (pch->ready_wait) {
                pch->ready = 1;
                pch->status  = eSTATUS_RUN;
            }
            break;
        /* Device status received */
        case 'S':
            {
                int pos = find_ditem_pos (p, pitem.gid, pitem.did);
                int uid = nch ? p->d_item[pos].uid_r : p->d_item[pos].uid_l;

                if (p->d_item[pos].is_str)
                    ui_set_sitem (p->pfb, p->pui, uid, -1, -1, pitem.resp_s);

                if (pitem.status_c != 'C') {
                    ui_set_ritem (p->pfb, p->pui, uid,
                                (pitem.status_i == 1) ? COLOR_GREEN : COLOR_RED, -1);
                } else {
                    ui_set_ritem (p->pfb, p->pui, uid, COLOR_YELLOW, -1);

                    pthread_mutex_lock   (&mutex);
                    device_resp_check (p, pch->i2c_fd, &pitem);
                    pthread_mutex_unlock (&mutex);
                }
            }
            DEVICE_RESP_FORM_STR(resp, pitem.status_c, pitem.resp_s);
            SERIAL_RESP_FORM(serial_resp, (pitem.cmd == 'S') ? 'A' : 'C',
                                            pitem.gid, pitem.did, resp);
            break;
        case 'M':   // mac print
            memset  (pch->mac, 0, DEVICE_RESP_SIZE);
            strncpy (pch->mac, pitem.resp_s, strlen(pitem.resp_s));
            printf ("%s : MAC Addr = %s\n", __func__, pch->mac);
            if (p->usblp_status && (pch->status == eSTATUS_RUN))
                usblp_print_mac (pch->mac, nch);
            return;
        case 'E':   // error msg
            memset  (&pch->err_msg [pch->err_cnt][0], 0, USBLP_MAX_CHAR);
            strncpy (&pch->err_msg [pch->err_cnt][0], pitem.resp_s, strlen(pitem.resp_s));
            pch->err_cnt++;
            printf ("%s : Err Msg(%i) = %s\n", __func__,  pitem.status_i, pitem.resp_s);
            return;
        case 'X':   // Device test complete
            pch->status = eSTATUS_PRINT;
            return;
        default :
            printf ("%s : unknown command!! (%c)\n", __func__, pitem.cmd);
            return;
    }
    protocol_msg_tx (pch->puart, serial_resp);    protocol_msg_tx (pch->puart, "\r\n");
}

//------------------------------------------------------------------------------
static void ts_event_check (server_t *p, int ui_id)
{
    char serial_resp [SERIAL_RESP_SIZE];
    int pos, nch;
    channel_t *pch;

    memset (serial_resp, 0, sizeof(serial_resp));

    if ((ui_id == p->u_item[eUID_STATUS_L]) || (ui_id == p->u_item[eUID_STATUS_R])) {
        pch = (ui_id == p->u_item[eUID_STATUS_L]) ? &p->ch[0] : &p->ch[1];

        if (!pch->ready)    return;

        if (pch->status != eSTATUS_RUN) {
            SERIAL_RESP_FORM(serial_resp, 'E', -1, -1, NULL);
            protocol_msg_tx (pch->puart, serial_resp);
            protocol_msg_tx (pch->puart, "\r\n");
            pch->err_cnt = 0;
        } else {
            SERIAL_RESP_FORM(serial_resp, 'X', -1, -1, NULL);
            protocol_msg_tx (pch->puart, serial_resp);
            protocol_msg_tx (pch->puart, "\r\n");
            pch->err_cnt = 0;
        }
        return;
    }

    if ((ui_id == p->u_item[eUID_CH_L]) || (ui_id == p->u_item[eUID_CH_R])) {
        pch = (ui_id == p->u_item[eUID_CH_L]) ? &p->ch[0] : &p->ch[1];
        if (pch->status != eSTATUS_RUN) {
            if (pch->err_cnt) {
                int i;
                for (i = 0; i < pch->err_cnt; i += 3)
                    usblp_print_err (&pch->err_msg[i + 0][0],
                                     &pch->err_msg[i + 1][0],
                                     &pch->err_msg[i + 2][0],
                                    (ui_id == p->u_item[eUID_CH_L]) ? 0 : 1);
                // Print Err msg L/R
                printf ("%s : error msg printing... (ch = %d)\n",
                    __func__, (ui_id == p->u_item[eUID_CH_L]) ? 0 : 1);
            }
        }
        return;
    }
    // printer reinit
    if (ui_id == p->u_item[eUID_USBLP]) {
        p->usblp_status = usblp_config ();
        return;
    }

    // request server ip
    if (ui_id == 2) {
        memset (p->ip_addr, 0, sizeof(p->ip_addr));
        get_board_ip(p->ip_addr);
    }

    if ((nch = find_ditem_uid (p, ui_id, &pos)) == -1) return;
    pch = &p->ch[nch];

    if ((ui_id == p->u_item[eUID_MAC_L]) || (ui_id == p->u_item[eUID_MAC_R])) {
        if ((pch->status == eSTATUS_RUN) || (pch->status == eSTATUS_ERR))
            return;

        usblp_print_mac (pch->mac, nch);
    }
    if (!pch->ready)    {
        printf ("%s : Device not ready. (ch = %d)\n", __func__, nch);
        return;
    }
    SERIAL_RESP_FORM(serial_resp, 'R', p->d_item[pos].gid, p->d_item[pos].did, NULL);
    protocol_msg_tx (pch->puart, serial_resp);
    protocol_msg_tx (pch->puart, "\r\n");
}

//------------------------------------------------------------------------------
static char *OPT_CFG_FNAME = SERVER_CFG;
static int OPT_SW_VALUE = 0; /* 0 : default config, 1 : force odroid-c4 mode */

static void print_usage (const char *prog)
{
    puts("");
    printf("Usage: %s [-c:server config file]\n", prog);
    puts("\n"
        "  e.g) -c {server cfg filename} : default {server.cfg}\n"
        "\n"
    );
    exit(1);
}

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
    while (1) {
        static const struct option lopts[] = {
            { "config"   ,  1, 0, 'c' },
            { "gpio num" ,  1, 0, 'g' },
            { NULL, 0, 0, 0 },
        };
        int c;

        c = getopt_long(argc, argv, "c:g:", lopts, NULL);

        if (c == -1)
            break;

        switch (c) {
        case 'c':
            OPT_CFG_FNAME = optarg;
            break;
        case 'g':
            {
                int gpio_num = -1, value = 0;
                gpio_num = atoi(optarg);
                if (gpio_export (gpio_num)) {
                    if (gpio_direction (gpio_num, 0)) {
                        if (gpio_get_value (gpio_num, &value))
                            OPT_SW_VALUE = (value == 0) ? 1 :0;
                        else
                            OPT_SW_VALUE = 0;

                        printf ("%s : gpio = %d, value = %d\n", __func__, gpio_num, value);
                    }
                }
            };
            break;
        case 'h':
        default:
            print_usage(argv[0]);
            break;
        }
    }
}

//------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
    int nch;
    server_t server;

    memset (&server, 0, sizeof(server));

    // option check
    parse_opts(argc, argv);

    // UI, UART (sw value 1 = server.c4.cfg, sw value 0 = OPT_CFG_FNAME)
    server_setup (&server, OPT_SW_VALUE ? "server.c4.cfg" : OPT_CFG_FNAME);

    pthread_create (&thread_ui,    NULL, thread_ui_func,    (void *)&server);

    // Send Server boot msg
    {
        char serial_resp[SERIAL_RESP_SIZE];

        SystemCheckReady = 0;
        SERIAL_RESP_FORM(serial_resp, 'B', -1, -1, NULL);

        protocol_msg_tx (server.ch[0].puart, serial_resp);
        protocol_msg_tx (server.ch[0].puart, "\r\n");
        protocol_msg_tx (server.ch[1].puart, serial_resp);
        protocol_msg_tx (server.ch[1].puart, "\r\n");
    }

    while (1) {
        for (nch = 0; nch < server.ch_cnt; nch ++) {
            if (protocol_msg_rx (server.ch[nch].puart, server.ch[nch].rx_msg))
                protocol_parse  (&server, nch);
        }

        if (server.pts != NULL) {
            ts_event_t event;
            if (ts_get_event (server.pfb, server.pts, &event)) {
                int ui_id = ui_get_titem (server.pfb, server.pui, &event);
                if ((ui_id != -1) && (event.status == eTS_STATUS_RELEASE)) {
                    ts_event_check (&server, ui_id);
                }
            }
        }
        usleep (MAIN_LOOP_DELAY);
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
