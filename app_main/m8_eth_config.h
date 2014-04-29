/*
 * m8_eth_config.h
 *
 * Created: 4/16/2014 10:33:54 PM
 *  Author: John
 */ 


#ifndef _M8_ETH_CONFIG_H_
#define _M8_ETH_CONFIG_H_

#define dhcp_tcp_client 1
//use email client app
//#define client_tuxgr_email 1
//#define client_www_dhcp2 1

#if defined(client_tuxgr_email) && (client_tuxgr_email!=0)
#include "../ex_apps/client-tuxgr-email/ip_config.h"
#endif

#if defined(client_www_dhcp1) && (client_www_dhcp1!=0)
#include "../ex_apps/client-www-dhcp/ip_config.h"
#endif

#if defined(client_www_dhcp2) && (client_www_dhcp2!=0)
#include "../ex_apps/client-www-dhcp/ip_config.h"
#endif

#if defined(dhcp_tcp_client) && (dhcp_tcp_client!=0)
#include "../ex_apps/dhcp-tcp-client/ip_config.h"
#endif



#if 0

#if defined(client_www) && (client_www!=0)
#include "../ex_apps/client-tuxgr-email/ip_config.h"
#endif

#if defined(client_tuxgr_email) && (client_tuxgr_email!=0)
#include "../ex_apps/client-tuxgr-email/ip_config.h"
#endif

#if defined(client_tuxgr_email) && (client_tuxgr_email!=0)
#include "../ex_apps/client-tuxgr-email/ip_config.h"
#endif

#if defined(client_tuxgr_email) && (client_tuxgr_email!=0)
#include "../ex_apps/client-tuxgr-email/ip_config.h"
#endif

#if defined(client_tuxgr_email) && (client_tuxgr_email!=0)
#include "../ex_apps/client-tuxgr-email/ip_config.h"
#endif

#endif

#endif //define _M8_ETH_CONFIG_H_