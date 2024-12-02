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
#include "lib_fbui/lib_fb.h"
#include "lib_fbui/lib_ui.h"
#include "device_c4.h"
#include "protocol.h"

//------------------------------------------------------------------------------
//
// JIG Protocol(V2.0)
// https://docs.google.com/spreadsheets/d/1Of7im-2I5m_M-YKswsubrzQAXEGy-japYeH8h_754WA/edit#gid=0
//
//------------------------------------------------------------------------------

#define SERVER_FB       "/dev/fb0"
#define SERVER_UART     "/dev/ttyUSB1"

#define SERVER_UI       "ui_c4_server.cfg"

#define UART_BAUDRATE   115200

/* NLP Printer Info */
#define NLP_MAX_CHAR    19
#define NLP_ERR_LINE    20

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define UID_ALIVE       0
#define UID_IPADDR      4
#define UID_STATUS      47

#define RUN_BOX_ON      RGB_TO_UINT(204, 204, 0)
#define RUN_BOX_OFF     RGB_TO_UINT(153, 153, 0)

//------------------------------------------------------------------------------
#define MAIN_LOOP_DELAY     500

#define FUNC_LOOP_DELAY     (100*1000)

#define CHECK_CMD_DELAY     (500*1000)
#define UPDATE_UI_DELAY     (500*1000)

// system state
enum { eSTATUS_WAIT, eSTATUS_RUN, eSTATUS_PRINT, eSTATUS_STOP, eSTATUS_END };

typedef struct server__t {
    // HDMI UI
    fb_info_t   *pfb;
    ui_grp_t    *pui;

    // UART communication
    uart_t      *puart;
    char        rx_msg [SERIAL_RESP_SIZE +1];
    char        tx_msg [SERIAL_RESP_SIZE +1];
}   server_t;


//------------------------------------------------------------------------------
#define DEFAULT_RUNING_TIME  30

volatile int SystemCheckReady = 0, RunningTime = DEFAULT_RUNING_TIME;
volatile int UIStatus = eSTATUS_WAIT;

pthread_t thread_ui;
pthread_t thread_check;

//------------------------------------------------------------------------------
struct ui_item {
    int gid, did, uid_l, uid_r, is_str;
};

const struct ui_item uitem[] = {
    // USB F/W C4
    { 12, 0, 92, 96, 1 },

    // ETHERNET, IP (Board ID)
    {  5, 0, 42, 46, 1 },
    // ETHERNET, MAC
    {  5, 1, 62, 66, 1 },
    // ETHERNET, IPERF
    {  5, 2, 94, 98, 1 },

    // SYSTEM, MEM
    {  0, 0,124,128, 0 },
    // SYSTEM, FB
    {  0, 3,142,146, 0 },

    // HDMI, EDID
    {  3, 0,143,147, 0 },
    // HDMI, HPD
    {  3, 1,144,148, 0 },

    // ADC, ADC37
    {  4, 0,134,138, 0 },
    // ADC, ADC40
    {  4, 1,135,139, 0 },

    // eMMC Speed
    {  1, 0,133,137, 1 },

    // IR
    { 10, 0,125,129, 0 },

    // USB OTG Read (2.0)
    {  2, 0,132,136, 1 },
    // USB 3.0 Read (3.0) L/D
    {  2, 1,113,117, 1 },
    // USB 3.0 Read (3.0) L/U
    {  2, 2,112,116, 1 },
    // USB 3.0 Read (3.0) R/D
    {  2, 3,115,119, 1 },
    // USB 3.0 Read (3.0) R/U
    {  2, 4,114,118, 1 },

    // HEADER_40, PT0
    {  6, 0,162,166, 0 },
    // HEADER_40, PT1
    {  6,10,163,167, 0 },
    // HEADER_40, PT2
    {  6,20,164,168, 0 },
    // HEADER_40, PT3
    {  6,30,165,169, 0 },

    // HEADER_7, PT0
    {  6, 1,172,176, 0 },
    // HEADER_7, PT1
    {  6,11,173,177, 0 },
    // HEADER_7, PT2
    {  6,21,174,178, 0 },
    // HEADER_7, PT3
    {  6,31,175,179, 0 },

    // LED, POWER
    {  8,10,145,149, 0 },
    // LED, ALIVE_ON
    {  8,11,154,158, 0 },
    // LED, ALIVE_OFF
    {  8, 1,155,159, 0 },
    // LED, LINK_100M
    {  8,12,152,156, 0 },
    // LED, LINK_1G
    {  8,13,153,157, 0 },
};

//------------------------------------------------------------------------------
// 문자열 변경 함수. 입력 포인터는 반드시 메모리가 할당되어진 변수여야 함.
//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
    int i, c = strlen(p);

    for (i = 0; i < c; i++, p++)
        *p = toupper(*p);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int print_test_result (server_t *p)
{
    return 0;
}

//------------------------------------------------------------------------------
static int get_board_ip (char *ip_addr)
{
    int fd, retry_cnt = 10;
    struct ifreq ifr;

retry:
    usleep (500 * 1000);    // 500ms delay
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
    memset (ip_addr, 0x00, sizeof(ip_addr));
    inet_ntop(AF_INET, ifr.ifr_addr.sa_data+2, ip_addr, sizeof(struct sockaddr));
    printf ("%s : ip_address = %s\n", __func__, ip_addr);

    return 1;
}

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
static void *thread_ui_func (void *pserver)
{
    static int onoff = 0;
    server_t *p = (server_t *)pserver;
    char ip_addr[20];

    memset (ip_addr,    0, sizeof(ip_addr));
    get_board_ip(ip_addr);

    while (1) {
        onoff = !onoff;
        ui_set_ritem (p->pfb, p->pui, UID_ALIVE,
                    onoff ? COLOR_GREEN : p->pui->bc.uint, -1);
        ui_set_sitem (p->pfb, p->pui, UID_IPADDR, -1, -1, ip_addr);

        if (onoff)  ui_update (p->pfb, p->pui, -1);

        usleep (UPDATE_UI_DELAY);
    }
    return pserver;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static int server_setup (server_t *p)
{
    if ((p->pfb = fb_init (SERVER_FB)) == NULL)         exit(1);
    if ((p->pui = ui_init (p->pfb, SERVER_UI)) == NULL) exit(1);
    // ODROID-C4 (115200 baud)
    if ((p->puart = uart_init (SERVER_UART, UART_BAUDRATE)) != NULL) {
        if (ptc_grp_init (p->puart, 1)) {
            if (!ptc_func_init (p->puart, 0, SERIAL_RESP_SIZE, protocol_check, protocol_catch)) {
                printf ("%s : protocol install error.", __func__);
                exit(1);
            }
        }
        return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
static int find_uitem_pos (int gid, int did)
{
    int i;

    for (i = 0; i < sizeof(uitem)/sizeof(uitem[0]); i++) {
        if ((uitem[i].gid == gid) && (uitem[i].did == did))
            return i;
    }

    return 0;
}

//------------------------------------------------------------------------------
static void protocol_parse (server_t *p)
{
    parse_resp_data_t pitem;
    char *rx_msg = (char *)p->rx_msg;
    char serial_resp[SERIAL_RESP_SIZE], resp [DEVICE_RESP_SIZE];

    if (!device_resp_parse (rx_msg, &pitem))   return;

    switch (pitem.cmd) {
        /* Device Ready received */
        case 'R':
            /* Server System Ready send */
            SERIAL_RESP_FORM(serial_resp, 'O', -1, -1, NULL);
            break;
        /* Device status received */
        case 'S':
            {
                int pos = find_uitem_pos (pitem.gid, pitem.did);
                // int uid = channel ? uitem[pos].uid_r : uitem[pos].uid_l;
                int uid = uitem[pos].uid_r;

                if (uitem[pos].is_str)
                    ui_set_sitem (p->pfb, p->pui, uid, -1, -1, pitem.resp_s);

                if (pitem.status_c != 'C') {
                    ui_set_ritem (p->pfb, p->pui, uid,
                                (pitem.status_i == 1) ? COLOR_GREEN : COLOR_RED, -1);
                } else {
                    ui_set_ritem (p->pfb, p->pui, uid, COLOR_YELLOW, -1);

                    /* Device request I2C ADC Check */
                    if ((pitem.gid == eGID_ETHERNET) && (pitem.did == 2)) {
                        int iperf_speed = 0;
                        iperf_speed = iperf3_client_func (pitem.resp_s);
                        printf ("%s : iperf = %d\n", __func__, iperf_speed);
                        memset (pitem.resp_s, 0, sizeof(pitem.resp_s));
                        sprintf(pitem.resp_s, "%d", iperf_speed);
                    }
                }
            }
            DEVICE_RESP_FORM_STR(resp, pitem.status_c, pitem.resp_s);
            SERIAL_RESP_FORM(serial_resp, (pitem.cmd == 'S') ? 'A' : 'C',
                                            pitem.gid, pitem.did, resp);
            break;
        case 'M':   // mac print
        case 'E':   // error msg
        case 'X':   // Device test complete
            return;
        default :
            printf ("%s : unknown command!! (%c)\n", __func__, pitem.cmd);
            return;
    }
    protocol_msg_tx (p->puart, serial_resp);    protocol_msg_tx (p->puart, "\r\n");
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main (void)
{
    server_t server;

    memset (&server, 0, sizeof(server));

    // UI, UART
    server_setup (&server);

    pthread_create (&thread_ui,    NULL, thread_ui_func,    (void *)&server);

    // Send Server boot msg
    {
        char serial_resp[SERIAL_RESP_SIZE];

        SystemCheckReady = 0;
        SERIAL_RESP_FORM(serial_resp, 'B', -1, -1, NULL);
        protocol_msg_tx (server.puart, serial_resp);    protocol_msg_tx (server.puart, "\r\n");
    }

    while (1) {
        if (protocol_msg_rx (server.puart, server.rx_msg))
            protocol_parse  (&server);
        usleep (MAIN_LOOP_DELAY);
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
