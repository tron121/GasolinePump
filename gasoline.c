/***************************************************/

#include <msp430.h>

#include <stdlib.h>

#include <stdio.h>

#include <string.h>

#include "peripherals.h"

#define ARRAYSIZE(x) sizeof(arr) / sizeof(arr[0])

// Function Prototypes
void swDelay(char numLoops);
void configLaunchpadButtons();
void configLabBoardButtons();
void configTimerA2();
__interrupt void Timer_A2_ISR(void);
char getLabBoardButtons();
char * priceToASCII(long unsigned int price);
char * priceToASCII2(long unsigned int price);
void priceToASCII3(long unsigned int price, char * string);
char * centiGallonsToASCII(long unsigned int centiG);
char * centiGallonsToASCII2(long unsigned int centiG);
void centiGallonsToASCII3(long unsigned int centiG, char * string);
char * stripPreceedingZeros(char * string, int length);
long unsigned int exp(unsigned int base, char exponent);
char arrayCmp(char array1[], char array2[], int length2);

// Declared globals
char pin1[8] = "12345678"; //{'1', '2', '3', '4', '5', '6', '7', '8'};
const char pin2[8] = "87654321";
char output[12];
volatile long unsigned int timeCount = 0;

typedef enum {
  INITIAL,
  SELECT,
  PUMP,
  PUMP_INT,
  PAY,
  UNPAYED
}
state;
typedef enum {
  DIESEL,
  SUPER,
  PREMIUM,
  REGULAR
}
gas;
state mState = INITIAL;
char mask;

// Main
void main(void) {
  WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer

  initLeds();
  configDisplay();
  configKeypad();

  configLaunchpadButtons();
  configLabBoardButtons();
  configTimerA2();
  _BIS_SR(GIE);

  state mState = INITIAL;
  GrClearDisplay( & g_sContext); // Clear the display

  char currKey;
  char pricePerGallon;
  char pinSequence[8], pini, seqerror;
  char transferingState = 1;
  char * printString;
  long unsigned int timePumped, loopTimer, prevTimeCount, currTimeCount;
  gas mGas;

  while (1) {
    switch (mState) {
    case INITIAL:
      if (transferingState) {
        GrClearDisplay( & g_sContext);
        GrStringDrawCentered( & g_sContext, "Welcome to", AUTO_STRING_LENGTH, 48, 15, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "OCTAN", AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "Press * to begin", AUTO_STRING_LENGTH, 48, 45, OPAQUE_TEXT);
        GrFlush( & g_sContext);
        transferingState = 0;
      }
      currKey = getKey();
      if (currKey == '*') {
        mState = SELECT;
        GrClearDisplay( & g_sContext);
        timePumped = 0;
        loopTimer = 0;
        transferingState = 1;
      }
      break;
    case SELECT:
      if (transferingState) {
        GrStringDrawCentered( & g_sContext, "Select Grade", AUTO_STRING_LENGTH, 48, 15, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "(1) Diesel", AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "(2) Super", AUTO_STRING_LENGTH, 48, 45, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "(3) Premium", AUTO_STRING_LENGTH, 48, 55, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "(4) Regular", AUTO_STRING_LENGTH, 48, 65, OPAQUE_TEXT);
        GrFlush( & g_sContext);
        transferingState = 0;
      }

      switch (getLabBoardButtons()) {
      case BIT0:
        // button 1 pressed
        pricePerGallon = 230;
        mGas = DIESEL;
        mState = PUMP;
        GrClearDisplay( & g_sContext);
        transferingState = 1;
        break;
      case BIT1:
        // button 2 pressed
        pricePerGallon = 235;
        mGas = SUPER;
        mState = PUMP;
        GrClearDisplay( & g_sContext);
        transferingState = 1;
        break;
      case BIT2:
        // button 3 pressed
        pricePerGallon = 220;
        mGas = PREMIUM;
        mState = PUMP;
        GrClearDisplay( & g_sContext);
        transferingState = 1;
        break;
      case BIT3:
        // button 4 pressed
        pricePerGallon = 210;
        mGas = REGULAR;
        mState = PUMP;
        GrClearDisplay( & g_sContext);
        transferingState = 1;
        break;
      default:
        break;
      }
      break;
    case PUMP:
      if (mGas == SUPER || mGas == PREMIUM || mGas == REGULAR && transferingState) {
        // gas; watch button 1
        GrStringDrawCentered( & g_sContext, "Pumping Gas", AUTO_STRING_LENGTH, 48, 15, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "Gallons pumped: ", AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
        mask = BIT0;
        GrFlush( & g_sContext);
        transferingState = 0;
      } else if (transferingState) {
        // diesel; watch button 2
        GrStringDrawCentered( & g_sContext, "Pumping diesel", AUTO_STRING_LENGTH, 48, 15, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "Gallons pumped: ", AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
        mask = BIT1;
        GrFlush( & g_sContext);
        transferingState = 0;
      }

      /*if(getLabBoardButtons() == mask) {
    				timeCount = 0;
    				long unsigned int prevTimeCount = 0;

    				while(getLabBoardButtons() == mask) {
    					long unsigned int currTimeCount = timeCount;
    					timePumped += currTimeCount - prevTimeCount;
    					prevTimeCount = currTimeCount;

    					if(timePumped % 200 == 0) {
    						//printString = centiGallonsToASCII2(timePumped / 80);
    						//snprintf(printString, 10, "%l", timePumped / 80);
    						centiGallonsToASCII3(timePumped / 80, printString);
							GrStringDrawCentered(&g_sContext, printString, AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
							GrFlush(&g_sContext);
							//free(printString);
    					}
    				}
    				loopTimer = timeCount;
    			}*/

      if (timeCount - loopTimer > 1000 && loopTimer > 0) {
        mState = PAY;
        GrClearDisplay( & g_sContext);
        transferingState = 1;
      }

      /*if(readIn == mask) {
    				if(timePumped == 0 || prevButton != mask) {
    					timeCount = 0;
    				}

    				GrStringDrawCentered(&g_sContext, "Pump pressed", AUTO_STRING_LENGTH, 48, 55, OPAQUE_TEXT);
    				timePumped += timeCount;
    			} else GrStringDrawCentered(&g_sContext, "Not  pressed", AUTO_STRING_LENGTH, 48, 55, OPAQUE_TEXT);

    			if(timePumped % 53 == 0) {
					GrStringDrawCentered(&g_sContext, "Gallons pumped: ", AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
					GrStringDrawCentered(&g_sContext, centiGallonsToASCII2(timePumped), AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
				}

    			prevButton = readIn;
    			GrFlush(&g_sContext);*/
      break;
    case PUMP_INT:
      if (transferingState) {
        prevTimeCount = timeCount;
      }
      currTimeCount = timeCount;
      timePumped += currTimeCount - prevTimeCount;
      prevTimeCount = currTimeCount;
      loopTimer = timeCount;

      if (timePumped % 200 == 0) {
        centiGallonsToASCII3(timePumped / 80, printString);
        GrStringDrawCentered( & g_sContext, printString, AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
        GrFlush( & g_sContext);
      }
      case PAY:
        //printString = priceToASCII(timePumped * pricePerGallon / 8000);
        priceToASCII3(timePumped * pricePerGallon / 8000, printString);
        GrStringDrawCentered( & g_sContext, "You owe: ", AUTO_STRING_LENGTH, 48, 15, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, printString, AUTO_STRING_LENGTH, 48, 25, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "Enter your pin", AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
        GrStringDrawCentered( & g_sContext, "to pay", AUTO_STRING_LENGTH, 48, 45, OPAQUE_TEXT);
        GrFlush( & g_sContext);
        free(printString);
        pini = 0;
        seqerror = 0;
        loopTimer = timeCount;
        while (mState == PAY) {
          currKey = getKey();
          if (currKey && pini < 8) {
            pinSequence[pini] = currKey;
            pini++;
            GrStringDrawCentered( & g_sContext, pinSequence, pini, 48, 55, OPAQUE_TEXT);
            GrStringDrawCentered( & g_sContext, "          ", 10, 48, 65, OPAQUE_TEXT);
            GrFlush( & g_sContext);
            while (getKey() != 0);
            loopTimer = timeCount;
          }

          if (pini == 8) {
            if (arrayCmp(pinSequence, pin1, 8) || arrayCmp(pinSequence, pin2, 8)) {
              mState = INITIAL;
              transferingState = 1;
            } else {
              seqerror++;
              pini = 0;
              GrStringDrawCentered( & g_sContext, "        ", 8, 48, 55, OPAQUE_TEXT);
              GrStringDrawCentered( & g_sContext, "Try Again!", AUTO_STRING_LENGTH, 48, 65, OPAQUE_TEXT);
              GrFlush( & g_sContext);
            }
          }

          if (seqerror >= 3 || timeCount - loopTimer > 10000) {
            mState = UNPAYED;
            transferingState = 1;
          }
        }
        break;
      case UNPAYED:
        if (transferingState) {
          BuzzerOn();
          GrClearDisplay( & g_sContext);
          GrStringDrawCentered( & g_sContext, "Enter pin to", AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
          GrStringDrawCentered( & g_sContext, "stop alarm", AUTO_STRING_LENGTH, 48, 45, OPAQUE_TEXT);
          GrFlush( & g_sContext);
          transferingState = 0;
          pini = 0;
        }
        currKey = getKey();

        if (currKey && pini < 8) {
          pinSequence[pini] = currKey;
          pini++;
          GrStringDrawCentered( & g_sContext, pinSequence, pini, 48, 55, OPAQUE_TEXT);
          GrStringDrawCentered( & g_sContext, "          ", 10, 48, 65, OPAQUE_TEXT);
          GrFlush( & g_sContext);
          while (getKey() != 0);
        }

        if (pini == 8) {
          if (arrayCmp(pinSequence, pin1, 8) || arrayCmp(pinSequence, pin2, 8)) {
            mState = INITIAL;
            BuzzerOff();
            setLeds(0);
            transferingState = 1;
          } else {
            pini = 0;
            GrStringDrawCentered( & g_sContext, "        ", 8, 48, 55, OPAQUE_TEXT);
            GrStringDrawCentered( & g_sContext, "Try Again!", AUTO_STRING_LENGTH, 48, 65, OPAQUE_TEXT);
            GrFlush( & g_sContext);
          }
        }

        if (timeCount % 200 == 0) {
          setLeds(rand() % 16);
        }

        break;
      default:
        mState = INITIAL;
        break;
    }
    //setLeds(getLabBoardButtons());
  }

}

void swDelay(char numLoops) {
  // This function is a software delay. It performs
  // useless loops to waste a bit of time
  // Input: numLoops = number of delay loops to execute
  // Output: none
 

  volatile unsigned int i, j; // volatile to prevent optimization
  // by compiler

  for (j = 0; j < numLoops; j++) {
    i = 50000; // SW Delay
    while (i > 0) // could also have used while (i)
      i--;
  }
}

void configLaunchpadButtons() {
  // Configure P1.1 and P2.1 for digital I/O
  P1SEL = P1SEL & ~BIT1;
  P2SEL = P2SEL & ~BIT1;

  // Configure P1.1 and P2.1 as inputs
  P1DIR = P1DIR & ~BIT1;
  P2DIR = P2DIR & ~BIT1;

  // Configure pull-up resistors for P1.1 and P2.1
  P1REN = P1REN | BIT1;
  P2REN = P2REN | BIT1;

  P1OUT = P1OUT | BIT1;
  P2OUT = P2OUT | BIT1;

  //Enable port interrupts
  P1IE = P1IE | BIT1;
  P2IE = P2IE | BIT1;
  P1IFG &= ~BIT1; // use as a switch
  P2IFG &= ~BIT1; // use as a switch
}

void configLabBoardButtons() {
  // Configure P7.0, P3.6, P2.2, and P7.4 for digital I/O
  P2SEL = P2SEL & ~BIT2;
  P3SEL = P3SEL & ~BIT6;
  P7SEL = P7SEL & ~(BIT0 | BIT4);

  // Configure P2.2, P3.6, P7.0, and P7.4 as inputs
  P2DIR = P2DIR & ~BIT2;
  P3DIR = P3DIR & ~BIT6;
  P7DIR = P7DIR & ~(BIT0 | BIT4);

  // configure pull-up resistors for P2.2, P3.6, P7.0, and P7.4
  P2REN = P2REN | BIT2;
  P3REN = P3REN | BIT6;
  P7REN = P7REN | (BIT0 | BIT4);

  P2OUT = P2OUT | BIT2;
  P3OUT = P3OUT | BIT6;
  P7OUT = P7OUT | (BIT0 | BIT4);
}

void configTimerA2() {
  TA2CTL = TASSEL_1 + MC_1 + ID_0;

  TA2CCR0 = 32; // 32 + 1 ACLK ticks = ~ 0.001 seconds

  TA2CCTL0 = CCIE; // enable interrupt
}

#pragma vector = TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
  timeCount++;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT_1(void) {
  if (mState == PUMP && mask == BIT2) {
    mState = PUMP_INT;
  } else if (mState == PUMP_INT && mask == BIT2) {
    mState = PUMP;
  }
}

#pragma vector = PORT2_VECTOR
__interrupt void PORT_2(void) {
  if (mState == PUMP && mask == BIT1) {
    mState = PUMP_INT;
  } else if (mState == PUMP_INT && mask == BIT1) {
    mState = PUMP;
  }
}

char getLabBoardButtons() {
  char s1 = ~P7IN & BIT0;
  char s2 = (~P3IN & BIT6) >> 5;
  char s3 = ~P2IN & BIT2;
  char s4 = (~P7IN & BIT4) >> 1;

  char output = s1 | s2 | s3 | s4;
  return output;
}

// price is in cents
char * priceToASCII(long unsigned int price) {
  char numdigits;
  long unsigned int mPrice = price;
  int i;
  for (i = 9; i >= 0; i--) {
    if (mPrice / exp(10, i)) {
      numdigits = i + 1;
      break;
    }
  }
  if (numdigits == 0) {
    return "$0.00";
  } else {
    char * digits = malloc(numdigits * sizeof(char));
    for (i = 0; i < numdigits; i++) {
      int poweroften = exp(10, numdigits - 1 - i);
      //int digit = mPrice / poweroften;
      int digit = (mPrice % (10 * poweroften)) / poweroften;
      digits[i] = digit;
      //mPrice = mPrice - digit * poweroften;
    }
    char * output = malloc((numdigits + 3) * sizeof(char));
    output[0] = '$';
    for (i = 1; i < numdigits - 1; i++) {
      output[i] = digits[i - 1] + 0x30;
    }
    output[numdigits - 1] = '.';
    output[numdigits] = digits[numdigits - 2] + 0x30;
    output[numdigits + 1] = digits[numdigits - 1] + 0x30;
    output[numdigits + 2] = '\0';
    free(digits);
    return output;
  }
}

char * priceToASCII2(long unsigned int price) {
  char * workingString = stripPreceedingZeros(centiGallonsToASCII2(price), 12);
  char * returnString = malloc(strlen(workingString) + sizeof(char));

  int i;
  returnString[0] = '$';
  for (i = 0; i < strlen(workingString); i++) {
    returnString[i + 1] = workingString[i];
  }

  if (returnString)

    free(workingString);
  return returnString;
}

void priceToASCII3(long unsigned int price, char * string) {
  long unsigned int intPart = price / 100;
  unsigned int decimalPart1 = (price - intPart * 100) / 10;
  unsigned int decimalPart2 = price - intPart * 100 - decimalPart1 * 10;
  sprintf(string, "  $%lu.%u%u  ", intPart, decimalPart1, decimalPart2);
}

char * centiGallonsToASCII(long unsigned int centiG) {
  char numdigits;
  long unsigned int mCentiG = centiG;
  int i;
  for (i = 9; i >= 0; i--) {
    if (mCentiG / exp(10, i)) {
      numdigits = i + 1;
      break;
    }
  }
  if (numdigits == 0) {
    return "0.00";
  } else {
    char * digits = malloc(numdigits * sizeof(char));
    for (i = 0; i < numdigits; i++) {
      int poweroften = exp(10, numdigits - 1 - i);
      int digit = mCentiG / poweroften;
      digits[i] = digit;
      mCentiG = mCentiG - digit * poweroften;
    }
    char * output = malloc((numdigits + 2) * sizeof(char));
    for (i = 0; i < numdigits - 2; i++) {
      output[i] = digits[i] + 0x30;
    }
    output[numdigits - 2] = '.';
    output[numdigits - 1] = digits[numdigits - 2] + 0x30;
    output[numdigits] = digits[numdigits - 1] + 0x30;
    output[numdigits + 1] = '\0';
    free(digits);
    return output;
  }
}

char * centiGallonsToASCII2(long unsigned int centiG) {
  output[0] = centiG / 1000000000 + 0x30;
  output[1] = (centiG % 1000000000) / 100000000 + 0x30;
  output[2] = (centiG % 100000000) / 10000000 + 0x30;
  output[3] = (centiG % 10000000) / 1000000 + 0x30;
  output[4] = (centiG % 1000000) / 100000 + 0x30;
  output[5] = (centiG % 100000) / 10000 + 0x30;
  output[6] = (centiG % 10000) / 1000 + 0x30;
  output[7] = (centiG % 1000) / 100 + 0x30;
  output[8] = '.';
  output[9] = (centiG % 100) / 10 + 0x30;
  output[10] = (centiG % 10) + 0x30;
  output[11] = '\0';
  return output;
}

void centiGallonsToASCII3(long unsigned int centiG, char * string) {
  unsigned long int intPart = centiG / 100;
  unsigned int decimalPart1 = (centiG - intPart * 100) / 10;
  unsigned int decimalPart2 = centiG - intPart * 100 - decimalPart1 * 10;
  sprintf(string, "  %lu.%u%u  ", intPart, decimalPart1, decimalPart2);
}

char * stripPreceedingZeros(char * string, int length) {
  char numChars;
  int i;
  for (i = 0; string[i] != '\0'; i++) {
    if (string[i] != '0') {
      numChars = length - i;
      break;
    }
  }
  char * returnString = malloc(sizeof(char) * numChars);
  for (i = 0; i < numChars; i++) {
    returnString[i] = string[length - numChars + i];
  }

  return returnString;
}

unsigned long int exp(unsigned int base, char exponent) {
  unsigned long int output = 1;
  int i;
  for (i = 0; i < exponent; i++) {
    output *= base;
  }

  return output;
}

char arrayCmp(char array1[], char array2[], int size2) {
  int i;
  for (i = 0; i < size2; i++) {
    if (array1[i] != array2[i]) return 0;
  }
  return 1;
}