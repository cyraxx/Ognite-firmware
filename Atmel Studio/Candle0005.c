/*
 * Candle0005.c
 *
 * Created: 4/9/2013 12:01:43 AM
 *  Author: josh
 */ 

/*
	09/21/13 - Reversed matrix lookup table to cancel out mirrored physical boards. Boards got mirrored accidentally and easier to change software than reprint.
	09/22/13 - Reversed the rowDirectionBits to fix the mess caused by the previous change. Argh.  
	09/26/13 - First GitHub commit
	10/16/13 - Added test mode on that will show a test pattern on power-up.
	10/31/13 - Starting to whittle. Program Memory Usage 	:	3254 bytes   
	10/31/13 - Changed everything except ISR to static inline. Program Memory Usage 	:	3240 bytes
	11/07/13 - video now plays a frame at a time asynchronously and things generally cleaned up
	11/21/13 - Switched over to running off the WatchDog and sleeping the rest of the time
*/

// TODO: Check if changing SUT to reduce the startup delay to only 4 cycles (without the default 64ms) would save power. 
//       Might need to turn on BOD and then turn it off when we sleep using BODCR

// TODO: Once everything is done, could set Watchdog fuse and then would not have to have code to turn it on at power up. Might also preserve the prescaler between resets, saving more time and code
// TODO: Once everything is done, could clear the the CLK/8 bit and have chip startup running fast and not have to do in code on every wake. 

// TODO: Maybe try to get everything running with fuze-set 4mhz so that we can run battery down to 2.7 volts 

#define F_CPU 8000000UL  // We will be at 8 MHz once we get all booted up and get rid of the prescaler

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>				// Watchdog Functions
#include <string.h>					// memset()

#include "candle.h"
#include "videoBitstream.h"

#ifndef DEBUG
	FUSES = {
		.low = (FUSE_SUT1 & FUSE_SUT0 & FUSE_CKSEL3 & FUSE_CKSEL1 & FUSE_CKSEL0),						// Startup with full speed, no startup delay, 8Mhz internal RC
		.high =  HFUSE_DEFAULT,
		.extended = EFUSE_DEFAULT
	};
#else
	FUSES = {
		.low = (FUSE_CKDIV8 & FUSE_SUT1 & FUSE_SUT0 & FUSE_CKSEL3 & FUSE_CKSEL1 & FUSE_CKSEL0),			// Startup with clock/8, no startup delay, 8Mhz internal RC
		.high =  HFUSE_DEFAULT,
		.extended = EFUSE_DEFAULT
	};
#endif


// Convert 5 bit brightness into 8 bit LED duty cycle
// Note that we could keep this in program memory but we have plenty of static RAM right now 
// and this way makes slightly smaller code

#define DUTY_CYCLE_SIZE (1<<BRIGHTNESSBITS)
#define FULL_ON_DUTYCYCLE 255	// how much is a full on LED?

const byte brightness2Dutycycle[DUTY_CYCLE_SIZE] = {
	0,     1 ,     2,     3,     4,     5,     7,     9,    12,
	15,    18,    22,    27,    32,    38,    44,    51,    58,
	67,    76,    86,    96,   108,   120,   134,   148,   163,
	180,   197,   216,   235,   255,
};

#define getDutyCycle(b) (brightness2Dutycycle[b])

#define FDA_X_MAX ( (byte) 5 )
#define FDA_Y_MAX ( (byte) 8 )
#define ROWS FDA_Y_MAX
#define COLS FDA_X_MAX

#define FDA_SIZE ( (byte) (FDA_X_MAX*FDA_Y_MAX) )

// This is the display array!
// Array of duty cycles levels. Brightness (0=off,  255=brightest)
// Anything you put in here will show up on the screen

// Now fda is linear - with x then y for easy scanning in x direction
// Graph paper style - x goes left to right, y goes bottom to top 

byte fda[FDA_SIZE];

#define REFRESH_RATE ( (byte) 62 )			// Display Refresh rate in Hz (picked to match the fastest we can get WDT wakeups)

#ifdef DEBUG
#define TIMECHECK 1				// Twittle bits so we can watch timing on an osciliscope
								// PA0 (pin 5) goes high while we are in the screen refreshing/PWM interrupt routine
								// PA1 (pin 4) goes high while we are decoding the next frame to be displayed
#endif

#define NOP __asm__("nop\n\t")

byte diagPos=0;		// current screen pixel when scanning in diagnostic modes 0=starting to turn on, FDA_SIZE=starting to turn off, FDA_SIZE*2=done with diagnostics

// Decode next frame into the FDA
static inline void nextFrame(void) {
	
	  #ifdef TIMECHECK
		  PORTA |= _BV(1);
	  #endif
	  
	  if (0) {
  				byte fdaptr = 0;
		  
				for(byte b=0;b<(_BV(BRIGHTNESSBITS));b++){
					byte d = getDutyCycle( b);				// normalize step variable to always cycle within brightness range
					fda[fdaptr++] = d;
				}
				
		return;				
	  }
	  
	  // TODO: this diagnostic screen generator costs 42 bytes. Can we make it smaller or just get rid of it?

#ifdef DEBUG		
	  if (diagPos<(FDA_SIZE*4)) {		// We are currently generating the startup diagnostics screens
#else
      if (diagPos<(FDA_SIZE*2)) {
#endif
		  
		  if (diagPos<FDA_SIZE) {						// Fill screen in with pixels
			  fda[diagPos] = FULL_ON_DUTYCYCLE;

#ifdef DEBUG
		  } else if (diagPos<FDA_SIZE*2) {				// Empty out
#else
		  } else {
#endif
			  fda[(diagPos)-FDA_SIZE] = 0;

#ifdef DEBUG
		  } else /* if (diagPos>=FDA_SIZE*2) && (diagPos<FDA_SIZE*4) */ {										// Brightness test pattern
				byte step=( diagPos-(FDA_SIZE*2) );
				byte fdaptr = 0;
			
				for(byte y=0; y<FDA_Y_MAX;y++) {
					byte b = getDutyCycle( step & (_BV(BRIGHTNESSBITS)-1) );				// normalize step variable to always cycle within brightness range
				
					for(byte x=0;x<FDA_X_MAX;x++) {
						fda[fdaptr++] = b;
					}

					step++;
				}
#endif
		  }
			  
		  diagPos++;
		  
	   } else {  // normal video playback....
		  // Time to display the next frame in the animation...
		  // copy the next frame from program memory (candel_bitstream[]) to the RAM frame buffer (fda[])
		  		  
		  static byte const *candleBitstremPtr;     // next byte to read from the bitstream in program memory
		  static byte workingByte;			  // current working byte
		  static byte workingBitsLeft;      // how many bits left in the current working byte? 0 triggers loading next byte
		  static framecounttype frameCount = FRAMECOUNT;		// what frame are we on?

		  if ( frameCount==FRAMECOUNT ) {							// Is this the last frame?
			  memset( fda , 0x00 , FDA_SIZE );			// zero out the display buffer, becuase that is how the encoder currently works
			  candleBitstremPtr=videobitstream;		// next byte to read from the bitstream in program memory
			  workingBitsLeft=0;							// how many bits left in the current working byte? 0 triggers loading next byte
			  frameCount= 0;
		  }
		  
		  frameCount++;
		  
		  byte fdaIndex = FDA_SIZE;		// Which byte of the FDA are we filling in? Start at end because compare to zero slightly more efficient and and that is how data is encoded
		  byte brightnessBitsLeft=0;	// Currently building a brightness value? How many bits left to read in?
		  byte workingBrightness;		// currently building brightness value
		  
		  do {			// step though each pixel in the fda
			  if (workingBitsLeft==0) {										// normalize to next byte if we are out of bits
				  workingByte=pgm_read_byte_near(candleBitstremPtr++);
				  workingBitsLeft=8;
			  }
			  
			  byte workingBit = (workingByte & 0x01);
			  
			  if (brightnessBitsLeft>0) { //are we currently reading brightness? Consume as many bits as we can (if too few) or as we need (if too manY) from working byte into brightness
				  workingBrightness <<=1;
				  workingBrightness |= workingBit;

				  brightnessBitsLeft--;

				  if (brightnessBitsLeft==0) {		// We've gotten enough bits to assign the next pixel!
					  fda[--fdaIndex] = getDutyCycle(workingBrightness);
				  }
					  				  
			  } else { // We are not currently reading in a pending brightness value 
				  if ( workingBit ==  0x00 ) {		// 0 bit indicates that this pixel has not changed
					  --fdaIndex;								// So skip it
				  } else {							// Start reading in the following bits as a brightness value
					  brightnessBitsLeft = BRIGHTNESSBITS;					// Now we will read in the brightness value on next loops though
					  workingBrightness = 0;
				  }
			  }
			  
			  workingByte >>=1;				
			  workingBitsLeft--;
			  
		  } while ( fdaIndex > 0 );
		  		  
		  #ifdef TIMECHECK
			 PORTA  &= ~_BV(1);
		  #endif
	  }
}


#define ALL_PORTD_ROWS_ZERO 1		// Just a shortcut hard-coded that all PORTD row bits are zero 

static byte const rowDirectionBits = 0b01010101;      // 0=row goes low, 1=Row goes high

static byte const portBRowBits[ROWS]  = {_BV(0),_BV(0),_BV(2),_BV(2),_BV(4),_BV(4),_BV(6),_BV(6) };
static byte const portDRowBits[ROWS]  = {     0,     0,     0,     0,     0,     0,     0,     0 };
	
// Note that col is always opposite of row so we don't need colDirectionBits

static byte const portBColBits[COLS] = {_BV(7),_BV(5),_BV(3), _BV(1),     0};
static byte const portDColBits[COLS] = {     0,     0,      0,     0,_BV(6)};

#define REFRESH_PER_FRAME ( REFRESH_RATE / FRAME_RATE )		// How many refreshes before we trigger the next frame to be drawn?

byte refreshCount = REFRESH_PER_FRAME+1;

#define LED_DUTY_CYCLE_PORT (DDRB)			// This is the port we use for actually timing the LEDs on time
											// We use PORTB rather than PORTD because in the current LED layout, setting DDRB=0 will always turn off all LEDs

// Set (ledonbits) to (LED_DUTY_CYCLE_PORT) for (cycles) CPU cycles, then send a zero to the port
static inline void ledDutyCycle(unsigned char cycles , byte ledonbits )
{
	switch (cycles) {
		// First, special case out the 0,1, and 2 cycle cases becuase these are so short that we can't do it any other way then hard code
		case 0:
			break;
		case 1:
			{
				__asm__ __volatile__ (
					"OUT %0,%1 \n\t"				// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
					"OUT %0,__zero_reg__ \n\t"
					: :  "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , "r" (ledonbits)
				);
			}
			break;
		case 2:
			{
				__asm__ __volatile__ (
					"OUT %0,%1 \n\t"				// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
					"NOP \n\t"		// waste one cycle that will (hopefully) not get optimized out
					"OUT %0,__zero_reg__ \n\t"
					: :  "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , "r" (ledonbits)
				);
			}
			break;
		case 3:
			{
				__asm__ __volatile__ (
					"OUT %0,%1 \n\t"				// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
					"RJMP L_%=\n\t"		// waste 2 cycles - use RJMP rather than 2 NOPS because it uses 1/2 the space
					"L_%=:OUT %0,__zero_reg__ \n\t"
					: :  "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , "r" (ledonbits)
				);
			}
			break;
		default:
			{
				// note that we could make a loop that only uses three cycles, but use 4 makes the math so much easier

				// for any delay greater than 3, we have enough cycles to go though a general loop construct
			
				// Loop delay for loop counter c:
			
				// --When c==1---
				//	loop:   DEC c		// 1 cycle  (c=0)
				//      NOP             // 1 cycle
				//	BRNE loop			// 1 cycle
				//                      // =============
				//                      // 3 cycles total

				// --When c==2---
				//	loop:   DEC c		// 1 cycle	(c=1)
				//	BRNE loop			// 2 cycles (branch taken)
				//  NOP					// 1 cycle
				//	loop:   DEC c		// 1 cycle	(c=0)
				//	BRNE loop			// 1 cycles (branch not taken)
				//  NOP					// 1 cycle
				//                      // =============
				//                      // 7 cycles total
			
				// if c==1 then delay=3 cycles (branch not taken at all)
				// if c==2 then delay=7 (2+4) cycles
				// if c==3 then delay=10 (2+4+4)
			
				// ...so loop overhead is always 3+(c-1)*4 cycles
			
				// Include the single cycle overhead for the trailing OUT after the loop and we get...
			
				// delay = 4+(c-1)*4
				// delay = 4+(4*c)-4
				// delay = (4*c)
				// delay/4 = c
			
				byte loopcounter = cycles/ (byte) 4;		// Compiler should turn this into a shift operation
				byte remainder = cycles & 0x03 ;			// AND with 0x03 will efficiently return the remainder of divide by 4
			
				switch (remainder) {
					case 0:	
						{ // No remainder, so just loop and we will get right delay
							__asm__ __volatile__ (
								"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
								"L_%=:dec %[loop]\n\t"			// 1 cycle
								"NOP \n\r"						// 1 cycle in loop
								"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
								"OUT %[port],__zero_reg__ \n\t"
								: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
							);
						}
						break;
					case 1: 
						{ // We need 1 extra cycle to come out with the right delay
							__asm__ __volatile__ (
								"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
								"L_%=:dec %[loop] \n\t"			// 1 cycle
								"NOP \n\r"						// 1 cycle in loop
								"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
								"NOP \n\r"						// 1 cycle after loop
								"OUT %[port],__zero_reg__ \n\t"
								: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
							);
						}
						break;
					case 2:
						{ // We need 2 extra cycles to come out with the right delay
							__asm__ __volatile__ (
								"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
								"L_%=:dec %[loop] \n\t"			// 1 cycle
								"NOP \n\r"						// 1 cycle in loop
								"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
								"RJMP .+0 \r\n"					// 2 cycles after loop (RJMP better than 2 NOPS becuase same time 1/2 the space)
								"OUT %[port],__zero_reg__ \n\t"
								: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
							);
						}
						break;
					case 3:
						{ // We need 2 extra cycles to come out with the right delay
							__asm__ __volatile__ (
								"OUT %[port],%[bits] \n\t"			// DO OUTPORT first because in current config it will never actually have both pins so LED can't turn on (not turn of DDRB)
								"L_%=:dec %[loop] \n\t"			// 1 cycle
								"NOP \n\r"						// 1 cycle in loop
								"BRNE L_%= \n\t"			// 1 on false, 2 on true cycles
								"RJMP .+0 \r\n"					// 2 cycles after loop (RJMP better than 2 NOPS becuase same time 1/2 the space)
								"NOP \n\r"						// 1 cycle after loop
								"OUT %[port],__zero_reg__ \n\t"
								: [loop] "=r" (loopcounter) : [port] "I" (_SFR_IO_ADDR(LED_DUTY_CYCLE_PORT)) , [bits] "r" (ledonbits) , "0" (loopcounter)
							);
						}
						break;
				} // switch (remainder)
			}	// default case where b>2
		
			break;
	}	// switch (b)
}

// Do a single full screen refresh     
// call nextframe() to decode next frame into buffer afterwards if it is time
// This version will work with any combination of row/col bits
static inline void refreshScreenClean(void)
{
	#ifdef TIMECHECK
		DDRA = _BV(0)|_BV(1);		// Set PORTA0 for output. Use OR because it compiles to single SBI instruction
		PORTA |=_BV(0);				// twiddle A0 bit for oscilloscope timing
	#endif
	
	byte *fdaptr = fda;		 // Where are we in scanning through the FDA?
	
	byte rowDirectionBitsRotating = rowDirectionBits;	// Working space for rowDirections bits that we shift down once for each row

	// Bit 0 will be bit for the current row.
	// TODO: in ASM, would could shift though the carry flag and jmp based on that and save a bit test
	
	for( byte y = 0 ; y < FDA_Y_MAX ; y++ ) {
		byte portBRowBitsCache = portBRowBits[y]; 
		byte portDRowBitsCache = portDRowBits[y]; 
		
		for( byte x = 0 ; x < FDA_X_MAX ; x++) {
			// get the brightness of the current LED
			register byte b = *( fdaptr++ );		// Want this in a register because later we will loop on it and want the loop entrance to be quick
			
			// If the LED is off, then don't need to do anything since all LEDs are already off all the time except for a split second inside this routine....
			if (b>0) {
				byte portBColBitsCache = portBColBits[x]; 
				byte portDColBitsCache = portDColBits[x]; 
				
				// Assume DDRB = DDRD = 0 coming into the INt since that is the Way we should have left them when we exited last...
				byte ddrbt;
				byte ddrdt;

				if ( rowDirectionBitsRotating & _BV(0) ) {    // lowest bit of the rotating bits is for this row. If bit=1 then row pin is high and col pins are low....
					PORTB = portBRowBitsCache;
					PORTD = portDRowBitsCache;

					// Only need to set the correct bits in PORTB and PORTD to drive the row high (col bit will get set to 0)
					ddrbt  = portBRowBitsCache | portBColBitsCache ;       // enable output for the Row pins to drive high, also enable output for col pins which are zero so will go low
					ddrdt  = portDRowBitsCache | portDColBitsCache;
				} else {      // row goes low, cols go high....
					PORTB = portBColBitsCache;
					ddrbt  = portBColBitsCache | portBRowBitsCache;               // enable output for the col pins to drive high, also enable output for row pins which are zero so will go low
					
					PORTD = portDColBitsCache;					
					ddrdt  = portDColBitsCache | portDRowBitsCache;
				}
				
				DDRD = ddrdt;
				ledDutyCycle( b , ddrbt );
				DDRD = 0x00;
			}
		}
		
		rowDirectionBitsRotating >>= 1;		// Shift bits down so bit 0 has the value for the next row
	}
	
	#ifdef TIMECHECK
		PORTA &= ~_BV(0);
	#endif
	
	refreshCount--;
	if (refreshCount == 0 ) {			// step to next frame in the animation sequence?
		refreshCount=REFRESH_PER_FRAME+1;
		// Update the display buffer with the next frame of animation
		nextFrame();
	}
}

void init0 (void) __attribute__ ((naked)) __attribute__ ((section (".init0")));

// This code will be run immedeately on reset, before any initilization or main()
void init0(void) {
	asm( "in	__tmp_reg__	, %[mcusr] "	: : [mcusr] "I" (_SFR_IO_ADDR(MCUSR)) ); 	// Get the value of the MCUSR register into the temp register
	asm( "sbrc	__tmp_reg__	,%[wdf] "		: : [wdf] "I" (WDRF) );						// Test the WatchDog Reset Flag and skip the next instruction if the bit is clear
	asm( "rjmp warmstart" 					: :   );									// If we get to here, the the WDF bit was set so we are waking up warm, so jump to warm code

	// On power up, This code will fall though to the normal .init seconds and set up all the variables and get ready for main() to start
}

// TODO: Make our own linker script and move warmstart here so we save one cycle on the RJMP, and instead take that hit only once with a jump to main() on powerup

// Main() only gets run once, when we first power up
int main(void)
{
	wdt_enable(WDTO_15MS);							// Could do this slightly more efficiently in ASM, but we only do it one time- when we first power up
	
	// The delay set here is actually just how long until the first watchdog reset so we will set it to the lowest value to get into cycyle as soon as possible
	// Once in the cycle,  warmstart will set it to the desired running value each time.
	
	// Note that we could save a tiny ammount of time if we RJMPed right into the warmstart() here, but that
	// could introduce a little jitter since the timing would be different on the first pass. Better to
	// always enter warmstart from exactly the same state for consistent timing.
	
	MCUCR = _BV( SE ) |	_BV(SM1 ) | _BV(SM0);		// Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
	asm("sleep");
	
	// we should never get here
}


// This is "static inline" so The code will just be inserted directly into the warmstart code avoiding overhead of a call/ret
// Important that this function always finishes before WDT expires or it will get cut short
static inline void userWakeRoutine(void) {
	refreshScreenClean();
}

void  __attribute__ ((naked)) warmstart(void) {
	// Set the timeout to the desired value. Do this first because by default right now it will be at the inital value of 16ms
	// which might not be long enough for us to do what we need to do before the WatchDog times out and does a reset.
	
	// Note that we do not have to set WDTCR because the default timeout is initialized to 16ms after a reset (which is now). This is ~60Hrz display refresh rate.
	#ifdef DEBUG							// In production build, the FUSE will already have us start at full speed
		CLKPR = _BV(CLKPCE);				// Enable changes to the clock prescaler
		CLKPR = 0;							// Set prescaler to 1, we will run full speed						
											// TODO: Check if running full speed uses more power than doing same work longer at half speed
	#endif
	
	// Now do whatever the user wants...
	userWakeRoutine();
	
	// Now it is time to get ready for bed. We should have gotten here because there was just a WDT reset, so...
	// By default after any reset, the watchdog timeout will be 16ms since the WDP bits in WDTCSR are set to zero on reset. We'd need to set the WDTCSR if we want a different timeout
	// After a WDT reset, the WatchDog should also still be on by default because the WDRF will be set after a WDT reset, and "WDE is overridden by WDRF in MCUSR. See �MCUSR � MCU Status Register� on page 45for description of WDRF. This means thatWDE is always set when WDRF is set."

	MCUCR = _BV( SE ) |	_BV(SM1 ) | _BV(SM0);		// Sleep enable (makes sleep instruction work), and sets sleep mode to "Power Down" (the deepest sleep)
	asm("sleep");
	
	// we should never get here
}