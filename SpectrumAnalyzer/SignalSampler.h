/*
    * SignalSampler.h
    *
    *  Created on: Aug 15, 2022
    * This header file stores the defines and funtion prototypes
    * required to comute the sampling of the signal and its processing. 
    *  
*/
#ifndef _SIGNALSAMPLER_H
#define _SIGNALSAMPLER_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <driver/i2s.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_err.h>
#include <esp_log.h>
#include <Math.h>
#include <stdio.h>
#include <Arduino.h>
#include "FFT.h"
//#include <arduinoFFT.h>

//DEFINES

//Constants to define the sampling frequency and the number of samples to be taken.
#define ReadFreq 11000
#define BUFFER_SIZE 1024
#define NumSeconds BUFFER_SIZE*(1.0/ReadFreq)
#define ReadDelayUs 1000000.0*(1.0/ReadFreq)
#define FFT_NOISE_THRESHOLD 4500
#define ADC_CHANNEL_USED ADC1_CHANNEL_6  //Formal name of Pin 34 (used for adc)


//Global variables
const int AnalogPin = 34;                             //Input signal is connected to GPIO 34 (Analog ADC1_CH6) 
const TickType_t xDelay = 3 / portTICK_PERIOD_MS;
//Function Definitions
double GetSampledData(float* AnalogValue_re);
void ADCSetup(Stream &Serial);
float ComputeFFT(fft_config_t *FFT, float *Magnitude);
void PrepareDisplayData(float *AnalogValue_re, int FreqS, int FreqE, int Channel, int SamplFreq, int SamplSize, uint32_t *DisplayData);
void PrintFFT(Stream &Serial, float *RealValue, int BUFFERSIZE);
uint32_t *InitializeDisplayArray(int Channel);
void ClearDisplayBuffer(uint32_t *Array, int Size);
#endif
