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
//------------------------------------------------------------------------------
#define RUN_BOX_ON      RGB_TO_UINT(204, 204, 0)
#define RUN_BOX_OFF     RGB_TO_UINT(153, 153, 0)

//------------------------------------------------------------------------------
#define STR_PATH_LENGTH     128
#define STR_NAME_LENGTH     16

//------------------------------------------------------------------------------
// delay us value
//------------------------------------------------------------------------------
#define MAIN_LOOP_DELAY     500

#define FUNC_LOOP_DELAY     (100*1000)

#define CHECK_CMD_DELAY     (500*1000)
#define UPDATE_UI_DELAY     (500*1000)

//------------------------------------------------------------------------------
/* Jig device-test limite-time(30s) */
#define DEFAULT_RUNING_TIME 30

/* UART protocol wait limite-time(60s) */
#define UART_WAIT_TIME      60

/* USBLP Printer Info */
#define USBLP_MAX_CHAR      19
#define USBLP_ERR_LINE      20

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

/* JIG Channel */
enum {
    eCH_LEFT = 0,
    eCH_RIGHT,
    eCH_END
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

typedef struct h_item__t {
    int did;    // device id (0=h40, 1=h7...)
    int pin;    // header pin
    int max;    // check max volt
    int min;    // check min volt
}   h_item_t;

typedef struct l_item__t {
    int did;
    int on_mV, off_mV;
}   l_item_t;

//------------------------------------------------------------------------------
/* spectial check item(ADC value) */
#define CHECK_ITEM_CNT  10

typedef struct channel__t {
    int         status;
    int         ready;      /* ready signal received */
    int         ready_wait; /* uart receive wait time */

    /* ADC board */
    int         i2c_fd;
    char        i2c_path [STR_PATH_LENGTH];

    uart_t      *puart;

    /* protocol serial port */
    char        uart_path[STR_PATH_LENGTH];
    int         uart_baud;

    /* protocol response(tx)/request(rx) */
    char        rx_msg [SERIAL_RESP_SIZE +1];
    char        tx_msg [SERIAL_RESP_SIZE +1];

    // led check item
    l_item_t    l_item[CHECK_ITEM_CNT];
    int         l_item_cnt;

    // mac msg for usblp
    char        mac [DEVICE_RESP_SIZE];
    // err msg for usblp
    char        err_msg [USBLP_ERR_LINE][USBLP_MAX_CHAR];
    int         err_cnt;

}   channel_t;

//------------------------------------------------------------------------------
typedef struct server__t {
    // Server control board name (c4, c5, m1s,...)
    char        b_name[STR_NAME_LENGTH];

    // jig control fname
    char        j_name[STR_NAME_LENGTH];

    // Frame buffer (HDMI or LCD)
    char        fb_path[STR_PATH_LENGTH];
    int         fb_rotate;
    fb_info_t   *pfb;

    // UI Control file
    char        ui_path[STR_PATH_LENGTH];
    ui_grp_t    *pui;

    // Touch device control
    ts_t        *pts;

/* IP Address string size */
#define IP_STR_SIZE     20

    // Server control board ip
    char        ip_addr[IP_STR_SIZE];

    // power check item
    pw_item_t   pw_item[CHECK_ITEM_CNT];
    int         pw_item_cnt;

    // spectial header check item
    h_item_t    h_item[CHECK_ITEM_CNT];
    int         h_item_cnt;

    // channel left/right
    int         ch_cnt;
    channel_t   ch[eCH_END];

    // connon ui control item (alive, bip,... eUID_xxx)
    int         u_item[eUID_END];

#define DISPLAY_DATA_COUNT  100
    // Device display item
    d_item_t    d_item[DISPLAY_DATA_COUNT];
    int         d_item_cnt;

    // usblp connect status
    int         usblp_status;
    int         usblp_mode;

    // ts find str (T item)
    // udevadm info -a -n /dev/input/event? | grep {ts_str}
    char        ts_str [STR_NAME_LENGTH];
    int         ts_reset_gpio;
    int         ts_reset_level;
}   server_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// setup.c
//------------------------------------------------------------------------------
extern void ts_reinit       (server_t *p);
extern int  board_config    (server_t *p, const char *j_cfg);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#endif  // __SERVER_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------