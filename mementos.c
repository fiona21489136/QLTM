/*
 * mementos.c
 *
 *  Created on: 2024年7月4日
 *      Author: User
 */

#include "mementos.h"


#pragma PERSISTENT(backup)
unsigned char backup = 0;


void IsReset(void){
    int reset = SYSRSTIV; // system reset interrupt vector
    while(reset){
        switch(reset){
            case SVSH_POR:
                break;
            case RST_BOR:
                backup = 0;
                 P1OUT |= BIT0;
                 __delay_cycles(500000);
                 P1OUT &= ~BIT0;
                 __delay_cycles(500000);
                 P1OUT |= BIT0;
                 __delay_cycles(500000);
                 P1OUT &= ~BIT0;
                 __delay_cycles(500000);
                 P1OUT |= BIT0;
                 __delay_cycles(500000);
                 P1OUT &= ~BIT0;
                 __delay_cycles(1500000);
                 break;
            case BROWNOUT_BOR:  // when the device is powering up
                if(backup)
                    Restore();
                break;
            case PMMSW_BOR:
                break;
            case LPM5_BOR:
                break;
            case SECURT_BOR:
                break;
            default :
                break;
        }
        reset = SYSRSTIV;
    }
}

void SetUp(void){
    //LED for debug
    P1OUT = 0x00;
    P1DIR |= 0xff;

    // p2.0 and p2.1 for USCI_A0 UART operation
    P2SEL0 &= ~(BIT0|BIT1);
    P2SEL1 |= BIT0 | BIT1;

    PM5CTL0 &= ~LOCKLPM5;

    //startup clock system with max DCO setting ~8MHz
    CSCTL0_H = CSKEY >> 8;  // unlock clock registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;  // set DC0 to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;  // ACLK > 10kHz
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1; // Set all dividers
    CSCTL0_H = 0; // Lock CS registers

    // Configure USCI_A0 for UART mode
    UCA0CTLW0 = UCSWRST; // enable software reset
    UCA0CTLW0 |= UCSSEL__SMCLK; // CLK = SMCLK

    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA0BR0 = 52; // 8000000/16/9600
    UCA0BR1 = 0x00;
    UCA0MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
    UCA0CTLW0 &= ~UCSWRST; // Initialize eUSCI, disable software reset
    UCA0IE |= UCRXIE; // Enable USCI_A0 RX interrupt

}

void Hibernate(void){
    //P1OUT |= BIT1;
    //copy Core register to FRAM
    // P0: PC  , P3: CGPR
    asm(" MOV.W R1, &0x42800");  // SP
    asm(" MOV R2, &0x42804");    // SR
    // skip R3
    asm(" MOV R4, &0x4280C");    // R4~R15: GPR
    asm(" MOV R5, &0x42810");
    asm(" MOV R6, &0x42814");
    asm(" MOV R7, &0x42818");
    asm(" MOV R8, &0x4281C");
    asm(" MOV R9, &0x42820");
    asm(" MOV R10, &0x42824");
    asm(" MOV R11, &0x42828");
    asm(" MOV R12, &0x4282C");
    asm(" MOV R13, &0x42830");
    asm(" MOV R14, &0x42834");
    asm(" MOV R15, &0x42838");
    // copy all the RAM onto the FRAM

    // DMA channel 1, to transfer data from RAM to FRAM.

    __data16_write_addr((unsigned short) &DMA1SA, (unsigned long) RAM_START); // source block address
    __data16_write_addr((unsigned short) &DMA1DA, (unsigned long) FRAM_BACKUP_START); // destination single address
    DMA1SZ = 0x800; // channel block size per transfer
    DMA1CTL = DMADT_5 | DMASRCINCR_3 | DMADSTINCR_3;  // auto Repeat transfer, address inc

    DMA1CTL |= DMAEN;
    DMA1CTL |= DMAREQ;  // A transfer is triggered when the DMAREQ bit is set

    backup = 1;

    // for debugging
    //P1OUT &= ~BIT1;
}

void Restore(void){
    // for debugging
    P1OUT |= BIT1;

    // Restore RAM

    // DMA2 transfer data from FRAM to RAM
    __data16_write_addr((unsigned short) &DMA2SA, (unsigned long) FRAM_BACKUP_START);
    __data16_write_addr((unsigned short) &DMA2DA, (unsigned long) RAM_START);

    DMA2SZ = 0x800;
    DMA2CTL = DMADT_5 | DMASRCINCR_3 | DMADSTINCR_3;
    DMA2CTL |= DMAEN;
    DMA2CTL |= DMAREQ;

    // core registers
    asm(" MOV.W &0x42804,R2"); //SR
    asm(" NOP");
    asm(" MOV &0x42800,R1"); // SP
    asm(" NOP");

    asm(" MOV &0x4280C,R4");
    asm(" MOV &0x42810,R5");
    asm(" MOV &0x42814,R6");
    asm(" MOV &0x42818,R7");
    asm(" MOV &0x4281C,R8");
    asm(" MOV &0x42820,R9");
    asm(" MOV &0x42824,R10");
    asm(" MOV &0x42828,R11");
    asm(" MOV &0x4282C,R12");
    asm(" MOV &0x42830,R13");
    asm(" MOV &0x42834,R14");
    asm(" MOV &0x42838,R15");

    __no_operation();
    //check_val++;
    //for debug
    P1OUT &= ~BIT1;
}

void ADC_init(void);

void init_timerB(void){
    TB0CTL = TBCLR;
    TB0R = 0;
    TB0CCR0 = CYCLE_TIME;  // trigger hibernate
    TB0CTL = TASSEL_1 | MC_1;
    TB0CCTL0 |= CCIE;
    __bis_SR_register(GIE);  // enable general interrupt
    __no_operation(); // delay 1 cycle
}

void UART_putn(long data, unsigned char ndigits){
    unsigned char sign, s[6];
    unsigned int i;
    sign = ' ';
    if(data < 0) {
        sign='-';
        data = -data;
    }
    i = 0;
    do {
        s[i++] = data % 10 + '0';
        if(i == ndigits) {
            s[i++]='.';
        }
    } while( (data /= 10) > 0);

    s[i] = sign;

    do {
        while (!(UCTXIFG&UCA0IFG));
        UCA0TXBUF = s[i];
    } while(i--);

}


void UART_puts(char *s){

    while ((UCA0STATW & UCBUSY));  // UART busy

    // after non-busy, put the string 's' into UART buffer
    while(*s){
        while(!(UCTXIFG&UCA0IFG));  // USCI_A0 TX buffer ready?
        UCA0TXBUF = *s++;
    }
}

volatile int check_val = 0;

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void)
#elif defined(__GNUC__)
void __attribute__((interrupt(TIMER0_B0_VECTOR))) Timer0_B0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    check_val++;
    Hibernate();
}

