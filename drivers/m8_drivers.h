/*
 * m8_drivers.h
 *
 * Created: 4/24/2014 8:47:17 PM
 *  Author: John
 */ 


#ifndef M8_DRIVERS_H_
#define M8_DRIVERS_H_

void Init_Uart(void);
void USART_Transmit(uint8_t data);
uint8_t USART_Receive(uint8_t * data);
void USART_print(char * text);

#endif /* M8_DRIVERS_H_ */