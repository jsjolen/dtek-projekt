/*
  Copyright (c) 2015, Axel Isaksson, 2016 Johan Sjolen

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <pic32mx.h>
#include <stdint.h>
#include "projekt.h"
/* Not by project auuthors */
#define DISPLAY_VDD PORTFbits.RF6
#define DISPLAY_VBATT PORTFbits.RF5
#define DISPLAY_COMMAND_DATA PORTFbits.RF4
#define DISPLAY_RESET PORTGbits.RG9
#define DISPLAY_VDD_PORT PORTF
#define DISPLAY_VDD_MASK 0x40
#define DISPLAY_VBATT_PORT PORTF
#define DISPLAY_VBATT_MASK 0x20
#define DISPLAY_COMMAND_DATA_PORT PORTF
#define DISPLAY_COMMAND_DATA_MASK 0x10
#define DISPLAY_RESET_PORT PORTG
#define DISPLAY_RESET_MASK 0x200

/* Johan Sjolen */
/* 128 ty 32 rader om 4 tal*/
/* Every byte is an 8-strip column of pixels */
/* (0,0) = Upper-left corner */
uint8_t block0[128] =  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t block1[128] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t block2[128] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

uint8_t block3[128] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


void delay(int cyc) {
  int i;
  for(i = cyc; i > 0; i--);
}

void clear_screen(void){
  int i = 0;
  for(i = 0; i < 128; i++){
    block0[i] = 0;
    block1[i] = 0;
    block2[i] = 0;
    block3[i] = 0;
  }
  blit();
}
void error_screen(void) {
  int i = 0;
  for(i = 0; i < 128; i++) {
    block0[i] = i%10;
    block1[i] = i%10;
    block2[i] = i%10;
    block3[i] = i%10;
  }
  blit();
}


/* Johan Sjolen */
int expt(int b, int x) {
  int i = 1;
  int r = 1;
  while(x >= i){
    r = r * b;
    i++;
  }
  return r;
}
/* Johan Sjolen */
/*
 A block is a 32x32 pixel sized square.
There are four, smallest being furthest to the left
*/
int coord_to_block(int x, int y) {
  if(x < 32) {
    return 0;
  }
  if(x < 64) {
    return 1;
  }
  if(x < 96) {
    return 2;
  }
  if(x < 128) {
    return 3;
  }
  return 3; /* Who cares about error checking */
}
/* Johan Sjolen */
/*
write == 1 writes the relevant pos1
*/
uint8_t pos(int x, int y, int write){
  x = x % 128; // Bind it to 0..127
  y = y % 32; // Bind it t0 0..31
  int i = 0;
  int byteindex = 0; // The byte which we write to
  int bitpattern = 0; // The correct bit to write
  int bitindex = 0; // The right bit index which sets the bit pattern
  int block = coord_to_block(x, y);
  x = x - 32*block; // adjust to local block
  /* Determine which byte */
  /* Determine which out of four parts we are in from top to bottom */
  if (y < 32) {
    i = 3;
  }
  if( y < 24) {
    i = 2;
  }
  if(y < 16){
    i = 1;
  }
  if(y < 8) {
    i = 0; // It's the zeroth row
  }
  byteindex = x + i*32;
  bitindex = y % 8;
  bitpattern = expt(2, bitindex);
  // If we write we OR to set the appropriate bit to 1 and leave the others be
  // If we read we AND to get the value and bitshift to the right with the bitindex to put it in the first position
  if(block == 0){
    if(write == 0)
      return (block0[byteindex]  & bitpattern) >> bitindex;
    block0[byteindex] = block0[byteindex] | bitpattern;
  }
  if(block == 1){
    if(write == 0)
      return (block1[byteindex]  & bitpattern) >> bitindex;
    block1[byteindex] = block1[byteindex] | bitpattern;
  }
  if(block == 2){
    if(write == 0)
      return (block2[byteindex]  & bitpattern) >> bitindex;
    block2[byteindex] = block2[byteindex] | bitpattern;
  }
  if(block == 3){
    if(write == 0)
      return (block3[byteindex]  & bitpattern) >> bitindex;
    block3[byteindex] = block3[byteindex] | bitpattern;
  }
  return 0;
}

uint8_t get_pos(int x, int y) {
  return pos(x,y,0);
}
void put_pos(int x, int y) {
  pos(x,y,1);
}
/* not by project authors*/
uint8_t spi_send_recv(uint8_t data) {
  while(!(SPI2STAT & 0x08));
  SPI2BUF = data;
  while(!(SPI2STAT & 0x01));
  return SPI2BUF;
}
/* not by project authors*/
void display_init() {
  DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
  delay(10);
  DISPLAY_VDD_PORT &= ~DISPLAY_VDD_MASK;
  delay(1000000);
	
  spi_send_recv(0xAE);
  DISPLAY_RESET_PORT &= ~DISPLAY_RESET_MASK;
  delay(10);
  DISPLAY_RESET_PORT |= DISPLAY_RESET_MASK;
  delay(10);
	
  spi_send_recv(0x8D);
  spi_send_recv(0x14);
	
  spi_send_recv(0xD9);
  spi_send_recv(0xF1);
	
  DISPLAY_VBATT_PORT &= ~DISPLAY_VBATT_MASK;
  delay(10000000);
	
  spi_send_recv(0xA1);
  spi_send_recv(0xC8);
	
  spi_send_recv(0xDA);
  spi_send_recv(0x20);
	
  spi_send_recv(0xAF);
}

/* not by project authors*/
void display_image(int x, const uint8_t *data) {
  int i, j;
	
  for(i = 0; i < 4; i++) {
    DISPLAY_COMMAND_DATA_PORT &= ~DISPLAY_COMMAND_DATA_MASK;
    spi_send_recv(0x22);
    spi_send_recv(i);
		
    spi_send_recv(x & 0xF);
    spi_send_recv(0x10 | ((x >> 4) & 0xF));
		
    DISPLAY_COMMAND_DATA_PORT |= DISPLAY_COMMAND_DATA_MASK;
		
    for(j = 0; j < 32; j++)
      spi_send_recv(~data[i*32 + j]);
  }
}

 
/* not by project authors*/
int calculate_baudrate_divider(int sysclk, int baudrate, int highspeed) {
  int pbclk, uxbrg, divmult;
  unsigned int pbdiv;
	
  divmult = (highspeed) ? 4 : 16;
  /* Periphial Bus Clock is divided by PBDIV in OSCCON */
  pbdiv = (OSCCON & 0x180000) >> 19;
  pbclk = sysclk >> pbdiv;
	
  /* Multiply by two, this way we can round the divider up if needed */
  uxbrg = ((pbclk * 2) / (divmult * baudrate)) - 2;
  /* We'll get closer if we round up */
  if (uxbrg & 1)
    uxbrg >>= 1, uxbrg++;
  else
    uxbrg >>= 1;
  return uxbrg;
}

/* Johan Sjolen*/
/* read only if there is a value. If read was succesful and written to *c, then return 0. On fail, return 1 */
int read_char_no_hang(char* c) {
  if(U1STA & 0x1) {
    *c = U1RXREG & 0xFF;
    return 0;
  }
  return 1;
}

/* Johan Sjolen*/
// wait until read buffer has a value, then return the value
unsigned char read_char() {
  while(!(U1STA & 0x1)); //wait for read buffer to have a value
  return U1RXREG & 0xFF;
}
int read_command(char* str){
  str[0] = (char)read_char();
  str[1] = (char)read_char();
  str[2] = (char)read_char();
  return 0;
}

/* Johan Sjolen*/
// wait until there's space in the write buffer, then write c.
void send_char(unsigned char c) {
  while(U1STA & (1 << 9)); //make sure the write buffer is not full
  // Write 1 byte
  U1TXREG = c;
}
/* Johan Sjolen*/
// write string s of length l
void send_string(unsigned char* s, int l){
  int i;
  for(i = 0; i < l; i++) {
    send_char(s[i]);
  }
}

/* Johan Sjolen*/
/* Wait until TIMER2 flag is set to 1, then set it to 0 */
void wait_for_next_frame(void){
  while(!(IFS(0) & 0x100)){} /* Wait */
  IFS(0) = IFS(0) & ~0x100; /* Set to 0 */
}
/* Johan Sjolen*/
void blit(void) {
  display_image(0, block0);
  display_image(32, block1);
  display_image(64, block2);
  display_image(96, block3);
}

/*
Johan Sjolen
*/
void (*render_fn)(int,int) = &put_pos;
void render_coord_pair(double x0,double y0,double x,double y) {
  double k = (((double)y-(double)y0)/((double)x-(double)x0)); /* f(x) = kx */
  double m = y0 - k*x0;
  double dy = y0;

  // Straight up or down (they're not functions)
  if(x0 == x) {
    if(y0 >= y) { // down
      while(y0 >= y) {
    (*render_fn)((int)x0,(int)y0);
    y0--;
    blit();
    wait_for_next_frame();
      }
    }
    else {
      while(y >= y0) {
    (*render_fn)((int)x0,(int)y0);
    y0++;
    blit();
    wait_for_next_frame();
      }
    }
  }

  else {
    if(x0 > x) { // We're going left
      while(x0 != x) { // We're going downwards and to the left
    (*render_fn)((int)x0,(int)dy);
    blit();
    dy = x0*k + m;
    x0--;
    blit();
    wait_for_next_frame();
      }
    }
    else if(x0 <= x) { // We're going right
      while(x0 != x) { // We're going downwards to the right
    (*render_fn)((int)x0,(int)dy);
    blit();
    dy = x0*k + m;
    x0++;
    blit();
    wait_for_next_frame();
      } 
    }
  }
}
/* Johan Sjolen*/
int intbuffer[4];
int render_loop(void) {
  int x = 10; /* End coord */
  int y = 10; 
  int x0 = 0 ; /* Start coord */
  int y0 = 0;
  int is_finished = parse_textbuffer(intbuffer);
  
  while(is_finished == 0) {
    x0 = (double)intbuffer[0];
    y0 = (double)intbuffer[1];
    x = (double)intbuffer[2];
    y = (double)intbuffer[3];
    render_coord_pair(x0,y0,x,y);
    is_finished = parse_textbuffer(intbuffer);
  }
  return 0;
}
/* Johan Sjolen*/
int parse_digit(char d) {
  switch(d){
  case '0':
    return 0;
  case '1':
    return 1;
  case '2':
    return 2;
  case '3':
    return 3;
  case '4':
    return 4;
  case '5':
    return 5;
  case '6':
    return 6;
  case '7':
    return 7;
  case '8':
    return 8;
  case '9':
    return 9;
  default:
    return -1;
  }
}
  
/* Johan Sjolen*/
int parse_integer(char* arr){
  int exp = 100;
  int r = 0;
  int d = parse_digit(arr[0]);
  if(d == -1) {
    return -1;  // Crash
  }
  r += exp*d;
  exp = 10;
  d = parse_digit(arr[1]);
  if(d == -1) {
    return -1; // Crash
  }
  r += exp*d;
  exp = 1;
  d = parse_digit(arr[2]);
  if(d == -1) {
    return -1; // Crash
  }
  r += exp*d;
  return r;
}


/* Johan Sjolen*/
unsigned char textbuffer[4096]; /* The text buffer where everything from streaming is put in */
int textbufferlen = 4096; /* Just the total length of the textbuffer */
int length_used = 0; /* Amount of textbuffer filled up by reading */
int length_parsed = 0;
/* intbufferlen == whole intbuffer length that you pass in, intbuffer is where we place the integers, retlength is how much of the intbuffer we used up */
int parse_textbuffer(int* intbuffer) {
  char darr[3];  //  i  +1 +2 +3 +4 +5 i+6
  int integer;
  if(length_parsed >= length_used) {
    return  1;
  }
  // x0
  darr[0] = textbuffer[length_parsed];
  darr[1] = textbuffer[length_parsed+1];
  darr[2] = textbuffer[length_parsed+2];
  integer = parse_integer(darr);
  intbuffer[0] = integer;

  // y0
  darr[0] = textbuffer[length_parsed+3];
  darr[1] = textbuffer[length_parsed+4];
  darr[2] = textbuffer[length_parsed+5];
  integer = parse_integer(darr);
  intbuffer[1] = integer;

  // x
  darr[0] = textbuffer[length_parsed+6];
  darr[1] = textbuffer[length_parsed+7];
  darr[2] = textbuffer[length_parsed+8];
  integer = parse_integer(darr);
  intbuffer[2] = integer;

  // x
  darr[0] = textbuffer[length_parsed+9];
  darr[1] = textbuffer[length_parsed+10];
  darr[2] = textbuffer[length_parsed+11];
  integer = parse_integer(darr);
  intbuffer[3] = integer;
  length_parsed += 12;
  return 0;
}

/* Not written by project authors*/
void init() {
  /* On Uno32, we're assuming we're running with sysclk == 80 MHz */
  /* Periphial bust can run at a maximum of 40 MHz, setting PBDIV to 1 divides sysclk with 2 */
  OSCCON &= ~0x180000;
  OSCCON |= 0x080000;

  T2CON = 0x0; /* Stop timer and clear control register */
  T2CONSET = 0x70; /* Set prescaler to 1:256 */
  PR2  = 31250; /* Let's assume 10 FPS, then we have 100ms @ 80/256 MHz is 31250 Hz (80 000 000 / 256) / 10 */
  TMR2 = 0x0; /* Clear timer */
  T2CONSET = 0x8000; /* Start timer */
	
  /* Set up output pins */
  AD1PCFG = 0xFFFF;
  ODCE = 0x0;
  TRISECLR = 0xFF;
  PORTE = 0x0;
	
  /* Output pins for display signals */
  PORTF = 0xFFFF;
  PORTG = (1 << 9);
  ODCF = 0x0;
  ODCG = 0x0;
  TRISFCLR = 0x70;
  TRISGCLR = 0x200;
	
  /* Set up SPI as master */
  SPI2CON = 0;
  SPI2BRG = 4;
	
  /* Clear SPIROV*/
  SPI2STATCLR &= ~0x40;
  /* Set CKP = 1, MSTEN = 1; */
  SPI2CON |= 0x60;
	
  /* Turn on SPI */
  SPI2CONSET = 0x8000;
  return;
}

/* Johan Sjolen */
int comcmp(char *com1, char *com2) {
  int i = 0;
  while(com1[i] == com2[i] && i != 2) {
    i++;
  }
  return i == 2;
}

/* Business logic made by Johan Sjolen, calls to UART1 stuff not made by project authors */
int main(void) {
  // This delay() seems to be necessary
  // to make installs *always* work and
  // communication immediately work
  interrupt_init();
  init();
  delay(10000); // <- This one
  enable_interrupt();
  int ret;
  int i;
  char com[3];
  ODCE = 0;
  /* Configure UART1 for 115200 baud, no interrupts */
  U1BRG = calculate_baudrate_divider(80000000, 115200, 0);
  U1STA = 0;
  /* 8-bit data, no parity, 1 stop bit */
  U1MODE = 0x8000;
  /* Enable transmit and recieve */
  U1STASET = 0x1400;
  display_init();
  clear_screen(); // This is needed to clear previous screen contents
  for(;;){
    read_command(com);
    if(comcmp(com, "STA")){ /* We're receiving code */
      /* Reset parsing machine */
      length_used = 0;
      length_parsed = 0;
      /* Ok, send us the code */
      send_string("ACK",3);
      read_command(com);
      /* Insert the code */
      // Really the parsing could've been made inline here, which would've significantly reduced the text buffer overhead
      // This is ignored because simplicity wins big.
      while(!comcmp(com,"END") && length_used < textbufferlen){
  	textbuffer[length_used] = com[0];
  	textbuffer[length_used+1] = com[1];
  	textbuffer[length_used+2] = com[2];
  	length_used += 3;
  	read_command(com);
      }
      send_string("ACK",3); /* OK, we acknowledge END */
      /* Parse the code */
      int ret = render_loop();
      if(ret != 0) {
  	error_screen();
      }
    }
    else if(comcmp(com, "SEN")){ // Send the bitmap back
      int i = 0;
      int j = 0;
      for(i = 0; i < 128; i++) {
	for(j = 0; j < 32; j++) {
	  uint8_t p = get_pos(i,j);
	  send_char((char)p);
	}
      }
    }
    else if(comcmp(com, "CLE")) {
      clear_screen();
    }
  }
}
