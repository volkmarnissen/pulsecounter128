
#include "pulsecounter.hpp"
#include <cstdint>
#include <string.h>
#include <iostream>

 static imask_t previousInputMasks[8]={0,0,0,0,0,0,0,0};
 static imask_t currentInputMasks[8]={0,0,0,0,0,0,0,0};
  
 static 
    OutputConfiguration outputConfigurations[8]={{NoType},{NoType},{NoType},{NoType},{NoType},{NoType},{NoType},{NoType}};

 bool Pulsecounter::inputHasChanged( uint16_t outPin, uint16_t inPin){
    imask_t pMask = previousInputMasks[outPin] & (1 << inPin);
    imask_t cMask = currentInputMasks[outPin] & (1 << inPin);
    const char *b = (outputConfigurations[outPin].type == EMeterType?"EMeter": "Water");
    return pMask != cMask;    
 }
 void Pulsecounter::setPreviousInputMaskForTest(imask_t *previousMask){
    memcpy(previousInputMasks, previousMask, sizeof(previousInputMasks));
 }
 void Pulsecounter::readPorts(OutputConfigurationType type){
    for( int a=0; a < numOutPorts;a++)
        if( outputConfigurations[a].type == type){
            omask_t currentMask = readOutputPorts();
            // Set bit for outPin 1-16
            omask_t pinMask = currentMask | (1 << a);
            writeOutputs( pinMask);
            // Read bits from input
            previousInputMasks[a] = currentInputMasks[a];
            currentInputMasks[a] = readInputPorts();
            writeOutputs(currentMask);
        }
 }
   
 void Pulsecounter::configureOutput(uint8_t port, OutputConfiguration config){
   outputConfigurations[port] = config;
 }   

