#include <msp430.h>
#include <./HAL_BOARD/HAL_BOARD.h>

void Software_Trim();                        // Software Trim to get the best DCOFTRIM value

void Watchdog_Init() {
    WDTCTL = WDTPW | WDTHOLD;   // Stop watch-dog timer
}

void Oscillator_Init() {
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    do {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }
    while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

    __bis_SR_register(SCG0);                     // disable FLL
    CSCTL3 |= SELREF__XT1CLK;                    // Set XT1 as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_5;                    // DCOFTRIM=5, DCO Range = 16MHz
    CSCTL2 = FLLD_0 + 487;                       // DCOCLKDIV = 16MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                     // enable FLL
    Software_Trim();                             // Software Trim to get the best DCOFTRIM value

    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;   // set XT1 (~32768Hz) as ACLK source, ACLK = 32768Hz
                                                // default DCOCLKDIV as MCLK and SMCLK source
}

void GPIOs_Init() {
    /*  FIRST STEP: All pins as GPIO, OUTPUT at LOW level (LPM Optimization)  */
    P1DIR = 0xFF;  //All pins as OUTPUT
    P1SEL0 = 0x00;  //All pins as GPIO
    P1OUT = 0x00;  //All pins as LOW

    P2DIR = 0xFF;  //All pins as OUTPUT
    P2SEL0 = 0x00;  //All pins as GPIO
    P2OUT = 0x00;  //All pins as LOW

    P3DIR = 0xFF;  //All pins as OUTPUT
    P3SEL0 = 0x00;  //All pins as GPIO
    P3OUT = 0x00;  //All pins as LOW

    P4DIR = 0xFF;  //All pins as OUTPUT
    P4SEL0 = 0x00;  //All pins as GPIO
    P4OUT = 0x00;  //All pins as LOW

    P5DIR = 0xFF;  //All pins as OUTPUT
    P5SEL0 = 0x00;  //All pins as GPIO
    P5OUT = 0x00;  //All pins as LOW

    P6DIR = 0xFF;  //All pins as OUTPUT
    P6SEL0 = 0x00;  //All pins as GPIO
    P6OUT = 0x00;  //All pins as LOW

    /*  SECOND STEP: INPUT PINS  */
    P4DIR &= ~GPIO_ROTARY_ENCODER_BUTTON;
    P4DIR &= ~GPIO_ROTARY_ENCODER_SIGNAL_A;
    P4DIR &= ~GPIO_ROTARY_ENCODER_SIGNAL_B;
    P4DIR &= ~GPIO_USB_DETECT;
    P4DIR &= ~GPIO_SDCARD_DETECT;
    P4DIR &= ~GPIO_EXTERNAL_SUPPLY_DETECT;

    /*  THIRD STEP: SPECIAL FUNCTIONS  */		
    P2SEL0  |= XTAL_IN | XTAL_OUT;                           //Low Frequency Crystal
	
	P1SEL0  |= ADC_BATTERY_VOLTAGE;                          //Battery Voltage Monitor (ADC)
	P1SEL1  |= ADC_BATTERY_VOLTAGE;

	P2SEL0  |= UART_UCA1_RX;                                 //Data logger (UART)
	
	SYSCFG3 |= USCIA0RMP;                       			 //Set the remapping source
	P1SEL0  |= SPI_UCA0_SIMO | SPI_UCA0_SOMI | SPI_UCA0_CLK; //SD Card (SPI)
	
	SYSCFG3 |= USCIB0RMP;                       			 //Set the remapping source
	P1SEL0  |= I2C_UCB0_SDA | I2C_UCB0_SCL;                  //RTC (I2C)
	
	SYSCFG3 |= USCIB1RMP;                       			 //Set the remapping source
	P4SEL0  |= SPI_UCB1_SIMO | SPI_UCB1_SOMI; //OLED Display (SPI)
	P5SEL0  |= SPI_UCB1_CLK;
	
    PM5CTL0 &= ~LOCKLPM5; // GPIO High-Impedance OFF
}

void GPIO_Interrupt_Init() {    
    //P1IES |= BIT2;   // Select the wake-up edge trigger (0: low-to-high | 1: high-to-low)
    //P1IE |= BIT2;   // Set the interrupt enable
    //P1IFG &= ~BIT2;    
}

void ADC_Init() {
    ADCCTL0 &= ~ADCENC;               // Disable ADC conversion (needed for the next steps)
    ADCMCTL0 |= ADCSREF_3;            // VR+ = VeREF+ and VR– = AVSS
    ADCMCTL0 |= ADCINCH_11;           // A11 as the highest channel for a sequence of conversions (goes down from A11)
    ADCCTL0 |= ADCSHT_10;            // ADC sample-and-hold time = 512 ADCCLK cycles
    ADCCTL0 |= ADCON;                // ADC on
    ADCCTL0 |= ADCMSC;               // ADC sample-and-conversions are performed automatically as soon as the prior conversion is completed (sequential mode)
    ADCCTL1 |= ADCSHS_0;             // ADC sample-and-hold source = ADCSC bit
    ADCCTL1 |= ADCSHP;               // ADC sample-and-hold pulse-mode select = SAMPCON signal is sourced from the sampling timer
    ADCCTL1 |= ADCCONSEQ_1;          // ADC conversion sequence mode = Sequence-of-channels
    ADCCTL2 &= ~ADCRES;               // ADC resolution = 12 bit (14 clock cycle conversion time)
    ADCCTL2 |= ADCRES_2;             // ADC resolution = 12 bit (14 clock cycle conversion time)
    ADCIE |= ADCIE0;               // ADC interrupts enable
}

void Software_Trim() {
	unsigned int oldDcoTap = 0xffff;
	unsigned int newDcoTap = 0xffff;
	unsigned int newDcoDelta = 0xffff;
	unsigned int bestDcoDelta = 0xffff;
	unsigned int csCtl0Copy = 0;
	unsigned int csCtl1Copy = 0;
	unsigned int csCtl0Read = 0;
	unsigned int csCtl1Read = 0;
	unsigned int dcoFreqTrim = 3;
	unsigned char endLoop = 0;

	do {
		CSCTL0 = 0x100;                         // DCO Tap = 256
		do {
			CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
		} while (CSCTL7 & DCOFFG);               // Test DCO fault flag

		__delay_cycles((unsigned int) 3000 * 16);               // Wait FLL lock status (FLLUNLOCK) to be stable
																		   // Suggest to wait 24 cycles of divided FLL reference clock
		while ((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0))
			;

		csCtl0Read = CSCTL0;                   // Read CSCTL0
		csCtl1Read = CSCTL1;                   // Read CSCTL1

		oldDcoTap = newDcoTap;                 // Record DCOTAP value of last time
		newDcoTap = csCtl0Read & 0x01ff;       // Get DCOTAP value of this time
		dcoFreqTrim = (csCtl1Read & 0x0070) >> 4;       // Get DCOFTRIM value

		if (newDcoTap < 256)                    // DCOTAP < 256
				{
			newDcoDelta = 256 - newDcoTap;     // Delta value between DCPTAP and 256
			if ((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
				endLoop = 1;                   // Stop while loop
			else {
				dcoFreqTrim--;
				CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
			}
		} else                                   // DCOTAP >= 256
		{
			newDcoDelta = newDcoTap - 256;     // Delta value between DCPTAP and 256
			if (oldDcoTap < 256)                // DCOTAP cross 256
				endLoop = 1;                   // Stop while loop
			else {
				dcoFreqTrim++;
				CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
			}
		}

		if (newDcoDelta < bestDcoDelta)         // Record DCOTAP closest to 256
				{
			csCtl0Copy = csCtl0Read;
			csCtl1Copy = csCtl1Read;
			bestDcoDelta = newDcoDelta;
		}

	} while (endLoop == 0);                      // Poll until endLoop == 1

	CSCTL0 = csCtl0Copy;                       // Reload locked DCOTAP
	CSCTL1 = csCtl1Copy;                       // Reload locked DCOFTRIM
	while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1))
		; // Poll until FLL is locked
}
