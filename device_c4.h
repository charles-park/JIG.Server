//------------------------------------------------------------------------------
/**
 * @file device_c4.h
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
#ifndef __DEVICE_C4_H__
#define __DEVICE_C4_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define STR_PATH_LENGTH     128
#define STR_NAME_LENGTH     16

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// https://docs.google.com/spreadsheets/d/1igBObU7CnP6FRaRt-x46l5R77-8uAKEskkhthnFwtpY/edit?gid=719914769#gid=719914769
//------------------------------------------------------------------------------
// DEVICE_ACTION Value
// 0 (10 > did) = Read, Clear, PT0
// 1 (20 > did) = Write, Set, PT1
// 2 (30 > did) = Link, PT2
// 3 (40 > did) = PT3
//------------------------------------------------------------------------------
#define DEVICE_ACTION(did)      (did / 10)
#define DEVICE_ID(did)          (did % 10)

//------------------------------------------------------------------------------
// DEVICE_ACTION GPIO Value (GPIO NUM : 0 ~ 999)
// 0 (1000 > did) = Clear
// 1 (2000 > did) = Set
//------------------------------------------------------------------------------
#define DEVICE_ACTION_GPIO(did)  (did / 1000)

//------------------------------------------------------------------------------
//
// message discription
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// start |,|cmd|,|GID|,|DID |,| status |,| value(%20s) |,| end | extra  |
//------------------------------------------------------------------------------
//   1    1  1  1  2  1  4   1     1    1       20      1   1      2      = 36bytes(add extra 38)
//------------------------------------------------------------------------------
//   @   |,| S |,| 00|,|0000|,|P/F/I/W |,|  resp data  |,|  #  | '\r\n' |
//------------------------------------------------------------------------------
#define SERIAL_RESP_SIZE    36
#define RESP_CMD_STATUS     'S'
#define RESP_CMD_REQUEST    'R'
#define RESP_CMD_BOOT       'B'
#define RESP_CMD_ERROR      'E'
#define RESP_CMD_ACK        'A'
#define RESP_CMD_OKAY       'O'

#define DEVICE_GID_SIZE     2
#define DEVICE_DID_SIZE     4
#define DEVICE_RESP_SIZE    22  // [status(1), value(20)]

#define SERIAL_RESP_FORM(buf, cmd, gid, did, resp)  sprintf (buf, "@,%c,%02d,%04d,%22s,#", cmd, gid, did, resp)
#define DEVICE_RESP_FORM_INT(buf, status, value)    sprintf (buf, "%c,%20d", status, value)
#define DEVICE_RESP_FORM_STR(buf, status, value)    sprintf (buf, "%c,%20s", status, value)

//------------------------------------------------------------------------------
// Group ID
//------------------------------------------------------------------------------
enum {
    eGID_SYSTEM = 0,
    eGID_STORAGE,
    eGID_USB,
    eGID_HDMI,
    eGID_ADC,
    eGID_ETHERNET,
    eGID_HEADER,
    eGID_AUDIO,
    eGID_LED,
    eGID_PWM,
    eGID_IR,
    eGID_GPIO,
    eGID_FW,
    eGID_END,
};

//------------------------------------------------------------------------------
typedef struct parse_resp_data__t {
    char    cmd;
    int     gid;
    int     did;
    char    status_c;
    int     status_i;
    char    resp_s[DEVICE_RESP_SIZE +1];
    int     resp_i;
}   parse_resp_data_t;

//------------------------------------------------------------------------------
extern int  device_resp_parse   (const char *resp, parse_resp_data_t *pdata);

//------------------------------------------------------------------------------
#endif  // __DEVICE_C4_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------