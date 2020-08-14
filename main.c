/*---------------------By Sage Hansen-------------------------------------------
youtube channel: https://www.youtube.com/user/artsage1
I hope this helps with some of the basics of GBA programming.
Draw image & background, key input, timers, frames per second, and sound.
Most of the things needed to make a simple game. 
------------------------------------------------------------------------------*/
#include "h_gba.h"
#include "sound_drums.h"
#include "sound_clap.h"
#include "img_ball.h"
#include "img_background.h"
#include "img_numbers.h"
int bx=0,by=106;                                                                //ball's position
int goal=0;                                                                     //if a goal is scored
int score=0;                                                                    //number of goals made
int hdrth=0,Lhdrth=0,frames=0,sec=0,fps=0;                                      //fps counter

typedef struct                                                                  //sound variables
{
 const unsigned char* song;                                                     //pointer to sound's data array	
 int frequency;                                                                 //sound frequency
 int tic;                                                                       //increase up to sounds end
 int end;                                                                       //end of sound
}sounds; sounds sound[2];                                                       //array of sounds structs


void PlotPixel(int x,int y,u16 cc)	                                            //check for transparent pixels
{u16 temp; u16 *v=&VideoBuffer[y*120+x];
 if((cc & 0x00FF)==0)                                                           //bottom is transparent
 {
  if((cc & 0xFF00)==0){ return;}                                                //top is also transparent
  temp = (*v & 0x00FF);                                                         //bottom transparent, top is not, get screen pixel value
  temp |= cc;                                                                   //or it with c (to get the highpart of c in
  *v= temp;                                                                     //set value
 }
 else if((cc & 0xFF00)==0)                                                      //only the top is transprent
 {
  temp = (*v & 0xFF00);
  temp |= cc;
  *v = temp;
 }
 else{ *v=cc;}                                                                  //no part is transparent
}


void playSound(int s)
{
 REG_SOUNDCNT_H = 0x0B04;                                                       //REG_SOUNDCNT_H = 0000 1011 0000 0100, volume = 100, sound goes to the left, sound goes to the right, timer 0 is used, FIFO buffer reset
 REG_SOUNDCNT_X = 0x0080;                                                       //REG_SOUNDCNT_X = 0000 0000 1000 0000, enable the sound system, DMA 1
 REG_DM1SAD     = (unsigned long) sound[s].song;                                //REG_DM1SAD = NAME, address of DMA source is the digitized music sample
 REG_DM1DAD     = 0x040000A0;                                                   //REG_DM1DAD = REG_SGFIFOA, address of DMA destination is FIFO buffer for direct sound A
 REG_DM1CNT_H   = 0xB640;                                                       //REG_DM1CNT_H = 1011 0110 0100 0000, DMA destination is fixed, repeat transfer of 4 bytes when FIFO , buffer is empty, enable DMA 1 (number of DMA transfers is ignored), INTERRUPT
 REG_TM0D       = 65536-(16777216/sound[s].frequency);                          //REG_TM0D = 65536-(16777216/frequency);, play sample every 16777216/frequency CPU cycles
 REG_TM0CNT     = 0x00C0;                                                       //REG_TM0CNT = 0000 0000 1100 0000, enable timer 0, send Interrupt Request when timer overflows    
}


void drawNumber(int nx,int ny,int n)                                            //numbers 0-100
{int x,y, sn=n*5, en=sn+5;                                 
 for(y=sn;y<en;y++)                                                             //skip down to next number                     
 {
  for(x=0;x<4;x++){ VideoBuffer[(y+ny-sn)*120+(x+nx)]=numbersData[y*4+x];}      //draw this number
 } 
}


void background()                                                               //copy background to video buffer using DMA
{
 REG_DM3SAD = (unsigned long)background_Data;
 REG_DM3DAD = (unsigned long)VideoBuffer;
 REG_DM3CNT = 0x80000000 | 120*160;
}


void ball()                                                                     //draw the ball
{int x,y;
 for(y=0;y<54;y++)
 { 
  for(x=0;x<27;x++)                                                             //2 pixels are drawn at once
  { 
   PlotPixel(x+bx,y+by, ball_data[y*27+x] );
  }
 }
}


void oneSecReset()
{
 *(volatile unsigned short *) 0x04000106 = 0x0000;                              //Timer 1 disable
 *(volatile unsigned short *) 0x04000104 = 49152;                               //Timer 1 data register = 49152
}


void idle()
{int s;
 while(*ScanlineCounter<160){}                                                  //wait until frame is finished drawing
 frames+=1;                                                                     //count up a frame
 hdrth = (*(volatile unsigned short*)0x04000104-49152)*100/16384;               //100 ticks equals 1 second
 if(Lhdrth>hdrth)                                                               //1 second reached 
 { 
  sec+=1;     if(sec>100){ sec= 0;}                                             //increase second counter
  fps=frames; if(fps> 60){ fps=60;}                                             //increase frames per second 
  frames=0;                                                                     //reset counter                     
  oneSecReset();                                                                //reset hundreth counter
  *(volatile unsigned short *) 0x04000106 = 0x0083;                             
 } 
 
 if(goal==0&&sound[0].tic==0){ sound[0].tic=1; playSound(0); goal=0;}           //no goal,   turn on drums
 if(goal==1&&sound[1].tic==0){ sound[1].tic=1; playSound(1); goal=0;}           //goal done, turn off claping
 if(bx<86&&bx>71&&by>39&&by<61)                                                 //goal,      turn on clapping
 { 
  score+=1; if(score>9){ score=0;}                                              //increase score
  REG_DM1CNT=0;                                                                 //turn off sound
  sound[0].tic=0;                                                               //turn off drums
  sound[1].tic=1;                                                               //turn on clapping
  playSound(1);                                                                 //play clapping
  Lhdrth=hdrth;                                                                 //reset last hundreth
  goal=1;                                                                       //goal is scored
  bx=0;by=106;                                                                  //reposition ball
 }

 if(sound[0].tic>0){ s=0;}                                                      //if drums on, increase conter
 if(sound[1].tic>0){ s=1;}                                                      //if clap  on, increase counter

 if(Lhdrth>hdrth){ sound[s].tic+=(100-Lhdrth)+hdrth;}                           //increase sound tic
 else{             sound[s].tic+=hdrth-Lhdrth;      }

 if(sound[s].tic>=sound[s].end){ REG_DM1CNT=0; sound[s].tic=0; goal=0;}        //song ended, turn off
 Lhdrth=hdrth;
}


void keys()
{
 if(KEY_DOWN(KEYLEFT       )){ bx-=1;}                                          //left
 if(KEY_DOWN(KEYRIGHT      )){ bx+=1;}                                          //right
 if(KEY_DOWN(KEYS&KEYUP    )){ by-=2;}                                          //up
 if(KEY_DOWN(KEYS&KEYDOWN  )){ by+=2;}                                          //down 
 if(bx<0){ bx=0;} if(bx>120-27){ bx=120-27;}                                    //limit x
 if(by<0){ by=0;} if(by>160-54){ by=160-54;}                                    //limit y
}


void flip()                                                                     //flips between the back/front buffer
{
 if(REG_DISPCNT & BACKBUFFER)                                                   //back buffer is current buffer, switch to font buffer
 { 
  REG_DISPCNT &= ~BACKBUFFER;                                                   //flip active buffer to front buffer
  VideoBuffer = BackBuffer;                                                     //point drawing buffer to the back buffer
 }
 else                                                                           //front buffer is active so switch it to backbuffer
 { 
  REG_DISPCNT |= BACKBUFFER;                                                    //flip active buffer to back buffer by setting back buffer bit
  VideoBuffer = FrontBuffer;                                                    //now we point our drawing buffer to the front buffer
 }
}


int main(void)//----------------------------------------------------------------
{int i;
 SetMode(MODE_4|BG2_ENABLE);                                                    //mode 4, background 2
 
 oneSecReset();                                                                 //reset timer
 *(volatile unsigned short *) 0x04000106 = 0x0083;                              //start timer
  
 for(i=0;i<256;i++){ BGPaletteMem[i]=ball_palette[i];}   
 sound[0].song=drums; sound[0].frequency=22050; sound[0].end=433; sound[0].tic=0;//init drums
 sound[1].song=clap;  sound[1].frequency=22050; sound[1].end=200; sound[1].tic=0;//init clap

 while(1)
 { 
  keys();                                                                       //key input
  background();                                                                 //draw background usind DMA
  drawNumber(0,0,hdrth);                                                        //print hundreths of a second
  drawNumber(10,0,sec);                                                         //print seconds   
  drawNumber(20,0,fps);                                                         //print frames per second
  drawNumber(95,30,score);                                                      //print score  
  ball();	                                                                    //draw ball
  idle();                                                                       //idle function  
  flip();                                                                       //flip buffers
 }
}//-----------------------------------------------------------------------------

