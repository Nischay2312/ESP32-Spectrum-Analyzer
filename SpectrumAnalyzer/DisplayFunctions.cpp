/*
*   DisplayFunctions.cpp
*   Created on: Aug 18, 2022
*   Holds functions required to do the visual computing of the spectrum analyser. 
*/
#include "DisplayFunctions.h"
#include "SignalSampler.h"

//Define the glolbal variables 
int Ymin;
int Ymax;
int MaxBarHeight = 0;
int Wskip;                                       //Used in plotting the data on the screen. 
int DispBufferElements;
uint16_t screencounter = 1;
unsigned long frame = 0;
unsigned long ttime_start = 0; 


//Functions

/*
*   Function to setup the TFT screen.
*   Input: TFT_eSPI &tft - Reference to the TFT screen.
*   Output: None.
*/
void TFTsetup(TFT_eSPI &tft){
// Use this initializer if using a 1.8" TFT screen:
  tft.init();     // Init ST7735S chip
  tft.setRotation(3);
  tft.fillScreen(BG_Color);
  tft.setSwapBytes(true);
  tft.setCursor(0, 105);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(1);
}

/*
*   Function to initialize the View Scale of the Waveform.
*   Input: None.
*   Output: None.
*/
void SetViewScale(Stream &Serial){
    //Set up the Graphing Viewing Scale setting
    switch(VIEW_SCALE){
      case 1:
        Serial.println("VIEW_SCALE 1 set");
        DispBufferElements = BUFFER_SIZE;
        Wskip = floor(DispBufferElements / BoxW);
        break;
      case 2:
        Serial.println("VIEW_SCALE 2 set");
        DispBufferElements = BUFFER_SIZE/2;
        Wskip = floor(DispBufferElements / BoxW); 
        break; 
      case 3:
        Serial.println("VIEW_SCALE 3 set");
        DispBufferElements = BUFFER_SIZE/4;
        Wskip = floor(DispBufferElements / BoxW); 
        break;     
      case 4:
        Serial.println("VIEW_SCALE 4 set");
        DispBufferElements = BUFFER_SIZE/8;
        Wskip = floor(DispBufferElements / BoxW);
        break;
      default:
        Serial.println("VIEW_SCALE incorrect. Setting Default scale(1)");
        DispBufferElements = BUFFER_SIZE;
        Wskip = floor(DispBufferElements / BoxW);  
        break;    
    }
    if(DispBufferElements < BoxW){    //If this true, then a part of box will be empty as less data is presented.
      // override the above values.
      DispBufferElements = BoxW;
      Wskip = floor(DispBufferElements / BoxW);
      Serial.println("Viewing scale not possible based on current BOxW and Scale factor. Overriding!!!");
    }
}

/*
*   Function to plot the sampled data on the TFT screen.
*   Input: TFT_eSPI &tft - Reference to the TFT screen.
*   Input: double* AnalogValue_re - Reference to the array to store the sampled data.
*   Input: double avg - Average of the sampled data.
*   Input: double fps - The FPS value computed beforehand.
*   Input: bool printfps - Boolean to indicate if the FPS value should be printed. (1-> print, 0-> print avg)
*   Output: None.
*/
void PlotSampledData(TFT_eSPI &tft, float* AnalogValue_re, double avg, double fps, uint16_t PlotColor){

  unsigned long timee = micros();       //Legacy code, used to get the time spent in the function
  
  uint16_t SCRCLR = BG_Color;
    
  //Clear the screen if the screen counter is greater than CLRSCREENCNTR 
  if(screencounter > CLRSCREENCNTR){  //Clear the screen every once in a while.
    tft.fillScreen(SCRCLR);
    tft.drawRect(startX, startY, BoxW, BoxH, TFT_BLACK);
    screencounter = 0;
  }
  //increment the screen counter
  screencounter++;

  //Plot the data on the screen
  //clear rectangular display, i.e last waveform.
  tft.fillRect(startX+1, Ymin-2, BoxW-2, (Ymax-Ymin)+5, SCRCLR);                    //This is to clear the previous plot
  tft.fillRect(TEXT_startX, TEXT_startY, TEXT_WIDTH, TEXT_HEIGHT, SCRCLR);          //This is to clear the text that prints the average value 
  //Clear the rest two texts as well.
  tft.fillRect(TEXT2_startX, TEXT2_startY, TEXT2_WIDTH, TEXT2_HEIGHT, SCRCLR);
  //tft.fillRect(TEXT3_startX, TEXT3_startY, TEXT3_WIDTH, TEXT3_HEIGHT, SCRCLR); 
  //Draw the bounding box. Only if required
  if((Ymin <= (startY + 5)) || (Ymax >= (startY + BoxH - 5))){
    tft.drawRect(startX, startY, BoxW, BoxH, TFT_BLACK);
  }

  //Reset the Ymax and Ymin values. so as to get the max and min of the new data.
  Ymax = startY;
  Ymin = startY + BoxH;

  //Plot the sampled data.
  int Xpos = startX;
  int LineYposStart = map(avg, LowerYcut, UpperYcut, startY+BoxH, startY);      //This is the centre of the waveform
  //These two are used to generate the line plot. As we need 2 set of points.
  int Ylast = startY;
  int Xlast = startX;

  //This loop plots the points on the screen.
  for(int counter = 0; counter < DispBufferElements; counter += Wskip){
    
    //get the y cordinate based on the input
    int Ypos = map(AnalogValue_re[counter], LowerYcut, UpperYcut, startY+BoxH, startY);
    
    //plot the Signal
    if(PlotType == 1){  //Shaded Graph

        //Since fast V line can only be drawn in one direction, so we need to first find the direction.
        //Whether to plot the line from Ypos to the Ypos of the Average(i.e LineYposStart) or from the Average to Ypos.
        if(LineYposStart > Ypos){
            int _height = LineYposStart - Ypos;
            if(Ypos < (startY + 1)){    //Check if the Ypos is out of the box range, and calculate the height accordingly.
            _height = LineYposStart - (startY + 1); 
            }                
            tft.drawPixel(Xpos, Ypos, PlotColor);
            tft.drawFastVLine(Xpos, Ypos, _height, PlotColor);  
        }
        else{
            int _height = Ypos - LineYposStart;
            if(Ypos > (startY + BoxH - 1)){
                _height = startY + BoxH - 1 - LineYposStart;
            }
            tft.drawPixel(Xpos, Ypos, PlotColor);
            tft.drawFastVLine(Xpos, LineYposStart, _height, PlotColor);
        }
    }
    else if(PlotType == 0){   //Simple point plot
        if((Ypos < (startY + BoxH - 1)) && (Ypos > (startY + 1))){        //This statement checks whether the Waveform is inside the display Box. If outside, then dont draw it.
        tft.drawPixel(Xpos, Ypos, PlotColor);
        tft.drawPixel(Xpos, Ypos+1, PlotColor);      //The two extra pixels helps thicken the waveform
        tft.drawPixel(Xpos, Ypos-1, PlotColor);      //It looks better.
        //tft.drawFastVLine(Xpos, Ypos, (Ypos - LineYposStart), TFT_RED);
        }
    }
    else if(PlotType == 2){ //Line plot
      if((Ypos < (startY + BoxH - 1)) && (Ypos > (startY + 1))){        //This statement checks whether the Waveform is inside the display Box. If outside, then dont draw it.
        //Now check if its the first point. Otherwise draw a line between the current and last set of X and Y coordinates.
        if(Xpos > startX){
          //now draw the line
          tft.drawLine(Xlast, Ylast, Xpos, Ypos, PlotColor);
          tft.drawLine(Xlast, Ylast+1, Xpos, Ypos+1, PlotColor); //These extra are used to make the line thicker
          tft.drawLine(Xlast, Ylast-1, Xpos, Ypos-1, PlotColor);   
        }
        //update the last values
        Xlast = Xpos;
        Ylast = Ypos;
      }
      else{ //If the values go outside the display box.
        //update the last values
        Xlast = Xpos;
        if(Ypos > (startY + BoxW - 1)){ //if the value is outside the box from the bottom side i.e Ypos > startY + BoxW -1
          Ylast = startY + BoxW - 1;
        }
        else if(Ypos < (startY + 1)){ //if the value is outside the box from the top side.
          Ylast = startY + 1;
        }
      }
    }
    //Save the largest and smallest Pixels for the box to refresh the display
    if(Ypos > Ymax){
        Ymax = Ypos+1; 
    }
    else if(Ypos < Ymin){
        Ymin = Ypos-1;
    }
    //update the display Xpos counter 
    Xpos++;
  }

  //tft.drawRect(startX+1, Ymin, BoxW-2, (Ymax-Ymin), TFT_GREEN);   //This is the bounding box of the waveform. Uncomment to visualizise how the program deletes a specific region.

  //Now print the text.
  tft.setCursor(TEXT_startX, TEXT_startY);
  tft.print((int)fps);    //printing FPS
  tft.setCursor(TEXT2_startX, TEXT2_startY);
  tft.print((int)avg);    //printing average value read
  tft.setCursor(TEXT3_startX, TEXT3_startY);
  tft.print("WAVEFORM PLOT");
  
  timee = micros() - timee;                 //Calculate the time taken to plot the data.
  // Serial.print("PLOTsTime: ");              //Print the time taken to plot the data.
  // Serial.println(timee);    

}

void  PlotFFTBarGraph(TFT_eSPI &tft, uint32_t *DisplayData, int Channel, float FPeak, double fps, uint16_t PlotColor){
  unsigned long timee = micros();       //Legacy code, used to get the time spent in the function
  //Clear Screen
  uint16_t SCRCLR = BG_Color;
  //tft.fillScreen(SCRCLR);
  //tft.fillRect(startX+1, startY+1, BoxW-2, BoxH-2, SCRCLR);                                     //This is to clear the previous plot
  tft.fillRect(startX+1, startY + BoxH - MaxBarHeight, BoxW-2, MaxBarHeight, SCRCLR);             //Plot based clear screen, potentially faster.
  tft.fillRect(TEXT_startX, TEXT_startY, TEXT_WIDTH, TEXT_HEIGHT, SCRCLR);                        //This is to clear the text that prints the average value 
  //Clear the rest two texts as well.
  tft.fillRect(TEXT2_startX, TEXT2_startY, TEXT2_WIDTH, TEXT2_HEIGHT, SCRCLR);
  //tft.fillRect(TEXT3_startX, TEXT3_startY, TEXT3_WIDTH, TEXT3_HEIGHT, SCRCLR);

  MaxBarHeight = 0;
  
  //Now plot the bar graph  200000
  uint16_t BarWidth = floor(BoxW / Channel);
  uint16_t BarHeight = 0;
  uint16_t Xpos = startX;
  uint16_t Ypos = startY + BoxH;
  for(int i = 0; i < Channel; i++){
    if(DisplayData[i] > FFTPLOT_THRESHOLD_UPPER){
      BarHeight = BoxH;   
    }
    else if(DisplayData[i] < FFTPLOT_THRESHOLD_LOWER){
      BarHeight = 0;
    }
    else{
      BarHeight = map(DisplayData[i], FFTPLOT_THRESHOLD_LOWER, FFTPLOT_THRESHOLD_UPPER, 0, BoxH);
    }
    Ypos = Ypos - BarHeight;

    if(BarHeight > MaxBarHeight){
      MaxBarHeight = BarHeight;
    }
    //Serial.printf("%d %d %d %d %d\n",DisplayData[i], Xpos, Ypos, BarWidth, BarHeight);
    //Now we have the rectangle's starting X and Y, and width and height. draw it.
    tft.fillRect(Xpos, Ypos, BarWidth, BarHeight, PlotColor);

    //Now increment the Xposition for the next bar.
    Xpos += BarWidth;
    Ypos = startY + BoxH;
  }
    //Draw Box
  tft.drawRect(startX, startY, BoxW, BoxH, TFT_BLACK);
    //Now print the text.
  tft.setCursor(TEXT_startX, TEXT_startY);
  tft.print((int)fps);                        //Print the Framerate if required.
  tft.setCursor(TEXT2_startX, TEXT2_startY);
  tft.print((int)FPeak);                      //Print the dominant frequency
  tft.setCursor(TEXT3_startX, TEXT3_startY);
  tft.print("FREQUENCY PLOT");
  timee = micros() - timee;                   //Calculate the time taken to plot the data.
//   Serial.print("PLOTsTime: ");             //Print the time taken to plot the data.
//   Serial.println(timee);  
//  
}

/*
*   Function to plot the sampled data on the TFT screen.
*   Input: Serial &Serial - Reference to the Serial port.
*   Input: double* AnalogValue_re - Reference to the array to store the sampled data.
*   Prints to the Serial Monitor the sampled data.
*/
void PrintSampledData(Stream &Serial, float* AnalogValue_re){
    Serial.println("Data acqusition Finish");
    //delay(1000);
    //print data
    for(int counter = 0; counter < BUFFER_SIZE; counter++){
        Serial.println(AnalogValue_re[counter]);
    }
}

/*
*   Function to get the framerate.
*   Input: unsigned long* frames - Reference to the variable that stores the number of frames. 
*   Input: unsigned long startTime - The time at which the program started.
*   Input: unsigned long timeNow - Current time.
*   Output: double fps - The framerate.
*/
double GetFrameRate(unsigned long timeNow){
    float fps = 0.0;
    frame++;    
    fps = 1.0/((timeNow - ttime_start)*1.0/1000000.0)*(frame);
    // Serial.print("TimeStart: ");
    // Serial.println(ttime_start);
    // Serial.print("TimeNow: ");
    // Serial.println(timeNow);
    // Serial.print("Frames genrated: ");
    // Serial.println(frame);
    // Serial.print("FPS output: ");
    // Serial.println(fps);
    // Serial.println((timeNow - ttime_start)*1.0/1000000.0);
    // Serial.println(1.0/((timeNow - ttime_start)*1.0/1000000.0)*(frame));
    if(frame > 100){
      frame = 0;
      ttime_start = timeNow;
    }
    return fps;
}

//RGBColor Class Functions

//Constructor
RGBColor::RGBColor(uint8_t UpdateValue){
  //Initialize the variables
  RGB = 0;
  Red = 31;
  Blue = 0;
  Green = 0;
  state = 0;
  ColorUpdateValue = UpdateValue;
  startFrame = 0;
}

void RGBColor::SetColor(uint16_t Color){
  RGB = Color;
}

void RGBColor::UpdateRainbow(){
  if(state == 0){ //initial state. Increment green till 63.
    Green+= ColorUpdateValue;
    if(Green >= 63){
      state = 1; //Go to next state;
      Green = 63;
    }
    RGB = ((Red << 11)) + (Green << 5) + Blue;
  }
  else if(state == 1){  //Second state, decrease R till its 0
    Red-= ColorUpdateValue;
    if(Red < ColorUpdateValue){
      state = 2;  //go to next state
      Red = 0;
    }
    RGB = ((Red << 11)) + (Green << 5) + Blue;
  }
  else if(state == 2){ //Third state, increment blue till 31
    Blue+= ColorUpdateValue;
    if(Blue >= 31){
      Blue = 31;
      state = 3; //Go to next state
    }
    RGB = ((Red << 11)) + (Green << 5) + Blue;
  }
  else if(state == 3){  //Fourth State, decrease green till its 0
    Green-= ColorUpdateValue;
    if(Green < ColorUpdateValue){
      state = 4;
      Green = 0;
    }
    RGB = ((Red << 11)) + (Green << 5) + Blue;
  }
  else if(state == 4){  //Fifth state, increment Red till 31
    Red+= ColorUpdateValue;
    if(Red >= 31){
      state = 5;
      Red = 31;
    }
    RGB = ((Red << 11)) + (Green << 5) + Blue;
  }
  else if(state == 5){  //Sixth state, decrease blue untill 0
    Blue-= ColorUpdateValue;
    if(Blue < ColorUpdateValue){
      state = 0;  //Cycle complete, go to initial state.
      Blue = 0;
    }
    RGB = ((Red << 11)) + (Green << 5) + Blue;
  }
}

uint16_t RGBColor::RGBValue(){
  return RGB;
}

void RGBColor::UpdateRainbowSpeed(uint8_t Value){
  ColorUpdateValue = Value;
}

void RGBColor::SetFrame(unsigned long target){
  startFrame = target;
}

unsigned long RGBColor::GetFrame(){
  return startFrame;
}
