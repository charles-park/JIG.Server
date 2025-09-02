//------------------------------------------------------------------------------
/**
 * @file device_check.h
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
#ifndef __DEVICE_CHECK_H__
#define __DEVICE_CHECK_H__

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
//------------------+-----------------------------------------------------------
// grp name(gid)    | dev_name(dev_id), action (0 , 10, 20, 30, ...)
//                  | did = dev_id + action
//------------------+-----------------------------------------------------------
//    SYSTEM(0)     | READ  : MEM(0), FB_X(1), FB_Y(2)
//------------------+-----------------------------------------------------------
//    STORAGE(1)    | READ  : eMMC(00), SD(01), SATA(02), NVME(03)
//                  | WRITE : eMMC(10), SD(11), SATA(12), NVME(13)
//                  | LINK  : eMMC(20), SD(21), SATA(22), NVME(23)
//------------------+-----------------------------------------------------------
//    USB(2)        | READ  : USB-0(00), USB-1(01), USB-2(02), USB-3(03), USB-4(04), USB-5(05),
//                  | WRITE : USB-0(10), USB-1(11), USB-2(12), USB-3(13), USB-4(14), USB-5(15),
//                  | LINK  : USB-0(20), USB-1(21), USB-2(22), USB-3(23), USB-4(24), USB-5(25),
//------------------+-----------------------------------------------------------
//    HDMI(3)       | READ  : EDID(0), HPD(1)
//------------------+-----------------------------------------------------------
//    ADC(4)        | READ  : ADC_37(0), ADC_40(1)
//------------------+-----------------------------------------------------------
//    ETHERNET(5)   | READ  : IP(0), MAC(1), IPERF_SERVER(2), LINK_SPEED(3), NLP_SERVER(4),
//                  |       : NLP_SERVER_PORT(5), IPERF_S(6), IPREF_C(7)
//------------------+-----------------------------------------------------------
//    HEADER(6)     | SET PATTERN0 : H40(00), H7(01), H14(02)
//                  | SET PATTERN1 : H40(10), H7(11), H14(12)
//                  | SET PATTERN2 : H40(20), H7(21), H14(22)
//                  | SET PATTERN3 : H40(30), H7(31), H14(32)
//------------------+-----------------------------------------------------------
//    AUDIO(7)      | SET OFF : LEFT(00), RIGHT(01)
//                  | SET ON  : LEFT(10), RIGHT(11)
//------------------+-----------------------------------------------------------
//    LED(9)        | SET OFF : POWER(00), ALIVE(01), LINK_100M(02), LINK_1G(03)
//                  | SET ON  : POWER(10), ALIVE(11), LINK_100M(12), LINK_1G(13)
//------------------+-----------------------------------------------------------
//    PWM(9)        | SET OFF : PWM0(00), PWM1(01)
//                  | SET ON  : PWM0(10), PWM1(11)
//------------------+-----------------------------------------------------------
//    IR(10)        | IR Keycode (Run Thread)
//------------------+-----------------------------------------------------------
//    GPIO(11)      | SET OFF :  0 ~  x (low)
//                  | SET ON  : 10 ~ 1x (High) : x -> gpio ID
//------------------+-----------------------------------------------------------
//    USB F/W(12)   | READ    : C4(0), XU4(1)
//------------------+-----------------------------------------------------------
//    MISC(13))     | READ    : SPI B/T Release(00)
//                  |         : SPI B/T Press  (01)
//                  | READ    : HP Detect Out  (10)
//                  |         : HP Detect In   (11)
//------------------+-----------------------------------------------------------
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
    eGID_MISC,
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
// server.c
// extern int  device_resp_parse   (const char *resp, parse_resp_data_t *pdata);
// extern int  device_resp_check   (server_t *p, int fd, parse_resp_data_t *pdata);

//------------------------------------------------------------------------------
#endif  // __DEVICE_CHECK_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------