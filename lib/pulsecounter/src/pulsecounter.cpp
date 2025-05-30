
#include "pulsecounter.hpp"
#include <cstdint>
#include <string.h>
#include <iostream>
#include <esp_pthread.h>

 struct OutputData{
   uint16_t currentCount;
   uint16_t maxCount;
   imask_t previousInputMask;
   imask_t currentInputMask;
   omask_t outputMask;
 };

// 20ms wait time before reading inputs again
// Will be lowered in unit tests
static u_int16_t waitTimeInMillis = 20;

// ULP data =============
static bool inputsHaveRisingEdges;
static OutputData outputData[8];

// local static data =====================
static PulseCounterType pulseCounters[128];
static std::thread *readInputThread= NULL;

 bool Pulsecounter::inputHasRisingEdge( PulseCounterType& pulseCounter){
    if(pulseCounter.name[0]== 0)
      return false;
    imask_t pMask = outputData[pulseCounter.numOutPort].previousInputMask & (1 << pulseCounter.numInputPort);
    imask_t cMask = outputData[pulseCounter.numOutPort].currentInputMask & (1 << pulseCounter.numInputPort);
    return cMask & ( ~pMask) >0; 
 }
 bool Pulsecounter::readInputsRisingEdge( ){
     inputsHaveRisingEdges = false;
     omask_t currentMask = readOutputPorts();
        
     for( int a =0; a < sizeof( outputData)/sizeof( outputData[0]);a++)
     {
       OutputData &odata = outputData[a];
       if( odata.currentCount == 0 && odata.maxCount>0){
            // Set bit for outPin 1-16
           omask_t outputPinMask = currentMask | odata.outputMask;
           writeOutputs( outputPinMask);
           odata.currentInputMask = readInputPorts();
           imask_t inputMask = odata.currentInputMask & ( ~odata.previousInputMask);
           if( inputMask > 0 )
               inputsHaveRisingEdges = true;      
       }
     }
     writeOutputs( currentMask);
             
     return inputsHaveRisingEdges;
 }

 void Pulsecounter::readPorts(OutputConfigurationType type){
    for( int a=0; a < numOutPorts;a++)
        if( outputData[a].currentCount == 0){
            omask_t currentMask = readOutputPorts();
            // Set bit for outPin 1-16
            omask_t pinMask = currentMask | (1 << a);
            writeOutputs( pinMask);
            // Read bits from input
            outputData[a].previousInputMask = outputData[a].currentInputMask;
            outputData[a].currentInputMask = readInputPorts();
            writeOutputs(currentMask);
        }
 }
   
 void Pulsecounter::setOutputConfiguration(uint8_t port, OutputConfiguration config){
   if( config.type == EMeterType)
      outputData[port].maxCount = 1;
   else
      outputData[port].maxCount = 1234;
     // Ceiling(Watermeter 2.5[m3/min?]* 60000[ms/min] / 1000[pulses/m3] / 30[ms] )
  }
 void Pulsecounter::setPulseCounter( uint8_t outputPort, uint8_t inputPort, const char *name){
   strncpy(pulseCounters[outputPort].name, name,sizeof(pulseCounters[outputPort].name));
   pulseCounters[outputPort].numInputPort = inputPort;
   pulseCounters[outputPort].numOutPort = outputPort;
 }
void Pulsecounter::countPulses(){
   for( int a; a < sizeof(pulseCounters)/sizeof(pulseCounters[0]);a++){
            PulseCounterType &pc = pulseCounters[a];
               if(Pulsecounter::inputHasRisingEdge(pc))
                  pc.counter++;
            }
}
void readInput()
{
   while(readInputThread != NULL){
      if( Pulsecounter::readInputsRisingEdge())
         Pulsecounter::countPulses();
      for( int a; a < sizeof(outputData)/sizeof(outputData[0]);a++){
         if(outputData[a].currentCount)
            outputData[a].currentCount--;
         else
            outputData[a].currentCount = outputData[a].maxCount;
      }
      // S0 interface requires less than 30ms .
      // 20ms makes shure to recognizes it
      std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeInMillis));
   }
}
void  startThread(){
#ifndef NATIVE_TEST
    auto cfg = esp_pthread_get_default_config();
    esp_pthread_set_cfg(&cfg);
#endif
   assert(readInputThread == NULL);
   readInputThread = new std::thread( readInput);
}
 void Pulsecounter::init(){
   for(int a=0; a < sizeof(outputData)/sizeof(outputData[0]);a++){
      outputData[a].maxCount = 0;
      outputData[a].currentCount = 0;
      outputData[a].previousInputMask = 0;
      outputData[a].currentInputMask = 0;
      outputData[a].outputMask  = 1 << a;
   } 
   for(int a=0; a < sizeof(pulseCounters)/sizeof(pulseCounters[0]);a++){
      pulseCounters[a].name[0]= 0;
      pulseCounters[a].numOutPort = a;
      pulseCounters[a].counter = 0;
   }
  startThread();
 }

 void Pulsecounter::stopThread(){
   std::thread *t = readInputThread;
   //This will stop the thread gracefully
   readInputThread = NULL;
   // This will synchronize with stopped thread
   if(t){
      (*t).join();
      delete t;
    }
  }
