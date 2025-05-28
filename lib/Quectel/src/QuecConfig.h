/**
 * @file       QuecConfig.h
 * @author     Umesh Walkar
 * @license    CONFIDENTIAL 
 * @copyright  Copyright (c) 2021 Umesh Walkar, All Rights Reserved
 * @date       July 2021
 */

#ifndef QUECCONFIG_H
#define QUECCONFIG_H

/**
 * select either module
 */
// #define USE_QUECTEL_M95
#define USE_QUECTEL_M66 //CGMR=M66FBR03A05 & M66FBR03A09
//    #define USE_QUECTEL_BG96  //CGMR=BG96MAR02A05M1G

/* refer: http://www.mobileussdcodes.com/apn-gprs-internet-setting-2g3g4g-configuration-apn/*/
#define GPRS_BEARER_APN "www"
#define GPRS_APN_AIRTEL "airtelgprs.com"
#define GPRS_APN_VODAFONE "www"
#define GPRS_APN_IDEA "INTERNET" //isafe
#define GPRS_APN_TATA "TATA.DOCOMO.INTERNET"
#define GPRS_APN_BSNL "bsnlnet"
#define GPRS_APN_RELIANCE "rcomnet"
#define GPRS_APN_AIRCEL "aircelgprs.pr"
#define GPRS_APN_JIO "jionet"
#define GPRS_APN_AIRTEL_WHITELIST "airteliot.com"
#define GPRS_APN_VI_WHITELIST "m2misafe"
#define GPRS_APN_IDEA_WHITELIST "isafe"

/**
 * Apply commands ATI and AT+GMI
 * The library shall support
 * M95: 
 * M66: MTK0828, M66FAR01A12BT
 * M66: M66FBR03A05, M66FBR03A09
 * M66-DS: M66DSFAR01A02 
 * BG96: BG96MAR02A05M1G
 */

#endif // QUECCONFIG_H