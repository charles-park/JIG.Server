//------------------------------------------------------------------------------
/**
 * @file server.h
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
#ifndef __SERVER_H__
#define __SERVER_H__

//------------------------------------------------------------------------------
#include "lib_fbui/lib_fb.h"
#include "lib_fbui/lib_ui.h"
#include "lib_i2cadc/lib_i2cadc.h"
#include "lib_usblp/lib_usblp.h"
#include "lib_gpio/lib_gpio.h"
#include "device_check.h"
#include "protocol.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// JIG Protocol(V2.0)
// https://docs.google.com/spreadsheets/d/1Of7im-2I5m_M-YKswsubrzQAXEGy-japYeH8h_754WA/edit#gid=0
//
//------------------------------------------------------------------------------
#define SERVER_CFG  "server.cfg"
#define SERVER_UI   "ui_server.cfg"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define RUN_BOX_ON      RGB_TO_UINT(204, 204, 0)
#define RUN_BOX_OFF     RGB_TO_UINT(153, 153, 0)

//------------------------------------------------------------------------------
#define STR_PATH_LENGTH     128
#define STR_NAME_LENGTH     16

//------------------------------------------------------------------------------
#define MAIN_LOOP_DELAY     500

#define FUNC_LOOP_DELAY     (100*1000)

#define CHECK_CMD_DELAY     (500*1000)
#define UPDATE_UI_DELAY     (500*1000)

//------------------------------------------------------------------------------
#define DEFAULT_RUNING_TIME  30

//------------------------------------------------------------------------------
// system state
//------------------------------------------------------------------------------
enum {
    eSTATUS_STOP,
    eSTATUS_WAIT,
    eSTATUS_RUN,
    eSTATUS_PRINT,
    eSTATUS_ERR,
    eSTATUS_END
};

//------------------------------------------------------------------------------
// ui control id
//------------------------------------------------------------------------------
enum {
    eUID_ALIVE,     // blink box
    eUID_IPADDR,    // board ip display
    eUID_CH_L,      // send stop cmd to device(touch)
    eUID_CH_R,
    eUID_STATUS_L,  // print err(touch)
    eUID_STATUS_R,
    eUID_MAC_L,     // print mac(touch)
    eUID_MAC_R,
    eUID_USBLP,     // usblp status display & usblp reconfig(touch)
    eUID_END,
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
typedef struct d_item__t {
    int gid, did, uid_l, uid_r, is_str;
}   d_item_t;

typedef struct pw_item__t {
    char cname[STR_NAME_LENGTH];
    int check_mV;
    int read_mV;
}   pw_item_t;

//------------------------------------------------------------------------------
/* USBLP Printer Info */
#define USBLP_MAX_CHAR  19
#define USBLP_ERR_LINE  20

typedef struct channel__t {
    int         status;
    int         ready;  /* ready signal received */

    int         i2c_fd;
    char        i2c_path [STR_PATH_LENGTH];

    uart_t      *puart;

    char        uart_path[STR_PATH_LENGTH];
    int         uart_baud;

    char        rx_msg [SERIAL_RESP_SIZE +1];
    char        tx_msg [SERIAL_RESP_SIZE +1];

    // power check item
    pw_item_t   pw_item[10];
    int         pw_item_cnt;

    // usblp mac msg
    char        mac [DEVICE_RESP_SIZE];
    // usblp err msg
    char        err_msg [USBLP_ERR_LINE][USBLP_MAX_CHAR];
    int         err_cnt;

}   channel_t;

//------------------------------------------------------------------------------
typedef struct server__t {
    // HDMI UI
    char        fb_path[STR_PATH_LENGTH];
    fb_info_t   *pfb;
    ui_grp_t    *pui;
    ts_t        *pts;

    // channel left/right
    int         ch_cnt;
    channel_t   ch[2];

    // ui control item (alive, bip,... eUID_xxx)
    int         u_item[eUID_END];

    // Device display item
    d_item_t    d_item[100];
    int         d_item_cnt;

    // usblp connect status
    int         usblp_status;
    int         usblp_mode;

    char        ts_vid [STR_NAME_LENGTH];
    int         ts_reset_gpio;
    int         ts_reset_level;
}   server_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// setup.c
//------------------------------------------------------------------------------
extern void ts_reinit       (server_t *p);
extern int  server_setup    (server_t *p);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // __SERVER_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------