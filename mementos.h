/*
 * mementos.h
 *
 *  Created on: 2024¦~7¤ë4¤é
 *      Author: User
 */


#ifndef MEMENTOS_H_
#define MEMENTOS_H_

#include <msp430fr5994.h>

extern unsigned char backup;
//RESET
#define BROWNOUT_BOR    0x02
#define RST_BOR         0x04
#define PMMSW_BOR       0x06
#define LPM5_BOR        0x08
#define SECURT_BOR      0x0A
#define SVSH_POR        0x0E  // power on reset

//TIMER
#define CYCLE_TIME  2000
#define ACLK        0x0100
#define UP          0x0010

//FRAM
#define FRAM_BACKUP_START   0x43000
#define FRAM_BACKUP_END     0x43FFF

//RAM
#define RAM_START 0x1C00
#define RAM_END 0x2BFF

volatile extern int check_val;


//function
void IsReset(void);
void Hibernate(void);
void Restore(void);
void SetUp(void);
void ADC_init(void);
void init_timerB(void);
void UART_putn(long data, unsigned char ndigits);
void UART_puts(char *s);



#endif /* MEMENTOS_H_ */
