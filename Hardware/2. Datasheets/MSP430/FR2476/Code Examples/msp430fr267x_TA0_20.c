/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430FR267x Demo - Timer0_A3, PWM TA0.1-2, Up/Down Mode, 32kHz ACLK
//
//  Description: This program generates two PWM outputs on P1.1,2 using
//  Timer_A configured for up/down mode. The value in TACCR0, 128, defines the
//  PWM period/2 and the values in TACCR1 and TACCR2 the PWM duty cycles. Using
//  32kHz ACLK as TACLK, the timer period is 7.8ms with a 75% duty cycle on
//  P1.2 and 25% on P1.1. Normal operating mode is LPM3.
//  ACLK = TACLK = REFO = 32768Hz, MCLK = 1MHz(default)
//  //* External watch crystal on XIN XOUT is required for ACLK *//
//
//                MSP430FR2676
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |               |
//        |     P1.1/TA0.1|--> CCR1 - 75% PWM
//        |     P1.2/TA0.2|--> CCR2 - 25% PWM
//
//   Longyu Fang
//   Texas Instruments Inc.
//   August 2018
//   Built with IAR Embedded Workbench v7.12.1 & Code Composer Studio v8.1.0
//******************************************************************************
#include <msp430.h>
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                     // Stop WDT

    // Configure GPIO
    P1DIR |= BIT1 | BIT2;                     // P1.1 and P1.2 output
    P1DIR |= BIT1 | BIT2;                     // P1.1 and P1.2 output
    P1SEL1 |= BIT1 | BIT2;                    // P1.1 and P1.2 options select

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    TA0CCR0 = 128;                           // PWM Period
    TA0CCTL1 = OUTMOD_6;                     // TACCR1 toggle/set
    TA0CCR1 = 32;                            // CCR1 PWM duty cycle
    TA0CCTL2 = OUTMOD_6;                     // TACCR2 toggle/set
    TA0CCR2 = 96;                            // CCR2 PWM duty cycle
    TA0CTL = TASSEL_1 | MC__UPDOWN;          // ACLK, up-down mode

    __bis_SR_register(LPM3_bits);             // Enter LPM3
    __no_operation();                         // For debugger
}
