/*
 * Lesson C1: Timer Interrupt - Toggle BLED [PF2]
 * Prof. Dr. Cl�riton Bernadelli
 * Electrical engineering - Federal University of Tri�ngulo Mineiro � UFTM
 * Signal processing group - GPS - UFTM - Brazil
 * Version jan/2021  - github.com/bernadelli/pds
 */

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include <my_def.h>
#include <stdbool.h>
#include "sysctl.h"


// Delay function
void delay(unsigned long count) {
   unsigned long i=0;
   for(i=0; i<count; i++);
}


// Initialization and configuration according data sheet (5.3)
void SetSystemClock(unsigned int SYSDIV2) {
    // 1. Enable USERCC2 in RCC2
    // RCC2[31]=USERCC2 // SYSCTL_RCC2_USERCC2=0x80000000 // RCC2->USERCC2->1
    SYSCTL_RCC2_R |= SYSCTL_RCC2_USERCC2; // 1=RCC2 override RCC

    // 2. Bypass PLL while initializing. RAW Clock source
    // RCC2[11]=BYPASS2 // SYSCTL_RCC2_BYPASS2=0x00000800 // RCC2->BYPASS2->1
    SYSCTL_RCC2_R |= SYSCTL_RCC2_BYPASS2; // 1=Clock OSC
    // RCC[22]=USESYSDIV // SYSCTL_RCC_USESYSDIV=0x00400000 // RCC->USESYSDIV->0
    // SYSCTL_RCC_R &= (~0x00400000); 0 on reset

    // 3. Set external crystal value and oscillator source for PLL
    // RCC[10:6]=XTAL // SYSCTL_RCC_XTAL_M=0x000007C0 // RCC->XTAL->0
    // SYSCTL_RCC_XTAL_16MHZ=0x00000540 // RCC->XTAL->0x15
    SYSCTL_RCC_R = (SYSCTL_RCC_R & ~SYSCTL_RCC_XTAL_M); //Clear RCC[10:6]=XTAL
    SYSCTL_RCC_R |= SYSCTL_RCC_XTAL_16MHZ;      // 16MHz external xtal
    // RCC2[6:4]=OSCSRC2 // SYSCTL_RCC2_OSCSRC2_M=0x00000070 // RCC2->OSCSRC2->0x0
    SYSCTL_RCC2_R &= ~SYSCTL_RCC2_OSCSRC2_M;//  000=Main Oscillator

    // 4. Clearing PWRDN to active PLL
    // RCC2[13]=PWRDN // SYSCTL_RCC_PWRDN=0x00002000 // RCC2->PWRDN->0x0
    SYSCTL_RCC2_R &= ~SYSCTL_RCC_PWRDN; //0=The PLL operate normally

    // 5. Set system clock
    //Set the USESYSDIV bit in RCC
    // RCC[22]=USESYSDIV // SYSCTL_RCC_USESYSDIV=0x00400000 // RCC->USESYSDIV->1
    // The system clock divider is forced to be used when the PLL is selected as the source.
    SYSCTL_RCC_R |= SYSCTL_RCC_USESYSDIV;  // 1=Divider is the source for the system clock
    // RCC2[30]=DIV400 // SYSCTL_RCC2_DIV400=0x40000000 // RCC2->DIV400->0x1
    SYSCTL_RCC2_R |= SYSCTL_RCC2_DIV400; //1=Select 400MHz(from PLL)
    // RCC2[28:23]=SYSDIV2 // SYSCTL_RCC2_SYSDIV2_M=0x1F800000 // RCC2->SYSDIV->0x02 // Table 5-6
    // RCC2[22]=SYSDIV2LSB // SYSCTL_RCC2_SYSDIV2LSB=0x00400000 // RCC2->SYSDIV2LSB->0 // Table 5-6
    SYSCTL_RCC2_R = (SYSCTL_RCC2_R & ~SYSCTL_RCC2_SYSDIV2_M);
    SYSCTL_RCC2_R |=SYSDIV2;
    SYSCTL_RCC2_R &= ~SYSCTL_RCC2_SYSDIV2LSB;


    // 6. Wait for PLL to lock by polling PLLLRIS
    // RIS[6]=PLLLRIS // SYSCTL_RIS_PLLLRIS=0x00000040 // RIS->PLLLRIS->0x1
    while( (SYSCTL_RIS_R & SYSCTL_RIS_PLLLRIS) == 0) {};

    // 7. Set BYPASS to 0, select PLL as the source of system clock
    // RCC2[11]=BYPASS2 // SYSCTL_RCC2_BYPASS2=0x00000800 // RCC2->BYPASS2->0x0
    SYSCTL_RCC2_R &= ~SYSCTL_RCC2_BYPASS2; // 0=Clock PLL
}


//Timer1 configurations
//Initialization and configuration according data sheet - Section 11.4
void Time1A_1sec_delay(void)
{
    //Enable clock Timer1 sub-timer A in run mode
    SYSCTL_RCGCTIMER_R |= (1<<1);
    //1. Optionally
    //2. Disable timer1 output
    TIMER1_CFG_R = 0;
    //Select 16-bit configuration option
    TIMER1_CFG_R = 0x4;
    //3. Select periodic down counter mode of timer1
    TIMER1_TAMR_R = 0x02;
    //TimerA prescaler value
    TIMER1_TAPR_R = 250-1;
    // 4. Optionally
    //5.TimerA counter starting count down value
    TIMER1_TAILR_R = 50000-1 ;
    //TimerA timeout flag bit clears
    TIMER1_ICR_R = 0x1;
    //6. Enables TimerA time-out  interrupt mask
    TIMER1_IMR_R |=(1<<0);
    //7. Enable TimerA module
    TIMER1_CTL_R |= 0x01;
    //Enable IRQ21 */
    NVIC_EN0_R |= (1<<21);
}


//Timer1 sub-timer. A interrupt service routine
TIMER1A_Handler()
{
    //8. Interrupt
      if(TIMER1_MIS_R & 0x1)
          GPIO_PORTF_DATA_R  ^= PF2; /* toggle Blue LED*/
          TIMER1_ICR_R = 0x1;        /* Timer1A timeout flag bit clears*/
}


void UART5_Transmitter(char data)
{
    while((UART5_FR_R & (1<<5)) != 0);     /* wait until Tx buffer not full */
    UART5_DR_R = data;                  /* before giving it another byte */
}


char UART5_Receiver(void)
{
    char data;
      while((UART5_FR_R & (1<<4)) != 0); /* wait until Rx buffer is not full */
    data = UART5_DR_R ;      /* before giving it another byte */
    return (unsigned char) data;
}



void printstring(char *str)
{
  while(*str)
    {
        UART5_Transmitter(*(str++));
    }
}

