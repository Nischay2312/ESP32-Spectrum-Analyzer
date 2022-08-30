/*
    * DisplayFunctions.h
    *
    *  Created on: Aug 18, 2022
    *  This header file contains the constants and defines needed to compute
    *   all the visual stuff on the spectrum analyser. 
    *  
*/
#ifndef _DISPLAYFUNCTIONS_H
#define _DISPLAYFUNCTIONS_H

#include <TFT_eSPI.h>
#include <Math.h>
#include <stdio.h>

//Defines
#define PlotType 2                                      //2 for line, 1 for shaded, 0 for line 
#define VIEW_SCALE 1                                    //The scale for the signal to be displayed. 
                                                        //1-> Full scale, 2-> half, 3-> 1/4th, 4-> 1/8th, 
                                                        //any other will default to full scale 

#define startX 0                                        //Start X coordinate for display box
#define startY 0                                        //Start Y coordinate for display box
#define BoxW 160                                        //Width of display box
#define BoxH 100                                        //Height of display box
#define TEXT_startX 0                                   //Start X coordinate for text              
#define TEXT_startY 105                                 //Start Y coordinate for text
#define TEXT_WIDTH 20                                   //Width of text box    
#define TEXT_HEIGHT 10                                  //Height of text box
//These are parameters for the other text that will
//printed on the display
#define TEXT2_startX 120
#define TEXT2_startY 105
#define TEXT2_WIDTH 40
#define TEXT2_HEIGHT 10 
#define TEXT3_startX 0
#define TEXT3_startY 116
#define TEXT3_WIDTH 80
#define TEXT3_HEIGHT 10
///////////////////////
#define UpperYcut 2700                                  //Upper cutoff for plotting the sampled data
#define LowerYcut 000                                   //Lower cutoff for plotting the sampled data

#define FPSdesired 24                                   //Desired FPS for the display(max 30)
#define FPSDelayMS 1/(FPSdesired*1.0)*1000              //The time we have to wait for between each new frame
#define CLRSCREENCNTR 500                               //Reset Full screen after this many frames

#define FFTPLOT_CHANNEL 80                              //The channels on the FFF plot
#define FFTPLOT_FREQ_START 50                          //The starting frequency for the FFT plot
#define FFTPLOT_FREQ_END 4500                          //The ending frequency for the FFT plot
#define FFTPLOT_THRESHOLD_LOWER 0
#define FFTPLOT_THRESHOLD_UPPER 80000

#define ColorChangeThreshold 1                          //The Speed at which FFT spectrum plot change color
#define Rainbow 1                                       //If we want to cycle the color of RGB plot(1). O/W plot will be a set color(0).
#define BG_Color  TFT_WHITE//0x5269                                //BG Color for all plots
#define FFTPLOT_DEFAULT_COLOR TFT_WHITE                 //Default color of the Plot

#define PUSH_BUTTON_PIN 22                              //The pin that is connceted to push button to toggle Plot Mode

//Global Variables
const   int FPSDelay = ceil(FPSDelayMS);                //The Delay in Milliseconds between each new frame.
extern  int Wskip;                                      //Used in plotting the data on the screen. 
extern  int DispBufferElements;                         //Number of elements in the display buffer, used in plotting function
extern  int Ymax;                                       //Used in plotting the sampled data
extern  int Ymin;                                       //Used in plotting the sampled data
extern  int MaxBarHeight;
extern  unsigned long frame;                            //Used to keep track of how many frames have been displayed                                        
extern  unsigned long ttime_start;                      //Used to keep track of how long the program has been running

//Function Prototypes
void    TFTsetup(TFT_eSPI &tft);
void    SetViewScale(Stream &Serial);
void    PlotSampledData(TFT_eSPI &tft, float* AnalogValue_re, double avg, double fps, uint16_t PlotColor);
void    PrintSampledData(Stream &Serial, float* AnalogValue_re);
double  GetFrameRate(unsigned long timeNow);
void    PlotFFTBarGraph(TFT_eSPI &tft, uint32_t *DisplayData, int Channel, float FPeak, double fps, uint16_t PlotColor);

//Structure for keeping track of Button Presses
struct Button{
  const uint8_t PIN;
  uint16_t NumPresses;
  bool state;
};

//Class for RGB color of Plot
class RGBColor {
  private:
    uint16_t RGB;
    uint16_t Red;
    uint16_t Blue;
    uint16_t Green;
    uint8_t state;
    uint8_t ColorUpdateValue;
    unsigned long startFrame;
  
  public:
    RGBColor(uint8_t UpdateValue);                    //constructor
    void SetColor(uint16_t Color);                    //Set the color to a user defined value
    void UpdateRainbow();                             //Set RGB to cycle in Rainbow spectrum
    uint16_t RGBValue();                              //Get the Current RGB value.
    void UpdateRainbowSpeed(uint8_t Value);           //Change the rate at which the Rainbow cycle runs.
    void SetFrame(unsigned long target);              //Set the value of the frame
    unsigned long GetFrame();                         //Get the value of the frame
};
#endif //_DISPLAYFUNCTIONS_H
