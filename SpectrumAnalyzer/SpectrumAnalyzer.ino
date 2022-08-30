#include <TFT_eSPI.h>
#include <SPI.h>
#include <Math.h>
#include "SignalSampler.h"
#include "DisplayFunctions.h"
#include "SPALSH_SCREEN.h"
//#include "FFT.h"

//Use them to see the output of various functions on the serial monitor

#define DISPLAY_DATA_DEBUG    0               //Setting this to 1 will print all data for Display Buffer
#define BUTTON_DEBUG          0               //Setting this to 1 will print all data related to button
#define FFT_DATA_DEBUG        0               //Setting this to 1 will print FFT data
#define WAVEFORM_DEBUG        0               //Setting this to 1 will print all data for Waveform Plot
#define TIME_DEBUG            0               //Setting this to 1 will print time taken for each task.

TFT_eSPI tft = TFT_eSPI();

//----FOR FFT----
//Variables
float AnalogValue_re[BUFFER_SIZE];
float FFT_output[BUFFER_SIZE];
float Magnitude[BUFFER_SIZE/2 - 1];
float MajorFreq = 0.0;
double SignalAverage = 0.0;
//Initialization of Arduino FFT object
//arduinoFFT FFT = arduinoFFT(AnalogValue_re, AnalogValue_im, BUFFER_SIZE, ReadFreq);
fft_config_t *FFT = fft_init(BUFFER_SIZE, FFT_REAL, FFT_FORWARD, AnalogValue_re, FFT_output);
//Initialization of the Display Buffer
uint32_t *FFTPLOT_DisplayData = InitializeDisplayArray(FFTPLOT_CHANNEL);
bool editingDisplayData = false;
bool clearDisplay = false;
//--------

bool StartDelay = false;    //used in Processing Task

//RGB color Stuff
RGBColor FFTPLOT_Color = RGBColor(5);

//Button Structure for Plot Change Button
Button PlotChangeButton = {PUSH_BUTTON_PIN, 0, false};

//Interrupt function for Plot Mode change button
void IRAM_ATTR PlotModeChange(){
  PlotChangeButton.NumPresses++;
  PlotChangeButton.state = (PlotChangeButton.state)? false: true;   
  clearDisplay = true;  
}

//Main tasks decleration
TaskHandle_t DataProcessingTask;
TaskHandle_t DataVisualizationTask;

//Task Function Prototypes
void DataProcessingTask_Code(void *Parameter);
void DataVisualizationTask_Code(void *Parameter);

void setup() {
  //Setup Serial communication
    Serial.begin(115200);
  // Setup the TFT screen
    TFTsetup(tft);
  // Setup the ADC
    ADCSetup(Serial);
  // Setup the viewing scale
    SetViewScale(Serial);
  // Display the SPLASH Animation
    tft.pushImage(0,4,SPALSH_SCREEN.width,SPALSH_SCREEN.height,SPALSH_SCREEN.data);
    delay(5000);
    tft.fillScreen(BG_Color);
    delay(250);    
  // Setup Hardware interrupt for the PUSH Button
    pinMode(PlotChangeButton.PIN, INPUT);
    attachInterrupt(PlotChangeButton.PIN, PlotModeChange, RISING); 
  // Setup the tasks to run on different cores.
    xTaskCreatePinnedToCore(DataProcessingTask_Code, "ProcessingTask", 10000, NULL, 1, &DataProcessingTask, 0); 
    delay(500);
    xTaskCreatePinnedToCore(DataVisualizationTask_Code, "VisualizationTask", 10000, NULL, 1, &DataVisualizationTask, 1);
    delay(1000); 
    Serial.println("Setup Complete");
  //used for FrameRate calculations
    ttime_start = micros();
    frame = 0;
    FFTPLOT_Color.SetFrame(frame);
}

void loop() {
  //Empty Loop
}

//Tasks Definitions
void DataProcessingTask_Code(void *Parameter){
  while(1){
    unsigned long timee = micros();       //Legacy code, used to get the time spent in the function
    //This task deals with all the stuff that is associated with Data acqisition and processing

    //This delay is added to give system time to setup and get the samples.
    if(StartDelay){
      delayMicroseconds(25000);
      StartDelay = true;
    }
    //1. Get the sampled data
    SignalAverage = GetSampledData(AnalogValue_re);
    //Serial.print("Got Signal\n");
    if(PlotChangeButton.state){ //No need if we are only using waveform plot i.e state = 0
      //2. Compute FFT and get frequency data
      MajorFreq = ComputeFFT(FFT, Magnitude);
      //Serial.println("GOT FFT Data");
      //Print the FFT (if required)
      if(FFT_DATA_DEBUG){
        PrintFFT(Serial, FFT_output, BUFFER_SIZE);
      }
      
      //This delay will ensure that watch dog timers are reset.
      vTaskDelay(xDelay); 

      //3. Prepare the FFT data for Displaying.
//      while(editingDisplayData){ //Wait if the Display Buffer is beings used by processing task
//        Serial.println("DP Waiting for DispBuffer");
//      } 
      editingDisplayData = true;
      PrepareDisplayData(FFT_output, FFTPLOT_FREQ_START, FFTPLOT_FREQ_END, FFTPLOT_CHANNEL, ReadFreq, BUFFER_SIZE, FFTPLOT_DisplayData);
      editingDisplayData = false;
      //Serial.println("GOT Display Data");
      
      //4. Get Major Frequency from our data
//      MajorFreq = FFT.MajorPeak(AnalogValue_re, BUFFER_SIZE, ReadFreq);
      //Serial.printf("Major Frequency: %.6lf\n", MajorFreq);      
      //Print the Display Data obtained (if required)
      if(DISPLAY_DATA_DEBUG){
        for(int i = 0; i < FFTPLOT_CHANNEL; i++){
          Serial.println(FFTPLOT_DisplayData[i]);
        }
      }
    }
    if(TIME_DEBUG){
      Serial.print("Time Taken by Processing Task:");
      Serial.println(timee);
    }
  }
}


void DataVisualizationTask_Code(void *Parameter){
while(1){
   unsigned long timee = micros();       //Legacy code, used to get the time spent in the function
  //This task deals will all the stuff associated with displaying and visualization of the FFT Data.
  
  //Get FrameRate  
  double frate = GetFrameRate(micros());

  //If button was pressed recently, then clear the last plot type.
  if(clearDisplay){
    tft.fillScreen(BG_Color);
    clearDisplay = false;
  }
  
  //  Print Button State: 
  if(BUTTON_DEBUG){
    Serial.printf("\nButton state: %i, presses: %d\n", PlotChangeButton.state, PlotChangeButton.NumPresses); 
  }
  //Do Color Stuff
  uint16_t PlotColor = Rainbow?FFTPLOT_Color.RGBValue(): FFTPLOT_DEFAULT_COLOR;
  
  if(PlotChangeButton.state){  //Based on button state, plot the waveform or FFT Plot
    //Plot the FFT Plot
     while(editingDisplayData){ //Wait if the Display Buffer is beight used by processing task
       Serial.println("DV Waiting for DispBuffer");
     } 
     editingDisplayData = true;
     PlotFFTBarGraph(tft, FFTPLOT_DisplayData, FFTPLOT_CHANNEL, MajorFreq, frate, PlotColor);
     editingDisplayData = false;  
  }
  else{
    //Plot the sampled data on the TFT screen (if want to see the waveform)
    PlotSampledData(tft, AnalogValue_re, SignalAverage, frate, PlotColor);
  
    //Print the sampled data to the serial port
    if(WAVEFORM_DEBUG){
      PrintSampledData(Serial, AnalogValue_re);
    }
  }

  //Get next color every X frame only for rainbow
  if(Rainbow){
      if((frame - FFTPLOT_Color.GetFrame()) > ColorChangeThreshold){
      FFTPLOT_Color.UpdateRainbow();
      FFTPLOT_Color.SetFrame(frame);
    }
  }
    
  //Now add the delay to keep the fps stable
    timee = micros() - timee;                 //Calculate the time taken to plot the data.
    int16_t Delay = ceil(1/(FPSdesired*1.0)*1000 - timee/1000.0);    //adaptive delay
    if(Delay < 0){
      Delay = FPSDelay; //Worst case scenario.
    }
  delay(Delay);
}
}
