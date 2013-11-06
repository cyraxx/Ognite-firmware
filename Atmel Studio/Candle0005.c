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
	
	      
*/

#define F_CPU 8000000UL  // We will be at 8 MHz once we get all booted up and get rid of the prescaller

#include <avr/io.h>

#include <avr/pgmspace.h>

#include <avr/interrupt.h>

#include <string.h>

typedef unsigned char byte;			// Define a byte
typedef unsigned int word;			// Define a word

#define FRAME_RATE 15				// Frames per second

#define	WIDTH 	5
#define	HEIGHT	8

typedef byte framecounttype;		// Define this type in case we ever go over 255 frames we can switch to an unsigned int

#define	FRAMECOUNT	((framecounttype) 195)

PROGMEM byte const candle_bitstream[]  = {
	0xaa,0x2f,0x61,0xf8,0x1b,0xe6,0x7f,0x64,0xf9,0x17,
	0xe6,0x6f,0xa3,0x9d,0x27,0xb2,0xa2,0x0e,0x85,0x22,
	0xb6,0x2d,0x10,0x5e,0x5f,0x2c,0xba,0x15,0xa2,0x4b,
	0x2b,0x3c,0x0c,0x0c,0x4b,0x4f,0x51,0x96,0x01,0x48,
	0xbf,0x96,0xe2,0x3f,0x02,0xc5,0xff,0xc6,0xb2,0x4f,
	0x21,0xb9,0xde,0x56,0x34,0xcd,0x2d,0x65,0x51,0x10,
	0x09,0x15,0x2a,0x6a,0x1b,0xb5,0x27,0x52,0xfc,0x72,
	0xe9,0xad,0x01,0x00,0x21,0xb9,0x9a,0xca,0x0e,0x62,
	0x3e,0x05,0xb4,0x00,0x00,0xa9,0x96,0xd2,0x41,0xe5,
	0x4d,0xa0,0x04,0x00,0x72,0x20,0x7e,0x46,0x72,0x77,
	0x28,0xff,0xbe,0x54,0xf6,0xaa,0x24,0x97,0x5e,0x68,
	0x02,0x80,0xf0,0xda,0x48,0xde,0x37,0x96,0xbe,0x7f,
	0x21,0x79,0x74,0xa2,0xf3,0x19,0x19,0x75,0xc8,0x21,
	0x20,0x58,0xef,0x50,0x70,0xfe,0x85,0xf0,0xd2,0x09,
	0x2f,0x93,0x60,0xb7,0x08,0x86,0xb1,0xa7,0x50,0x10,
	0x42,0xfb,0x67,0xcc,0x3a,0xc1,0x6a,0x16,0x6c,0x36,
	0x16,0x3b,0xdd,0x34,0x86,0x72,0x65,0x08,0x14,0x2a,
	0x3a,0x23,0x93,0x95,0xd9,0xc1,0xf4,0xa2,0x85,0xac,
	0x00,0x11,0x99,0x9a,0x16,0x46,0x98,0xec,0x00,0x26,
	0xa8,0x00,0x80,0x09,0x30,0x03,0x00,0xb9,0x12,0x40,
	0x0b,0x33,0x6c,0x70,0x30,0x3f,0x47,0xa8,0x03,0x39,
	0x00,0x03,0x8d,0x8d,0xc1,0x01,0x57,0x88,0x33,0x62,
	0x14,0x91,0x55,0x80,0xd4,0x0c,0x07,0x5c,0x21,0xee,
	0xc8,0x74,0x8b,0xb5,0x83,0x18,0x00,0xac,0xd4,0xce,
	0x50,0xef,0x8e,0x4c,0x9e,0x18,0x57,0x02,0x48,0x45,
	0x00,0x1b,0x5c,0x20,0x06,0x5c,0x00,0x94,0x80,0x0c,
	0x38,0x41,0x64,0x84,0x11,0x00,0x00,0xac,0x70,0x04,
	0x3a,0x17,0xc0,0x7d,0xc6,0x18,0x91,0x01,0xb0,0x50,
	0x01,0x42,0x83,0x3b,0x32,0x00,0x00,0x20,0x35,0xc3,
	0x4e,0xeb,0x0c,0x00,0xd0,0x8c,0x00,0x80,0x89,0xd2,
	0x06,0xe8,0x5d,0x21,0xcf,0x11,0x41,0x44,0x5e,0x84,
	0x08,0x49,0x00,0x34,0x0e,0x3a,0xe8,0xc1,0x84,0x0c,
	0x00,0xc8,0xa1,0x86,0x16,0x3a,0xa8,0x01,0x80,0xd8,
	0x4c,0x06,0x95,0x93,0x06,0x5a,0x27,0x00,0x80,0xf0,
	0x5d,0x49,0x1d,0x14,0x84,0x6a,0x77,0xa4,0x21,0x86,
	0x18,0x00,0xc1,0xb3,0x91,0x38,0xc9,0x5d,0x54,0x9e,
	0x18,0xa8,0x40,0x21,0x02,0xdc,0x07,0xf1,0x77,0x91,
	0xb9,0x23,0x25,0xd4,0xae,0x84,0x19,0x00,0xb8,0x10,
	0xbd,0x42,0xa9,0x87,0x02,0xaa,0x17,0x25,0x00,0x00,
	0x10,0xe0,0x06,0x00,0x00,0x80,0x7b,0x27,0xfe,0x4e,
	0x32,0x42,0x25,0xd4,0xce,0x58,0x05,0x08,0x01,0xcf,
	0x8a,0xff,0x20,0x77,0x06,0x2a,0x77,0x04,0x20,0x02,
	0x40,0xf8,0x4d,0xa4,0x56,0x0a,0x07,0xb5,0x2b,0xd4,
	0x3a,0x22,0x00,0x80,0xf8,0x1f,0xc8,0xcc,0x54,0x36,
	0x1a,0x67,0xa0,0xfb,0xf6,0x50,0x6d,0x08,0x01,0x90,
	0xe8,0x28,0x4c,0xd4,0x56,0x3a,0x07,0x00,0x00,0x90,
	0x6a,0x29,0xa1,0x01,0xd0,0xbf,0x02,0x0d,0x00,0x40,
	0x07,0x00,0x3d,0x20,0x04,0x00,0x48,0xf4,0x30,0xc3,
	0x46,0xe7,0x04,0x00,0x00,0x18,0x29,0xac,0xd4,0x76,
	0xb8,0x42,0x1c,0x98,0xc6,0x08,0x00,0xb1,0x99,0xdc,
	0x46,0xe5,0xa0,0x85,0x8e,0x48,0x0d,0x00,0x44,0xdf,
	0x42,0xe6,0x80,0x33,0xd4,0xb8,0x23,0xad,0x13,0x20,
	0x07,0x08,0x9f,0x8d,0xd4,0x49,0xe1,0xa2,0xf6,0x80,
	0x18,0x62,0x0a,0x80,0xe0,0xde,0x49,0xbe,0x8b,0xdc,
	0x1d,0xa9,0x88,0x35,0x0f,0x2a,0x00,0xc0,0x75,0x12,
	0xbb,0x43,0x99,0x27,0x56,0x92,0xa8,0x5d,0x09,0x26,
	0x10,0x01,0xce,0x8b,0xe8,0x7d,0x22,0x29,0xf0,0x02,
	0x4a,0x48,0xf3,0x12,0x60,0xbf,0x09,0x1f,0xf8,0xde,
	0x44,0x41,0xaa,0x22,0x85,0x04,0x00,0x3c,0x21,0xf7,
	0x1b,0x4b,0x48,0x01,0x6e,0x00,0x00,0x56,0x04,0xd7,
	0x97,0xf0,0x22,0xf7,0x65,0x4a,0x32,0x00,0x00,0xde,
	0x08,0x88,0x7d,0x00,0x80,0x14,0x00,0x16,0x38,0xa5,
	0x90,0xc9,0xc8,0x15,0xe4,0x0a,0x00,0x44,0x98,0xbe,
	0x98,0xe3,0x27,0x7a,0xe4,0x52,0x3f,0x00,0x28,0xc4,
	0x80,0x11,0x76,0x99,0xf0,0xfe,0x49,0x3e,0x85,0x9c,
	0x42,0x4e,0x06,0x00,0xc3,0x9f,0xb0,0x01,0x05,0x94,
	0xb2,0x1b,0x20,0x01,0x00,0x80,0x0b,0xf1,0x0b,0x00,
	0x52,0x00,0x00,0x00,0x00,0xcc,0x00,0xc0,0xf8,0xc5,
	0xec,0x70,0xcb,0x25,0x9f,0x02,0x00,0x80,0x10,0x13,
	0x90,0x02,0x20,0x7f,0x80,0x14,0x00,0xe6,0x37,0xe2,
	0xf8,0x12,0x7c,0x19,0xe4,0x90,0x03,0x00,0x58,0x84,
	0x80,0x07,0x7c,0x00,0x00,0x00,0x0f,0x4e,0xb1,0x88,
	0x14,0x32,0x00,0x20,0x02,0xac,0x77,0xc0,0xf5,0x46,
	0x78,0x13,0xe9,0x0f,0x64,0x0a,0x00,0xc0,0x7e,0xe1,
	0x7e,0x88,0x5f,0x20,0x55,0x00,0x12,0x00,0x38,0x0f,
	0xc2,0xe7,0x0e,0x25,0xdf,0x13,0xcb,0xbd,0x89,0xf2,
	0xbd,0x52,0x00,0x00,0xf7,0x46,0xf4,0x9d,0x81,0xf4,
	0xbf,0x43,0x85,0x27,0x56,0x91,0x28,0x89,0x01,0x10,
	0xbe,0x33,0xf1,0xbf,0x93,0x39,0x03,0xa5,0x3b,0x52,
	0x3b,0x63,0x8c,0x28,0x8b,0x10,0x88,0xff,0x9e,0xd4,
	0x42,0x61,0xa7,0x76,0x06,0x9a,0xef,0x88,0x54,0x8b,
	0x48,0x06,0x42,0xa4,0x1a,0x0a,0x23,0x95,0x95,0xc6,
	0x41,0x6b,0x0f,0xd5,0x84,0x00,0x28,0x94,0x54,0x7a,
	0x1a,0x33,0x9d,0x9d,0xce,0x16,0x60,0x00,0x80,0x4a,
	0x41,0xab,0xa5,0x33,0x31,0xd8,0x18,0xa0,0x81,0x1c,
	0x40,0x23,0xa3,0xd3,0x30,0x18,0x19,0xad,0x80,0x16,
	0xc8,0x03,0xa0,0x95,0xd3,0x6b,0x61,0x82,0x0d,0x00,
	0x28,0x42,0xa0,0x56,0xd0,0xe9,0xe8,0xcd,0x0c,0x76,
	0xd8,0x69,0x00,0x80,0x52,0x4d,0x6d,0xa4,0xb5,0xd2,
	0x39,0xe8,0x1c,0x21,0xc6,0x08,0x00,0x99,0x9e,0xd2,
	0x4a,0xed,0xa0,0x71,0x85,0xda,0xf7,0x8c,0xd4,0xc4,
	0x32,0x00,0xb1,0x99,0xcc,0x4e,0xe9,0x0a,0xd5,0xee,
	0x48,0xe3,0x8a,0x55,0x80,0x08,0x60,0x80,0x15,0xf6,
	0x00,0x57,0xc8,0x77,0x46,0x10,0x41,0x08,0x64,0x6a,
	0x0a,0x03,0x95,0x85,0xc6,0x1e,0x68,0xed,0x81,0xda,
	0x10,0x02,0xa0,0x94,0x52,0xab,0x68,0xf4,0x74,0x66,
	0xba,0x7f,0xc5,0xda,0x07,0x54,0x79,0x00,0xd4,0x5f,
	0x44,0x23,0xa7,0xd3,0xd0,0x1b,0xe8,0xcd,0x34,0x1b,
	0xf2,0x1a,0x80,0x10,0x32,0x5a,0x35,0x00,0xd8,0x01,
	0x40,0x05,0x35,0x34,0xd0,0x01,0x00,0x59,0x03,0x50,
	0x8a,0xa8,0x00,0x00,0x00,0x00,0x28,0xc4,0x90,0x43,
	0x03,0x00,0x20,0x07,0x90,0x43,0xa9,0xa0,0x86,0xd6,
	0x48,0x07,0x20,0x03,0x00,0xc8,0x01,0x0c,0x00,0x90,
	0x03,0x28,0xde,0x88,0x4a,0x46,0xa3,0x86,0x1e,0x26,
	0x00,0x00,0x80,0x14,0x2a,0x3a,0xe8,0x81,0x0e,0x00,
	0x08,0x01,0x00,0x00,0x00,0x00,0x11,0x64,0x50,0x03,
	0x00,0xc8,0x00,0xe4,0x5f,0x4c,0x29,0xa7,0xd6,0xd0,
	0x1a,0xe8,0xcc,0xd0,0xa3,0x06,0xc8,0xfe,0x84,0x42,
	0x09,0x2d,0x8c,0xb0,0x50,0x6f,0x00,0x20,0x95,0x42,
	0x45,0xa5,0x83,0x19,0x00,0x00,0x20,0x23,0x57,0x43,
	0x4f,0x03,0xac,0x00,0x00,0x12,0x39,0x34,0x30,0xc0,
	0x42,0x0b,0x0c,0xa8,0x0a,0x00,0x25,0x99,0x96,0xd2,
	0x08,0x2b,0x6c,0x00,0x00,0x62,0x15,0xa9,0x9e,0xc2,
	0x4c,0x6d,0xa3,0x81,0x6a,0x15,0x02,0x20,0xfa,0x1a,
	0x12,0x03,0xb9,0x85,0xca,0x0e,0x3b,0x8c,0x00,0x10,
	0xbe,0x1d,0xb1,0x89,0xcc,0x4a,0xe9,0xa0,0x76,0x84,
	0x00,0x42,0x20,0x78,0x06,0x22,0x0b,0xa9,0x9d,0xc2,
	0x49,0xe5,0xa4,0x24,0x92,0x02,0x70,0x4f,0xf8,0x36,
	0x38,0xc8,0x5d,0x21,0x9f,0x08,0x00,0xc0,0xb9,0x12,
	0xbe,0x07,0x89,0x2b,0xc4,0x1d,0x29,0x5d,0xb1,0x62,
	0x99,0x62,0x4a,0x00,0xc7,0x4e,0xf0,0x5c,0xc4,0xdf,
	0x1d,0xc9,0x3c,0xb1,0xe2,0x95,0x00,0x00,0xd8,0x0f,
	0xdc,0x37,0xde,0x07,0xde,0x04,0x00,0x88,0x00,0x4e,
	0x78,0x42,0x88,0x01,0xee,0x14,0x12,0x00,0x38,0x10,
	0x3e,0x48,0x3e,0x20,0x55,0xba,0x30,0x03,0x00,0x27,
	0x00,0xc8,0x01,0x94,0x46,0x14,0x00,0x20,0x7a,0x45,
	0xd2,0x5f,0xa2,0x80,0x0a,0x40,0x06,0xc0,0x85,0xd8,
	0x4d,0x06,0x25,0xd4,0x0f,0x2a,0x00,0x02,0xb8,0x0f,
	0x92,0x0f,0x39,0x54,0xd0,0x80,0x09,0x00,0x00,0x3f,
	0x0a,0xa8,0x01,0xd4,0x00,0x40,0xf8,0x20,0x85,0x12,
	0x40,0x0b,0x20,0xcf,0x01,0xa2,0x77,0x27,0x73,0x85,
	0x2a,0x62,0x0d,0x00,0x00,0x00,0x1b,0x39,0x70,0xd3,
	0x92,0xe8,0x48,0x34,0x00,0x40,0xfc,0x2d,0x70,0x06,
	0x6a,0x22,0x3c,0x31,0x4e,0x10,0x53,0x00,0x24,0xff,
	0x4c,0x69,0xa7,0x71,0x86,0x3a,0x77,0xa4,0x27,0x06,
	0xc8,0x43,0x20,0x35,0x50,0x59,0x69,0x1d,0x81,0xde,
	0x15,0xf2,0x1e,0x11,0x86,0x08,0x00,0x85,0x86,0xc6,
	0x44,0x67,0x65,0x70,0x04,0x06,0x7b,0xa8,0x9d,0x85,
	0x40,0x14,0xa2,0x52,0xd0,0x69,0x19,0x4c,0x4c,0x56,
	0xc6,0x6f,0x0d,0x74,0x04,0x8a,0x52,0x00,0xf4,0x62,
	0x46,0x05,0x93,0x96,0xd9,0xc8,0x6c,0xc6,0xd2,0x03,
	0xc0,0x7c,0x07,0x2c,0x12,0x56,0x25,0x9b,0x8e,0xf5,
	0x1f,0xe9,0xd7,0x8e,0xb2,0xca,0x00,0xfb,0x8e,0xe3,
	0x0d,0x39,0x64,0x1c,0x6a,0x36,0x3d,0x83,0x16,0x00,
	0xae,0x19,0xd7,0x15,0x70,0x7d,0x09,0xa7,0x92,0x5d,
	0xcb,0xa8,0xa1,0x02,0x70,0x0f,0x84,0xcf,0x41,0x78,
	0xbf,0xb1,0xf0,0x92,0x73,0x68,0x98,0x00,0x00,0x00,
	0x88,0x00,0x00,0x00,0x70,0x21,0xb8,0x11,0x5c,0x1f,
	0xc1,0x09,0x60,0x04,0x00,0xce,0x19,0xe7,0x85,0x43,
	0xcc,0xa1,0x60,0xd3,0x62,0x43,0x59,0x03,0x6c,0x2b,
	0xf6,0x07,0xdb,0x9f,0xb0,0x29,0x59,0x61,0x00,0x00,
	0x96,0x1d,0xcb,0x8b,0x45,0xca,0xa2,0x62,0xd1,0xd1,
	0xef,0x28,0x1a,0x80,0xf1,0xc0,0x24,0x64,0x82,0x19,
	0x26,0xe8,0xb4,0x80,0x10,0xfa,0x13,0xfd,0x87,0x41,
	0xc6,0xa8,0x66,0x34,0xd0,0x42,0x2e,0x07,0x34,0x37,
	0xda,0x3f,0xa6,0x53,0xd0,0x6b,0x19,0x8c,0x34,0x3a,
	0x00,0xa8,0x1e,0x34,0x12,0x5a,0x25,0x74,0xf4,0x00,
	0x00,0xf0,0x85,0x90,0x42,0x05,0x3d,0x83,0x09,0x1b,
	0xd4,0x00,0xb5,0x08,0xd0,0xc1,0x00,0x8c,0xb4,0x00,
	0x40,0xfb,0x04,0xf4,0x12,0x46,0x05,0x93,0x96,0xc9,
	0x80,0x1d,0x00,0xcc,0x07,0x96,0x37,0x64,0x95,0xb2,
	0xaa,0x58,0x74,0xf4,0x5a,0x8a,0x06,0xe0,0x98,0x71,
	0x5e,0x01,0xc7,0x17,0x71,0xc8,0xd9,0x34,0x0c,0x1a,
	0x40,0x04,0x57,0x87,0x7b,0xc5,0x75,0x07,0x9c,0x52,
	0x76,0x15,0xe3,0x51,0xa3,0xcd,0x10,0x41,0x8d,0x67,
	0xc6,0x7d,0x81,0x04,0x4a,0x9c,0x15,0x65,0x07,0x70,
	0x56,0xb8,0xe1,0x84,0x43,0x0c,0x70,0x41,0x9f,0x22,
	0xc6,0x56,0xe3,0x58,0xb0,0xdf,0xd8,0x24,0xac,0x30,
	0xdc,0x00,0x60,0xee,0xb0,0x6e,0x58,0x5e,0x2c,0x52,
	0x66,0x15,0xd4,0x18,0x32,0xc0,0x30,0x60,0x3a,0x31,
	0x7e,0x21,0x93,0x8c,0x49,0x4d,0x0f,0x00,0x74,0x33,
	0xfa,0x1b,0xc3,0x1f,0x31,0xc8,0x19,0x35,0x74,0x1a,
	0x8a,0x1e,0xa0,0x59,0xd1,0xbd,0xe8,0x25,0x50,0x42,
	0x0b,0x2d,0xe4,0x00,0x3b,0x7c,0x21,0xa4,0x50,0x41,
	0x8f,0xab,0x43,0x07,0x50,0x1f,0x68,0xff,0x08,0x72,
	0x68,0x18,0x0c,0xb4,0x27,0x80,0x08,0xd5,0x0d,0x12,
	0x3a,0x25,0xbd,0x0e,0x26,0x1c,0x3d,0x79,0x5b,0x00,
	0x8a,0x37,0xa2,0x96,0xd3,0x6a,0xe8,0x0c,0xf4,0x16,
	0x9a,0x7d,0x40,0x83,0x30,0x44,0xfa,0xa5,0x14,0x6a,
	0x2a,0x03,0x8d,0x85,0xd6,0x46,0xbd,0x8d,0x64,0x75,
	0x09,0x88,0x34,0x24,0x46,0x72,0x2b,0x95,0x83,0xda,
	0x11,0xaa,0xd6,0x29,0x94,0x56,0x42,0x20,0xb8,0x67,
	0xa2,0x6f,0x27,0x75,0x52,0xb8,0x23,0xd5,0x7b,0xc5,
	0xca,0x59,0x4c,0x21,0x22,0x80,0x73,0xc7,0x7b,0x85,
	0x78,0x62,0xbc,0x29,0xee,0x14,0x52,0x00,0xb8,0x10,
	0x7f,0x37,0x99,0x37,0x51,0xfa,0x32,0xf5,0x23,0x53,
	0x4d,0xc8,0xf2,0x22,0x46,0x14,0x72,0xaf,0xa4,0xff,
	0x45,0xe9,0xa1,0xf1,0xd2,0x92,0xab,0x47,0x99,0x3c,
	0x03,0x60,0x22,0x73,0x06,0x6a,0xe8,0x7d,0x0c,0xd0,
	0x0e,0x28,0x52,0x89,0x08,0xcc,0x33,0xf1,0xfb,0x44,
	0x5a,0x6f,0x66,0xa2,0xb4,0xde,0x4f,0x65,0xea,0xe7,
	0x42,0x9d,0x94,0xa9,0x04,0x34,0x3b,0xe1,0xf5,0x67,
	0x6a,0x7f,0x8d,0xbf,0xb5,0x79,0x5b,0xa8,0x88,0x65,
	0x10,0x51,0x3c,0xa9,0x60,0xd3,0x48,0x3f,0x83,0x9a,
	0xc1,0xf0,0x3c,0x83,0x7e,0x98,0x1a,0x65,0x52,0xe4,
	0x62,0x10,0x5f,0x15,0x83,0x49,0x74,0xdb,0x14,0x6c,
	0x1a,0x56,0xed,0xa8,0x87,0x02,0x62,0xc2,0xb3,0xa5,
	0xb7,0x0a,0x39,0x65,0x9c,0x6a,0xf7,0xae,0x31,0x0e,
	0x8a,0x34,0x27,0x02,0xf1,0xdd,0x31,0xda,0xb8,0x5c,
	0xd2,0x0f,0x25,0x9b,0xca,0xd0,0xcb,0xc9,0x21,0x22,
	0x79,0x47,0x06,0x87,0x60,0x77,0x8a,0x1e,0x87,0xf4,
	0xb6,0xca,0x69,0xa5,0x99,0x4c,0x08,0x62,0x1b,0x8d,
	0x93,0x05,0xc1,0x89,0xf8,0xb4,0x48,0xa7,0xb1,0x91,
	0xe4,0x05,0x08,0x49,0xbe,0x1d,0x0e,0x66,0x3b,0x6c,
	0x30,0x41,0x4d,0x21,0x45,0x88,0xec,0x9f,0xe9,0x2c,
	0xac,0x16,0x2e,0x33,0x97,0x01,0x4a,0x48,0x00,0x85,
	0x9e,0xc1,0xc0,0x66,0x14,0xde,0x06,0x09,0xad,0x6c,
	0x9e,0x0a,0x00,0xa8,0xb4,0x8c,0x3a,0x76,0x3d,0x74,
	0x78,0x1a,0xc8,0x29,0xcb,0x98,0x50,0x00,0x00,0x74,
	0x00,0x00,0x00,0x68,0x34,0xcc,0x5a,0x0e,0x44,0x0f,
	0xd2,0x1b,0x26,0x28,0x80,0x10,0x40,0x07,0x3d,0xe0,
	0xba,0x41,0x01,0x09,0x41,0x04,0x5a,0xe8,0x39,0x0d,
	0xd0,0x43,0x0b,0x00,0x80,0x0e,0x06,0x0e,0x23,0x0c,
	0xd0,0x41,0x09,0x00,0xb5,0x9e,0x09,0x00,0x00,0x00,
	0x80,0x0a,0x46,0xd8,0x0d,0xc2,0x1b,0x09,0x2d,0x14,
	0x00,0x21,0xe8,0xa0,0x87,0x1e,0x3a,0x68,0x20,0x87,
	0x98,0x10,0x68,0xa0,0x81,0x56,0xf4,0x68,0xa4,0x9e,
	0x9a,0x19,0x4a,0x20,0xa0,0x51,0x30,0x2b,0x39,0x54,
	0x50,0x71,0xdf,0x05,0xa4,0x52,0x22,0x40,0x2f,0x61,
	0x95,0x72,0xc9,0xc4,0xef,0x97,0xc9,0x1e,0x99,0x7c,
	0x91,0x50,0x01,0x98,0x9f,0x80,0xe3,0x09,0x85,0xcf,
	0x1b,0x4b,0xfe,0x37,0x91,0xbf,0x57,0xa2,0x20,0x46,
	0x11,0x02,0xf6,0x03,0xf7,0x19,0x88,0xbe,0x2b,0x90,
	0xb9,0x43,0xe5,0x77,0x46,0xca,0x75,0x8c,0x64,0x35,
	0x80,0x73,0xc1,0xbb,0x91,0xfc,0x3b,0xb9,0x33,0x50,
	0xfd,0x47,0xc8,0x26,0x84,0x40,0x04,0xee,0x91,0xe8,
	0x5b,0x48,0x6d,0x14,0x0e,0x6a,0x7b,0xa0,0x32,0x00,
	0x40,0xf8,0x75,0x24,0xff,0x40,0x61,0xa6,0xb6,0xd2,
	0x59,0x69,0xf4,0x81,0x42,0x0e,0xc8,0xff,0x8c,0x5a,
	0x49,0xa7,0x61,0x34,0x30,0x19,0xe9,0xd6,0x96,0xb2,
	0xca,0x00,0xc3,0x1b,0x30,0x89,0x59,0xe4,0x6c,0x1a,
	0x56,0x3d,0xc3,0xd2,0xa0,0x4c,0x01,0xdb,0x85,0xfd,
	0x8b,0x38,0x64,0x1c,0x2a,0xf6,0xaf,0x63,0x04,0x32,
	0xc0,0x71,0xe2,0x80,0x13,0x4e,0x35,0xc0,0x0c,0x00,
	0xf6,0x1b,0xfe,0x98,0x43,0xc1,0xa1,0x81,0x9e,0x41,
	0x8b,0x02,0x60,0x7b,0xb0,0x4b,0xa0,0x84,0x8e,0xcd,
	0x00,0x20,0x07,0x78,0x43,0x36,0x29,0xbb,0x0a,0x7a,
	0xbc,0x23,0x74,0x00,0x01,0xd6,0x2f,0x82,0x0c,0x6a,
	0x76,0x03,0x4c,0x00,0x00,0x96,0x3f,0x86,0x1c,0x1a,
	0x18,0x59,0xa1,0x9f,0x7a,0x0a,0x00,0x80,0x02,0x5a,
	0x00,0x33,0x40,0x5e,0x00,
};


// Convert 5 bit brightness into 8 bit LED duty cycle

// Note that we could keep this in program memory but we have plenty of static RAM right now 
// and this way makes slightly smaller code

#define DUTY_CYCLE_SIZE 32

const byte dutyCycle32[DUTY_CYCLE_SIZE] = {
	0,     1 ,     2,     3,     4,     5,     7,     9,    12,
	15,    18,    22,    27,    32,    38,    44,    51,    58,
	67,    76,    86,    96,   108,   120,   134,   148,   163,
	180,   197,   216,   235,   255,
};

#define getDutyCycle(b) (dutyCycle32[b])

#define FDA_X_MAX 5
#define FDA_Y_MAX 8

#define FDA_SIZE (FDA_X_MAX*FDA_Y_MAX)

// This is the display array!
// Array of duty cycles levels. Brightness (0=off,  255=brightest)
// Anything you put in here will show up on the screen

// Now fda is linear - with x then y for easy scanning in x direction

// Graph paper style - x goes left to right, y goes bottom to top 

byte fda[FDA_SIZE];

// Set up the pins - call on startup

static inline void fdaInit() {

	// set all rows and cols to Hi-Z so nothing shows up on the screen

	DDRB = 0;
	PORTB= 0;        

	DDRD = 0;
	PORTD =0;
}

#define REFRESH_RATE 50			// Display Refresh rate in Hz

#define TIMECHECK 1				// Twittle bits so we can watch timing on an osciliscope
								// PA0 (pin 5) goes high while we are in the screen refreshing/PWM interrupt routine
								// PA1 (pin 4) goes high while we are decoding the next frame to be displayed

static inline void initInt()
{
	
	#ifdef TIMECHECK
		// Use porta just for debuging on osciliscope
	
		PORTA=0x00;
		DDRA = 0xff; //_BV(0);
		
	#endif


	// initialize timer1

	cli();           // disable all interrupts
	
		
	// TCCR1A Timer/Counter1 Control Register A, Controls the timer output pins, which we will not use
		
	TCNT1  = 0;					// Timer/Counter(TCNT1), start it at zero (seems safe)

	OCR1A = (F_CPU/(REFRESH_RATE*FDA_SIZE));	// Output Compare Registers(OCR1A), interrupt every this many cycle. 50hz refresh rate * 40 pixels = hopefully no flicker

	TCCR1B =  _BV( WGM12 ) | 	// Timer/Counter Control Registers(TCCR1A/B), CTC mode - Clear Timer on Compare Match (CTC) Mode
		
			    _BV( CS10  );		// x1 (no) prescaler - timer runs at clk
		
	TIMSK |= _BV( OCIE1A );		// OCIE1A: Timer/Counter1, Output Compare A Match Interrupt Enable
			

	sei();             // enable all interrupts
	
}

#define ROWS FDA_Y_MAX
#define COLS FDA_X_MAX

// Row 0 = Top

//ATTINY 4313

static byte const rowDirectionBits = 0b01010101;      // 0=row goes low, 1=Row goes high

static byte const portBRowBits[ROWS]  = {_BV(0),_BV(0),_BV(2),_BV(2),_BV(4),_BV(4),_BV(6),_BV(6) };
static byte const portDRowBits[ROWS]  = {     0,     0,     0,     0,     0,     0,     0,     0 };    


// Note that col is always opposite of row so we don't need colDirectionBits
	
static byte const portBColBits[COLS] = {_BV(7),_BV(5),_BV(3), _BV(1),     0};
static byte const portDColBits[COLS] = {     0,     0,      0,     0,_BV(6)};
		
// These keep track of the pixel to display on the next interrupt
static byte int_y = FDA_Y_MAX-1;
static byte int_x = FDA_X_MAX-1;

#define NOP __asm__("nop\n\t")

static unsigned int refreshCount=0;		// how many times have we refreshed the screen so far?

#define REFRESH_PER_FRAME ( REFRESH_RATE / FRAME_RATE )		// How many refreshes before we trigger the next frame to be drawn?

static volatile byte nextFrameFlag=0;	// Signal to the main thread that it is time to update the display with the next frame in the animation

static volatile int test_state=0;		// Keep track of current test mode pattern

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
	
  #ifdef TIMECHECK          
    PORTA |=_BV(0);      // twiddle A0 bit for oscilloscope timing
  #endif
  
  // get the brightness of the current LED

  byte b = fda[ (int_y*FDA_X_MAX) + int_x ];
  
  // If the LED is off, then don't need to do anything since all LEDs are already off all the time except for a split second inside this routine....

  if (b>0) {

     // Assume DDRB = DDRD = 0 coming into the INt since that is the Way we should have left them when we exited last...

      byte ddrbt;
      byte ddrdt;

      if ( rowDirectionBits & _BV( int_y ) ) {    /// for this row, row pin is high and col pins are low....

          // Only need to set the correct bits in PORTB and PORTD to drive the row high (col bit will get set to 0)

          PORTB = portBRowBits[int_y];             // Row pins go high level
          PORTD = portDRowBits[int_y];             // this will activate PULL-UP on the high pin, which is ok...

          ddrbt  = PORTB | portBColBits[int_x] ;       // enable output for the Row pins to drive high, also enable output for col pins which are zero so will go low
          ddrdt  = PORTD | portDColBits[int_x] ;

      } else {      // row goes low, cols go high....

          PORTB = portBColBits[int_x];                        // Col pins go high level
          PORTD = portDColBits[int_x];

          ddrbt  = PORTB | portBRowBits[int_y];               // enable output for the col pins to drive high, also enable output for row pins which are zero so will go low
          ddrdt  = PORTD | portDRowBits[int_y];
		  
      }


      while (b--) {


        DDRB = ddrbt;
        DDRD = ddrdt;

        // Ok, LED is on now!

        //NOP;    // ....wait.....a....split...second.....

        DDRB = 0;
        DDRD = 0;

        // ...and off again

      }
  }

  // now get ready for next pass...
  // run backwards because decrement/compare zero is supposed to be slightly faster in AVR

  if ( int_x-- == 0  ) {  // On left col

    int_x = FDA_X_MAX-1;                // retrace to right col

    if ( int_y-- == 0 ) { // on top row?
  
	  int_y = FDA_Y_MAX-1;
	  
	  refreshCount++;
	  
	  if (refreshCount >= REFRESH_PER_FRAME ) {
		  
		  nextFrameFlag = 1;
		  refreshCount=0;
	  }
	  
    }

  }

  // Just a check to see if we can keep up
  // if LED L in an ardunio (digital PIN 13) is lit, then we missed an intterrupt

  #ifdef TIMECHECK
    PORTA &= ~_BV(0);
  #endif
             
}

/*
static inline byte getNextBit() {

  if (bitsLeft==0) {
    workingByte=pgm_read_byte_near(candle_bitstream_ptr++);
    bitsLeft=8;
  }

  byte retVal = workingByte & 0x01;    // Extract the bottom bit

  workingByte >>= 1;                   // shift working byte down a notch
  bitsLeft--;
  
  return( retVal );
}

*/

// ger rid of semaphores so it runs in the simulator

//#define SIMULATOR 

// copy the next frame from program memory (candel_bitstream[]) to the RAM frame buffer (fda[])
// assumes that the compiler will set the fda[] to all zeros on startup....

void displayNextFrame() {
	
	  static byte const *candle_bitstream_ptr;     // next byte to read from the bitstream in program memory

	  static byte workingByte;			  // current working byte
	  
	  static byte workingBitsLeft;      // how many bits left in the current working byte? 0 triggers loading next byte
	  
	  static framecounttype frameCount = FRAMECOUNT;		// what frame are we on?

      #ifdef TIMECHECK
		PORTA |= _BV(1);
	  #endif
	  	  
	  if ( frameCount==FRAMECOUNT ) {							// Is this the last frame? 
		  
			memset( fda , 0x00 , FDA_SIZE );			// zero out the display buffer, becuase that is how the encoder currently works
			candle_bitstream_ptr=candle_bitstream;		// next byte to read from the bitstream in program memory
			workingBitsLeft=0;							// how many bits left in the current working byte? 0 triggers loading next byte
			frameCount= 0;
			
	  } else {		  
		  
		  frameCount++;
		  
	  }
	  
	  byte fdaIndex = FDA_SIZE;		// Which byte of the FDA are we filling in? Start at end because compare to zero slightly more efficient and and that is how data is encoded
	  			 	  
	  do {			// step though each pixel int the fda
		  
			fdaIndex--;
		  
			if (workingBitsLeft==0) {						
					
				workingByte=pgm_read_byte_near(candle_bitstream_ptr++);
				workingBitsLeft=7;
					
			} else {						
				
				workingByte >>=1;
				workingBitsLeft--;								
				
			} 
			
											
			if ( (workingByte & (byte) 0x01) != (byte) 0x00 ) {		// 1 bit indicates that this pixel has changed and next 5 bits are brightness

		  		byte brightness=0;
				  
				byte brightnessBitsLeft = 5; 
					
				do {						
					
					if (workingBitsLeft==0) {
							
						workingByte=pgm_read_byte_near(candle_bitstream_ptr++);
						workingBitsLeft=7;
							
					} else {
							
						workingByte >>=1;
						workingBitsLeft--;
												
					}			  
					

			  
					brightness <<= 1;
					brightness |= (workingByte & 0x01);		// ASM rotate so much better here...
						
					brightnessBitsLeft--;
						
				} while (brightnessBitsLeft>0);
			  
			  	  	  
				fda[fdaIndex] = getDutyCycle(brightness);		  
					
			}
						  
	  } while ( fdaIndex > 0 ); 
	  
	  #ifdef TIMECHECK
		PORTA  &= ~ _BV(1);
	  #endif 	  	
}

static inline void playVideo() {
	
  
	while (1) {
		
		
		displayNextFrame();
		
		#ifndef SIMULATOR
			while (!nextFrameFlag);
		#endif
					
		nextFrameFlag = 0;
	
	  }

}

static inline void setup() {

  // On boot, clock prescaler will be 8. Lets set it to 1 to get full speed
  // Note that no interrupts should be on yet so we'll be sure to hit the second step in time (you only get 4 cycles).
  
  CLKPR = (1 << CLKPCE); // enable a change to CLKPR
  CLKPR = 0; // set the CLKDIV to 0 - was 0011b = div by 8
  
  fdaInit();
  
  #ifndef SIMULATOR
	  initInt();
  #endif
  
   
}


static void scanscreen( byte duty_cycle ) {

	for( int x=0;  x < FDA_SIZE ; x++ ) {			// Scan the screen bottom-left to top-right
		
		fda[x]=duty_cycle;
		
		#ifndef SIMULATOR
			while (!nextFrameFlag);
		#endif

		nextFrameFlag = 0;		
	}
		
}

static inline void testpattern() {
	
	scanscreen( getDutyCycle( DUTY_CYCLE_SIZE-1 ) );		// First turn on all the LEDs
	scanscreen( getDutyCycle( 0 ) );						// Then turn them off

}

static inline void loop()
{
	
	playVideo();
	
}




int main(void)
{	
	
	setup();

/*		for(int z=0; z< FDA_SIZE ; z++) {
				fda[z] = z * (255/40);		
		
		}

	
	DDRA = 0xff;
	
	while (1) {
	
		
		_delay_ms( 1000/15 );    // Current video was at 15 fps
		
		
		_delay_ms( 1000/15 );    // Current video was at 15 fps
			
	}
	
	*/

		testpattern();
	

	    while(1)
    {

        loop();
    }
}