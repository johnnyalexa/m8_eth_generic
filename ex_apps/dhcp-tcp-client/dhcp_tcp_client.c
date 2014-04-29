/*
 * dhcp_tcp_client.c
 *
 * Created: 4/29/2014 9:06:57 PM
 *  Author: John
 */ 

#include "../../app_main/m8_eth_config.h"
#if defined(dhcp_tcp_client) && (dhcp_tcp_client!=0)


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// http://www.nongnu.org/avr-libc/changes-1.8.html:
#define __PROG_TYPES_COMPAT__
#include <avr/pgmspace.h>
//#include "analog-10bit.h"
#include "../../enc28j60_tcp_ip_stack/ip_arp_udp_tcp.h"
#include "../../web_server_help_functions/websrv_help_functions.h"
#include "../../enc28j60_tcp_ip_stack/enc28j60.h"
#include "../../enc28j60_tcp_ip_stack/timeout.h"
#include "../../enc28j60_tcp_ip_stack/net.h"
#include "../../dns/dnslkup.h"
#include "../../dhcp/dhcp_client.h"

//
// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x27};
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
//
// The name of the virtual host which you want to
// contact (hostname of the first portion of the URL):
#define WEBSERVER_VHOST "tuxgraphics.org"

//// listen port for udp
#define MYUDPPORT 4601
#define MYTCPPORT 55056
//static const char webserver_vhost[]="tuxgraphics.org";
//
// Our DNS resolver can only look up IP addresses in the public internet
// for testing purposes you may as well use a server in your local LAN.
// In that can you can specify just the IP address as a
// sting in webserver_vhost.
//static const char webserver_vhost[]="10.0.0.1";
//static const char webserver_vhost[]="77.37.2.152";
//
// if you plan to use the sdat cgi-script instead of upld on tuxgraphics.org
// then replace upld by sdat further down in the function client_browse_url().
//
// -- there should not be any need to change something below this line --
static uint8_t otherside_www_ip[4]={192,168,0,100}; // dns will fill this
// My own IP (DHCP will provide a value for it):
static uint8_t myip[4]={0,0,0,0};
// Default gateway (DHCP will provide a value for it):
static uint8_t gwip[4]={0,0,0,0};
#define TRANS_NUM_GWMAC 1
static uint8_t gwmac[6];
#define TRANS_NUM_WEBMAC 2
static uint8_t otherside_www_gwmac[6]={0x78,0x92,0x9C,0x43,0x53,0x4A};
// Netmask (DHCP will provide a value for it):
static uint8_t netmask[4];
static char urlvarstr[32];
static volatile uint8_t sec=0; // counts up to 6 and goes back to zero
static volatile uint8_t gsec=0; // counts up beyond 6 sec
//static  uint8_t gmin=0;

// set output to VCC, red LED off
#define LEDOFF PORTB|=(1<<PORTB1)
// set output to GND, red LED on
#define LEDON PORTB&=~(1<<PORTB1)
// to test the state of the LED
#define LEDISOFF PORTB&(1<<PORTB1)
// packet buffer
#define BUFFER_SIZE 650
static uint8_t buf[BUFFER_SIZE+1];
//static uint8_t start_web_client=0;
static uint8_t web_client_sendok=0;
//static int8_t processing_state=0;

// set output to VCC, red LED off
#define LEDOFF PORTB|=(1<<PORTB1)
// set output to GND, red LED on
#define LEDON PORTB&=~(1<<PORTB1)
// to test the state of the LED
#define LEDISOFF PORTB&(1<<PORTB1)
//

// set output to VCC, red LED off
#define PD0LEDOFF PORTD|=(1<<PORTD0)
// set output to GND, red LED on
#define PD0LEDON PORTD&=~(1<<PORTD0)
// to test the state of the LED
#define PD0LEDISOFF PORTD&(1<<PORTD0)
//

// timer interrupt, called automatically every second
ISR(TIMER1_COMPA_vect){
	sec++;
	gsec++;
	if (sec>5){
		sec=0;
		dhcp_6sec_tick();
	}
}

// Generate an interrup about ever 1s form the 12.5MHz system clock
// Since we have that 1024 prescaler we do not really generate a second
// (1.00000256000655361677s)
void timer_init(void)
{
	/* write high byte first for 16 bit register access: */
	TCNT1H=0;  /* set counter to zero*/
	TCNT1L=0;
	// Mode 4 table 14-4 page 132. CTC mode and top in OCR1A
	// WGM13=0, WGM12=1, WGM11=0, WGM10=0
	TCCR1A=(0<<COM1B1)|(0<<COM1B0)|(0<<WGM11);
	TCCR1B=(1<<CS12)|(1<<CS10)|(1<<WGM12)|(0<<WGM13); // crystal clock/1024
	// divide clock by 1024: 12.5MHz/1024=12207.0313 Hz

	// At what value to cause interrupt. You can use this for calibration
	// of the clock. Theoretical value for 12.5MHz: 12207=0x2f and 0xaf
	OCR1AH=0x2f;
	OCR1AL=0xaf;
	// interrupt mask bit:
	//TIMSK1 for atmega88
	TIMSK = (1 << OCIE1A);
}

// we were ping-ed by somebody, store the ip of the ping sender
// and trigger an upload to http://tuxgraphics.org/cgi-bin/upld
// This is just for testing and demonstration purpose
//
// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void browserresult_callback(uint16_t webstatuscode,uint16_t datapos __attribute__((unused)), uint16_t len __attribute__((unused))){
	if (webstatuscode==200){
		web_client_sendok++;
		LEDOFF;
	}
}

// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number,uint8_t *mac){
	uint8_t i=0;
	if (reference_number==TRANS_NUM_GWMAC){
		// copy mac address over:
		while(i<6){gwmac[i]=mac[i];i++;}
	}
	if (reference_number==TRANS_NUM_WEBMAC){
		// copy mac address over:
		while(i<6){otherside_www_gwmac[i]=mac[i];i++;}
	}
}



// Declare a callback function to get the result (tcp data from the server):
//

uint8_t your_client_tcp_result_callback(uint8_t fd, uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data){
	// Do not close the tcp connection after sending the packet
	uint8_t close_tcp_session = 0;
	
	//statuscode=0 means the buffer has valid data
	if(statuscode==0){
		
		
	}
	
	return(close_tcp_session);
}


// Declare a callback function to be called in order to fill in the
// request (tcp data sent to the server):

uint16_t your_client_tcp_datafill_callback(uint8_t fd){
	//no data has been loaded yet
	uint16_t len_of_data_filled_in = 0; 
	
	return(fill_tcp_data_p(buf,0,PSTR("SUCCESS")));
	
	return(len_of_data_filled_in);
	
}

void send_tcp_data(void){
	
	uint8_t fd;
	
	//get_mac_with_arp(otherside_www_ip,TRANS_NUM_WEBMAC,&arpresolver_result_callback);
	
	
	fd=client_tcp_req(&your_client_tcp_result_callback,&your_client_tcp_datafill_callback,MYTCPPORT,otherside_www_ip,otherside_www_gwmac);
 
}







int main(void){
	uint16_t dat_p,plen;
    uint8_t payloadlen=0;
	char str[20];
	uint8_t rval;
	uint16_t i=0;

	_delay_loop_1(0); // 60us

	/*initialize enc28j60*/
	enc28j60Init(mymac);
	enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
	_delay_loop_1(0); // 60us
	
	timer_init();
	sei();

	/* Magjack leds configuration, see enc28j60 datasheet, page 11 */
	// LEDB=yellow LEDA=green
	//
	// 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
	// enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
	enc28j60PhyWrite(PHLCON,0x476);

	// DHCP handling. Get the initial IP
	rval=0;
	init_mac(mymac);
	while(rval==0){
		plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
		buf[BUFFER_SIZE]='\0';
		rval=packetloop_dhcp_initial_ip_assignment(buf,plen,mymac[5]);
	}
	// we have an IP:
	dhcp_get_my_ip(myip,netmask,gwip);
	client_ifconfig(myip,netmask);

	if (gwip[0]==0){
		// we must have a gateway returned from the dhcp server
		// otherwise this code will not work
		PD0LEDON; // error
		while(1); // stop here
	}

	// we have a gateway.
	// find the mac address of the gateway (e.g your dsl router).
	get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
	while(get_mac_with_arp_wait()){
		// to process the ARP reply we must call the packetloop
		plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
		packetloop_arp_icmp_tcp(buf,plen);
	}

#if 0 //ramane daca vrem dns
	if (string_is_ipv4(WEBSERVER_VHOST)){
		// the the webserver_vhost is not a domain name but alreay
		// an IP address written as sting
		parse_ip(otherside_www_ip,WEBSERVER_VHOST);
		processing_state=2; // no need to do any dns look-up
	}
#endif


	while(1){
		plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
		buf[BUFFER_SIZE]='\0'; // http is an ascii protocol. Make sure we have a string terminator.
		// DHCP renew IP:
		plen=packetloop_dhcp_renewhandler(buf,plen); // for this to work you have to call dhcp_6sec_tick() every 6 sec
		dat_p=packetloop_arp_icmp_tcp(buf,plen);
		
		if(plen==0){
			//we are idle so send message
			if(i>2000){
			send_tcp_data();
			i=0;
			continue;
			}else
				i++;
			//continue;
		}
#if 0		
		if(plen==0){
			// we are idle here
			if (processing_state==0){
				if (!enc28j60linkup()) continue; // only for dnslkup_request we have to check if the link is up.
				gsec=0;
				processing_state=1;
				dnslkup_request(buf,WEBSERVER_VHOST,gwmac);
				LEDON;
				continue;
			}
			if (processing_state==1 && dnslkup_haveanswer()){
				dnslkup_get_ip(otherside_www_ip);
				processing_state=2;
				LEDOFF;
			}
			if (processing_state==2){
				if (route_via_gw(otherside_www_ip)){
					// otherside_www_ip is behind the GW
					i=0;
					while(i<6){
						otherside_www_gwmac[i]=gwmac[i];
						i++;
					}
					processing_state=4;
					}else{
					// the web server we want to contact is on the local lan. find the mac:

					LEDON;
					get_mac_with_arp(otherside_www_ip,TRANS_NUM_WEBMAC,&arpresolver_result_callback);
					processing_state=3;
				}
				continue;
			}
			if (processing_state==3 && get_mac_with_arp_wait()==0){
				// we have otherside_www_gwmac
				LEDOFF;
				processing_state=4;
			}
			if (processing_state==4){
				processing_state=5;
				// we are ready to upload the first measurement data after startup:
				if (start_web_client==0) start_web_client=1;

			}
			if (processing_state==1){
				// retry every 20 sec if dns-lookup failed:
				if (gsec > 20){
					processing_state=0;
				}
				// don't try to use web client before
				// we have a result of dns-lookup
				continue;
			}
			//----------
			if (start_web_client==1){
				LEDON;
				gsec=0;
				gmin=0;
				start_web_client=2;
				mk_net_str(str,myip,4,'.',10);
				urlencode(str,urlvarstr);
				strcat(urlvarstr,"&adc0=");
				//                                itoa(convertanalog(0),str,10);
				strcat(urlvarstr,str);
				//client_browse_url(PSTR("/cgi-bin/sdat?pw=sec&action=auto&ethbrd_ip="),urlvarstr,PSTR(WEBSERVER_VHOST),&browserresult_callback,otherside_www_ip,otherside_www_gwmac);
				//                                client_browse_url(PSTR("/cgi-bin/upld?pw=sec&action=auto&ethbrd_ip="),urlvarstr,PSTR(WEBSERVER_VHOST),&browserresult_callback,otherside_www_ip,otherside_www_gwmac);
			}
			// our clock:
			if (gsec>59){
				gsec=0;
				gmin++;
			}
			if (gmin>59){
				gmin=0;
				start_web_client=1; // upload one measurement every hour
			}
			continue;
		}
#endif		
		if(dat_p==0){ // plen!=0
			// check for incomming messages not processed
			// as part of packetloop_arp_icmp_tcp, e.g udp messages
			//udp_client_check_for_dns_answer(buf,plen);
			//continue;
			// check for udp
			goto UDP;
		}

UDP:
		// check if ip packets are for us:
		if(eth_type_is_ip_and_my_ip(buf,plen)==0){
			continue;
		}		

        if (buf[IP_PROTO_P] == IP_PROTO_UDP_V &&\
            buf[UDP_DST_PORT_H_P] == (MYUDPPORT>>8) &&\
            buf[UDP_DST_PORT_L_P] == (MYUDPPORT&0xff)) {		
               payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
               // you must sent a string starting with v				
				
				strcpy(str,"Sensolight! usage: ver");
				make_udp_reply_from_request(buf,str,strnlen(str,35),MYUDPPORT);
			}
		
		// when dat_p!=0 then we get normally a http
		// request but this is not a web server so we do nothing.
	}
	return (0);
}






















#endif