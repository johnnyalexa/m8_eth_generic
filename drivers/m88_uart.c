/*
 * m88_uart.c
 *
 * Created: 4/1/2014 4:39:57 PM
 *  Author: ionut.alexa
 */ 

#include<avr/io.h>
#include <string.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "m8_drivers.h"


/*-------------------- Init_Uart      -------------------------
*    Function:    Init_Uart
*    Purpose:    Initialize serial connectivity @115200 baud.
*
*    Parameters:    none
*    Returns:    none
*------------------------------------------------------------*/

#if !defined(__AVR_ATmega8__)
void Init_Uart(void){
	cli(); 
	// Enable U2Xn to get a baud rate with less error margin
	//UCSR0A = (1<<U2X0);
	// Transmitter enable and Receiver enable
	UCSR0B = (1<<TXEN0)|(1<<RXEN0);
	// Asynchronous USART | No parity | 1 stopbit | CH size 8-bit
	UCSR0C = (1<<UCSZ00) | (1<<UCSZ01) | (1<<USBS0);
	// 115200 Baudrate @ 0.9216 Mhz
	UBRR0L = 0x35; //9600 @ 12.5 Mhz
	sei();
}
#else

#define FOSC 8000000// Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1

void Init_Uart(void){
	cli();
	
	/* Set baud rate */
	//UBRRH = (unsigned char)((MYUBRR)>>8);
	//UBRRL = (unsigned char)MYUBRR;
	UBRRH = 0;
	UBRRL = 0x33; //9600@8Mhz
	/* Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN);
	/* Set frame format: 8data, 1stop bit */
	UCSRC = (1<<URSEL)|(3<<UCSZ0);
	
	sei();
}
#endif


/*-------------------- USART_Transmit   -------------------------
*    Function:    USART_Transmit
*    Purpose:    Transmit a byte through UART.
*
*    Parameters:
*        data - data to be sent
*    Returns: none
*------------------------------------------------------------*/

#if !defined(__AVR_ATmega8__)
void USART_Transmit(uint8_t data){
	/* Wait for empty transmit buffer */
	//    while(!(UCSR0A & (1<<UDRE0)));  // Make sure that the data register is empty before putting the device to sleep
	//        ;
	/* Put data into buffer, sends the data */
	UDR0 = data;
	/* Wait for empty transmit buffer */
	while(!(UCSR0A & (1<<UDRE0)));  // Make sure that the data register is empty before putting the device to sleep
	;
}
#else
void USART_Transmit(uint8_t data){
	/* Wait for empty transmit buffer */
	//    while(!(UCSR0A & (1<<UDRE0)));  // Make sure that the data register is empty before putting the device to sleep
	//        ;
	/* Put data into buffer, sends the data */
	UDR = data;
	/* Wait for empty transmit buffer */
	while(!(UCSRA & (1<<UDRE)));  // Make sure that the data register is empty before putting the device to sleep
	;
}
#endif
/*-------------------- USART_Receive   -------------------------
*    Function:    USART_Receive
*    Purpose:    Receive a byte from UART.
*
*    Parameters:
*        data - data pointer to save data to
*    Returns: none
*------------------------------------------------------------*/

#if !defined(__AVR_ATmega8__)
uint8_t USART_Receive(uint8_t * data)
{
	/* Wait for data to be received */
	if(UCSR0A & (1<<RXC0)){
		/* Get and return received data from buffer */
		*data = UDR0;
		return 1;
	}else
	return 0;
}
#else

uint8_t USART_Receive(uint8_t * data)
{
	/* Wait for data to be received */
	if(UCSRA & (1<<RXC)){
		/* Get and return received data from buffer */
		*data = UDR;;
		return 1;
	}else
	return 0;
}


#endif

/*-------------------- USART_print   -------------------------
*    Function:    USART_print
*    Purpose:    Print a string to UART.
*
*    Parameters:
*        text - pointer to byte array (string)
*    Returns: none
*------------------------------------------------------------*/

void USART_print(char * text){
	
	uint8_t text_size=strlen(text);
	uint8_t i;
	
	for(i=0;i<text_size;i++){
		USART_Transmit(text[i]);
	}
	USART_Transmit(0x0D);
	USART_Transmit(0x0A);
}