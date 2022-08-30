/*
*   SignaleSampler.cpp
*   Created on: Aug 15, 2022
*   Signal Sampler cpp file.
*   Holds functions needed to plot graph of the sampled data on the TFT screen. 
*/

#include "SignalSampler.h"

//Functions


/*
*   Function to get the sampled data.
*   Input: double* AnalogValue_re - Reference to the array to store the sampled data.
*   Return: Average of the sampled data.
*/
double GetSampledData(float* AnalogValue_re){
    unsigned long timee = micros();
    double avg = 0; 
    size_t bytes_read = 0;
    int16_t* buffer = (int16_t*)malloc(BUFFER_SIZE * sizeof(int16_t));
    if (buffer == NULL) {
        Serial.println("Failed to allocate memory");
        while(1);
    }
    //Read data from the ADC
    i2s_read(I2S_NUM_0, buffer, sizeof(int16_t)* BUFFER_SIZE, &bytes_read, portMAX_DELAY);
    i2s_adc_disable(I2S_NUM_0); //Disable ADC

    int samplesRead = bytes_read / sizeof(int16_t);
    
    if(1/*bytes_read != BUFFER_SIZE*/){ //Used to debug, will be removed later
        char Stringbuff[80];  
        sprintf(Stringbuff, "Read %d bytes out of %d bytes", samplesRead, BUFFER_SIZE);
        //Serial.println(Stringbuff);
    }
    
    //Now copy the data into the output data array
    for(int i = 0; i < BUFFER_SIZE; i++){
        buffer[i] = (int)ADC_CHANNEL_USED * 0x1000 + 0xFFF - buffer[i];     //Some Voodoo magic to get the correct value, I think it to convert the output format of i2s. Found online.
        AnalogValue_re[i] = 4096.0 - buffer[i];                             //The value needs to be substracted from 4096 to get the correct value. (i.e the one read from AnalogRead())
        avg += AnalogValue_re[i];
        //Serial.printf("Value %d: ", i);
        //Serial.println(4095-buffer[i]);
    }

    free(buffer);   //Free the memory allocated for the buffer
    avg /= BUFFER_SIZE;

    //Stuff to do with the time taken to sample the data will be deleted later
    timee = micros() - timee;
    //Serial.print("ReadTime: ");
    //Serial.println(timee);

    i2s_adc_enable(I2S_NUM_0);  //Enable the ADC again
    return avg; //return average value
}

/*
*   Function to initialize the ADC.
*   Initializes ADC and i2s DMA writing.
*   Input: Handle to the Serial object.
*   Output: None.
*/
void ADCSetup(Stream &Serial){
    esp_err_t err;

    Serial.println("Initializing ADC...");

    // Cofiguring the i2s driver for ADC
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = ReadFreq,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S_LSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    //Configure i2s driver
    err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.println("Failed installing i2s driver");
        while(1);
    }

    //inti ADC pad
    err = i2s_set_adc_mode(ADC_UNIT_1, ADC_CHANNEL_USED);
    if (err != ESP_OK) {
        Serial.println("Failed setting adc pad");
        while(1);
    }

    err = i2s_adc_enable(I2S_NUM_0);
    if (err != ESP_OK) {
        Serial.println("Failed enabling ADC");
        while(1);
    }

    Serial.println("ADC initialized");
}

/*
*   Function to compute the FFT of the sampled data.
*   Input: Pointer to FFT Config - to compute the FFT.
*   Input: Float array of predefined length (BufferSize/2 - 1) to store magnitude data
*   Output: Returns the frequency with maximum magnitude.
*/
float ComputeFFT(fft_config_t *FFT, float *Magnitude){
//    FFT.DCRemoval();
//    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
//    FFT.Compute(FFT_FORWARD);
//    FFT.ComplexToMagnitude();
    fft_execute(FFT);    //Do fft.

    //Serial.println("FFT Done");
    //Now get magnitude and Major Frequency
    float max_magnitude = 0.0;
    float major_freq = 0.0;
    for(int i = 1; i < FFT->size/2; i++){
      Magnitude[i-1] = sqrt(pow(FFT->output[2*i], 2) + pow(FFT->output[2*i+1],2))/1;
      float freq = i * 1/(BUFFER_SIZE*1.0/ReadFreq);
      if(Magnitude[i-1] > max_magnitude){
        max_magnitude = Magnitude[i-1];
        major_freq = freq;
      }
      FFT->output[i] = Magnitude[i - 1];
    }
    
    return major_freq;
}

/*
*   Function to print FFt data to the Serial object.
*   Input: Stream &Serial - Reference to the Serial object.
*   Input: double* ReaalValue - Reference to the array to store the FFT data.
*   Input: int BUFFERSIZE - Size of the array.
*   Output: None.
*/
void PrintFFT(Stream &Serial, float *RealValue, int BUFFERSIZE){
    Serial.println("----Printing FFT----");
    for(int i = 0; i < (BUFFERSIZE/2); i++){
        Serial.printf("%lf\n",RealValue[i]);
    }
    Serial.println("----FFT printed Finished-----");
}

/*
*   Function to prepare the data for the FFT plot.
*   Input: double* AnalogValue_re - Reference to the array that has the FFT data.
*   Input: int FreqS - The starting frequency for the plot to consider.
*   Input: int FreqE - The ending frequency for the plot to consider.
*   Input: int Channel - The channels in the plot, i.e the number of bars
*   Input: int SamplFreq - The sampling frequency of the data.
*   Input: int SamplSize - The size of the FFT sampling Data.
*   Input: uint32_t* DisplayData - Reference to the array to store the data for the plot. Assumed that the user calls InitializeDisplayArray(int Channel) to get this.
*   Output: None.
*/
void PrepareDisplayData(float *AnalogValue_re, int FreqS, int FreqE, int Channel, int SamplFreq, int SamplSize, uint32_t *DisplayData){
    //Firstly Clear Display Buffer
    ClearDisplayBuffer(DisplayData, Channel);
    
    //Variables needed to do the computation
    //Check how the data is stored after FFT computaition to understand Bins and BinSize
    int Bins = SamplSize/2;
    float BinSize = SamplFreq / SamplSize;
    int BinStart = ceil(FreqS / BinSize);   //The bin to start from, i.e which has the data from the starting frequency.
    int BinEnd = ceil(FreqE / BinSize);     //The bin to end at, i.e which has the data from the ending frequency.
    int BinCount = BinEnd - BinStart + 1;
    int BinIncrement = floor(BinCount/Channel);
    int DispChannel = 0;                    //Used to fill the display data array.
    bool Is_Float_Increnment = (BinCount % Channel == 0)? 0 : 1;    //Used to determine if the total bins and channel are completly dvivisible. Otherwise the last channel will have more bins than the others.
    
    for(int i = BinStart; i <= BinStart + Channel*BinIncrement; i+=BinIncrement){   //The loop is simple, start the the starting bin and fill the next 'n' bins in the DisplayData[Ch] array position. 
        if((DispChannel == Channel)&&(Is_Float_Increnment)){    //This part deals with unven number of Bins in case they are present  
            for(int j = i; j <= BinEnd; j++){
                DisplayData[DispChannel] += AnalogValue_re[j]/100 - FFT_NOISE_THRESHOLD; 
            }
        }
        for(int j = i; j <= i + BinIncrement - 1; j++){
            DisplayData[DispChannel] += AnalogValue_re[j];      //The filling takes place here.
        }
        DispChannel++;      //One one of the channel is filled, increment the channel.
    }
}

/*
*   Function to intialize the display array for FFT plot
*   Input: int Channel - Number of channels to be displayed. i.e no of bars in the plot.
*   Output: uint32_t *DisplayData - Reference to the array to store the data.
*/
uint32_t *InitializeDisplayArray(int Channel){
    uint32_t *DisplayData = (uint32_t *)malloc(Channel * sizeof(uint32_t));     //Pretty self explanatory
    for(int i = 0; i < Channel; i++){
        DisplayData[i] = 0;
    }
    return DisplayData;
}

void ClearDisplayBuffer(uint32_t *Array, int Size){
  for(int i = 0; i < Size; i++){
    Array[i] = 0;
  }
}
