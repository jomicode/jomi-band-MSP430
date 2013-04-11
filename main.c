#include <msp430.h>
#include <stdint.h>

#define LED1    BIT0
#define LED2    BIT1
#define LED3    BIT3
#define LED4    BIT4
#define BTN1    BIT2
#define ACC1	BIT5
#define SCL		BIT6
#define	SDA		BIT7

void roll(void);
void blink(void);
uint16_t c=0;								//number for reset button
uint16_t LEDi[4]={LED1, LED2, LED3, LED4};
uint16_t LEDi2[4]={LED1, LED1+LED2, LED1+LED2+LED3, LED1+LED2+LED3+LED4};
uint16_t wdt_th=0, wdt_cnt=0;				//Watchdog timer
uint16_t btn_cnt = 0;						//Button pushes
uint16_t blink_15min_flag = 0, blink_30min_flag = 0, blink_45min_flag = 0, blink_60min_flag = 0;	//Interval flags
uint16_t day_start = 0, day_start_cnt = 0, start_flag = 0, btn_off_flag = 0;						//Day start and button off flags

//Watchdog timer - 15bit max 32768
#pragma vector=WDT_VECTOR
__interrupt void wdt_int(void) {
  wdt_cnt++;
  if(start_flag == 1){				//If first initialization is done
	  day_start_cnt++;				//Increase 12h counter
	  if(day_start & 1 == 1){		//If need to roll

		  if(day_start_cnt == 150){ //10800
			  day_start_cnt = 0;			//Evening is coming
			  day_start++;					//Day start is 2 - even - don't roll
		  }
	  }
	  else{									//If day start is even - don't roll
		  if(day_start_cnt == 150){
			  day_start_cnt = 0;
			  day_start++;
		  }
	  }
  }
  if (wdt_cnt>wdt_th) {
	wdt_cnt=0;
	__low_power_mode_off_on_exit();
  }
 }

#pragma vector=PORT1_VECTOR
__interrupt void Comm_port_interrupt(void) {

	if (P1IFG&BTN1 == 0){
	    return;
	  }
	  P1IFG &= ~BTN1;
	  __delay_cycles(2400); // 20 ms
	  c=0;
	  //start of the day, 3sec hold!!!!!!!!!!!!!
	  while(c<20000){
		  if ((~P1IN) & BTN1 & P1IES){
				  c++;
				  if(c == 19999){
					  start_flag = 1;
					  day_start = 1;		//wdt_th = 10800;
					  day_start_cnt = 0;
					  blink();
				  }
		  }else{
			     break;
			}
	  }
	  if(start_flag == 1){
		  if(day_start & 1 == 1){
			  btn_cnt++;
			  blink_15min_flag = 0;
			  blink_30min_flag = 0;
			  blink_45min_flag = 0;
			  blink_60min_flag = 0;
			  btn_off_flag = 0;

			  if(btn_cnt == 1){
				  wdt_th = 1;				//
				  wdt_cnt = wdt_th - 1;		//wdt_th is less than wdt_th for giving 4sec time for the next press
				  blink_15min_flag = 1;
				  P1OUT |= LED1;
			  }
			  else if(btn_cnt == 2){
				  wdt_th++;
				  wdt_cnt = wdt_th - 1;
				  blink_30min_flag = 1;
				  P1OUT |= LED2;
			  }
			  else if(btn_cnt == 3){
				  wdt_th++;
				  wdt_cnt = wdt_th - 1;
				  blink_45min_flag = 1;
				  P1OUT |= LED3;
			  }
			  else if(btn_cnt == 4){
				  wdt_th++;
				  wdt_cnt = wdt_th - 1;
				  blink_60min_flag = 1;
				  P1OUT |= LED4;
			  }
			  else if(btn_cnt == 5){
				  P1OUT &= ~(LED1+LED2+LED3+LED4);
				  btn_cnt = 0;
				  btn_off_flag = 1;
			  }
		  }
	  }
}


void blink(void){
  P1OUT |=LED1+LED2+LED3+LED4;
  __delay_cycles(300); //2.4 ms
  P1OUT &= ~(LED1+LED2+LED3+LED4);
}

void roll(void){
  uint16_t j=0;

  for (j=0;j<4;j++){
    P1OUT|=LEDi[j];
    __delay_cycles(400); //3.2 ms
    P1OUT &= ~LEDi[j];
    __delay_cycles(6000); //48ms
  }
}  


void main(void){
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  P1OUT=BTN1;
  P1REN |= BTN1; //port 1 resistor enable
  P1IES|=BTN1;	//port 1 interrupt edge select
  P1DIR=LED1+LED2+LED3+LED4; //LEDs - output

  BCSCTL1 = CALBC1_1MHZ+DIVA_2+XT2OFF;
  DCOCTL = CALDCO_1MHZ;
  BCSCTL2 = DIVM_3+DIVS_3;
  BCSCTL3 |= LFXT1S_2; //swithes on the ACLK by sourcing it to the VLO
  __delay_cycles(625); // wait 5 ms

  WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTSSEL + WDTIS0; //ACLK/32768 = 4s
  IE1 |= WDTIE;
  while (1) {
	  if(day_start & 1 == 1){
    		btn_cnt = 0;

    		if(blink_15min_flag == 1){

    		    	wdt_th = 50;
    		    	roll();
    		    }
    		if(blink_30min_flag == 1){
    			roll();
    		    	wdt_th = 75;
    		    }
    		if(blink_45min_flag == 1){
    			roll();
    		    	wdt_th = 100;
    		    }
    		if(blink_60min_flag == 1){
    			roll();
    		    	wdt_th = 125;
    		    }
    		if(btn_off_flag == 1){
    			wdt_th = 0;
    		}
    		P1IE |= BTN1;
    		__low_power_mode_3();
    		P1IE &= ~BTN1;
	  }else{
    	//}
    	P1IE |= BTN1;
        __low_power_mode_3();
        P1IE &= ~BTN1;
	  }
	  }
}
