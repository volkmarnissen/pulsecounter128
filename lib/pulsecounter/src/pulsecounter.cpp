
#include "pulsecounter.hpp"
#include <cstdint>
#include <thread>
#include <chrono>
#include <string.h>
#include <iostream>
#include <cassert>
#include "pclog.hpp"

static const char *TAG = "pulsecounter";

// 20ms wait time before reading inputs again
// Will be lowered in unit tests
static u_int16_t waitTimeInMillis = 20;
#ifdef NATIVE
#define STATIC_ESP32
#else
#define STATIC_ESP32 static
#endif

// ULP data =============
STATIC_ESP32 OutputData outputData[8];
STATIC_ESP32 NoOutputData noOutputData;
static bool resetRequest = false;

// local static data =====================
STATIC_ESP32 PulseCounterType pulseCounters[maxPulseCounters];
STATIC_ESP32 int pulseCounterCount;
static std::thread *readInputThread = NULL;
static bool runReadInputThread = false;

bool Pulsecounter::inputHasRisingEdge(PulseCounterType &pulseCounter)
{
   if (pulseCounter.numInputPort == noInputPort)
      return false;
   imask_t pMask = noOutputData.previousInputMask & (1 << pulseCounter.numInputPort);
   imask_t cMask = noOutputData.currentInputMask & (1 << pulseCounter.numInputPort);

   if (pulseCounter.numOutPort != 0xFF)
   {
      ESP_LOGI(TAG, "OutputPort InputHasRisingEdge\n");
      pMask = outputData[pulseCounter.numOutPort].previousInputMask & (1 << pulseCounter.numInputPort);
      cMask = outputData[pulseCounter.numOutPort].currentInputMask & (1 << pulseCounter.numInputPort);
   }
   return (cMask & (~pMask)) > 0;
}

void Pulsecounter::setConfig(const Config &cfg)
{
   Pulsecounter::init();
   for (auto counter : cfg.getCounters())
      Pulsecounter::setPulseCounter(counter.getOutputPort(), counter.getInputPort());
   I2c *i2c = I2c::get();
   while (i2c == nullptr)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      i2c = I2c::get();
   }
   omask_t outputMask = i2c->readOutputPorts();
   omask_t initialOutputMask = outputMask;
   for (auto output : cfg.getOutputs())
   {
#ifndef NATIVE
      ESP_LOGI(TAG, "Output %d %d", output.getPort(), output.getConfiguration().type);
#endif
      Pulsecounter::setOutputConfiguration(output, cfg);
      if (output.getConfiguration().type == EMeterType || output.getConfiguration().type == WaterMeterType)
         outputMask &= ~(1 << output.getPort());
   }

   bool rc = i2c->writeOutputs(outputMask);
#ifndef NATIVE
   ESP_LOGI(TAG, "Set Outputmask from 0x%02x to 0x%02x %s", (unsigned)initialOutputMask, (unsigned)outputMask, rc ? "Successfully" : "Error!");
#endif

   // Clear EMeter outputPorts
   I2c::get()->writeOutputs(outputMask);

   if (readInputThread != NULL)
   {
      Pulsecounter::stopThread();
      Pulsecounter::joinThread();
      Pulsecounter::startThread();
   }
}
bool readInputsRisingEdgeOutputs()
{
   if (resetRequest)
   {
      for (int a = 0; a < sizeof(pulseCounters) / sizeof(pulseCounters[0]); a++)
         pulseCounters[a].counter = 0;

      resetRequest = false;
   }
   bool inputsHaveRisingEdges = false;
   bool atLeastOneOutput = false;
   for (int pc = 0; pc < pulseCounterCount && !atLeastOneOutput; pc++)
      if (pulseCounters[pc].numOutPort != 0xFF)
         atLeastOneOutput = true;
   omask_t currentMask = 0;
   if (atLeastOneOutput)
      currentMask = I2c::get()->readOutputPorts();

   for (int a = 0; a < sizeof(outputData) / sizeof(outputData[0]); a++)
   {
      OutputData &odata = outputData[a];
      bool hasCounter = false;
      for (int pc = 0; pc < pulseCounterCount && !hasCounter; pc++)
         if (pulseCounters[pc].numOutPort == a)
            hasCounter = true;
      if (hasCounter && odata.currentCount == 0 && odata.maxCount > 0)
      {
         // Set bit for outPin 1-16
         omask_t outputPinMask = (currentMask & ~odata.pcMask) | odata.onMask;
         I2c::get()->writeOutputs(outputPinMask);
         odata.previousInputMask = odata.currentInputMask;
         odata.currentInputMask = I2c::get()->readInputPorts();

         imask_t inputMask = odata.currentInputMask & (~odata.previousInputMask);
         if (inputMask > 0)
            inputsHaveRisingEdges = inputsHaveRisingEdges | true;
      }
   }
   if (atLeastOneOutput)
      I2c::get()->writeOutputs(currentMask);

   return inputsHaveRisingEdges;
}
bool readInputsRisingEdgeNoOutputs()
{
   bool inputsHaveRisingEdges = false;

   noOutputData.previousInputMask = noOutputData.currentInputMask;
   noOutputData.currentInputMask = I2c::get()->readInputPorts();
   imask_t inputMask = noOutputData.currentInputMask & (~noOutputData.previousInputMask);
   if (inputMask > 0)
      inputsHaveRisingEdges = inputsHaveRisingEdges | true;
   return inputsHaveRisingEdges;
}

bool Pulsecounter::readInputsRisingEdge()
{
   bool inptHaveRigingO = readInputsRisingEdgeOutputs();
   bool inptHaveRigingNoO = readInputsRisingEdgeNoOutputs();
   return inptHaveRigingO || inptHaveRigingNoO;
}

void Pulsecounter::readPorts(OutputConfigurationType type)
{
   for (int a = 0; a < numOutPorts; a++)
      if (outputData[a].currentCount == 0)
      {
         omask_t currentMask = I2c::get()->readOutputPorts();
         // Set bit for outPin 1-16
         omask_t pinMask = currentMask | (1 << a);
         I2c::get()->writeOutputs(pinMask);
         // Read bits from input
         outputData[a].previousInputMask = outputData[a].currentInputMask;
         outputData[a].currentInputMask = I2c::get()->readInputPorts();
         I2c::get()->writeOutputs(currentMask);
      }
}

void Pulsecounter::setOutputConfiguration(const OutputConfig &output, const Config &config)
{
   OutputData &odata = outputData[output.getPort()];
   odata.onMask = odata.offMask = odata.pcMask = 0;
   for (auto o : config.getOutputs())
   {
      switch (o.getConfiguration().type)
      {
      case EMeterType:
         odata.onMask |= (o.getPort() == output.getPort() ? 0 : 1) << o.getPort();
         odata.offMask |= (o.getPort() == output.getPort() ? 1 : 0) << o.getPort();
         odata.pcMask |= 1 << o.getPort();
         break;
      case WaterMeterType:
         odata.onMask |= (o.getPort() == output.getPort() ? 1 : 0) << o.getPort();
         odata.offMask |= (o.getPort() == output.getPort() ? 0 : 1) << o.getPort();
         odata.pcMask |= 1 << o.getPort();
         break;
      case NoType:
         break;
      }
   }

   if (output.getConfiguration().type == EMeterType)
   {
      odata.maxCount = 1;
   }
   else
   {
      int maxDivider = 1;
      for (auto c : config.getCounters())
         if (c.getOutputPort() == output.getPort() && c.getDivider() > maxDivider)
            maxDivider = c.getDivider();
      // Ceiling(Watermeter 2.5[m3/h] * 1000[pulses/m3] / (3600*1000[ms/h]  / waitTimeInMillis[ms] / min(WaterMeter divider) )
      odata.maxCount = (uint16_t)(2.5 * maxDivider / (3600 * 1000 / waitTimeInMillis));
   };
}
void Pulsecounter::setPulseCounter(uint8_t outputPort, uint8_t inputPort)
{
   int found = -1;
   for (int a = 0; a < pulseCounterCount && found >= 0; a++)
      if (pulseCounters[a].numInputPort == inputPort && pulseCounters[a].numOutPort == outputPort)
         found = a;
   if (found < 0 && pulseCounterCount < sizeof(pulseCounters) / sizeof(pulseCounters[0]))
   {
      pulseCounters[pulseCounterCount].numInputPort = inputPort;
      pulseCounters[pulseCounterCount].numOutPort = outputPort;
      time_t now = time(NULL);
      pulseCounters[pulseCounterCount].lastSecond = now;
      found = pulseCounterCount++;
   }
   if (found >= 0)
      pulseCounters[found].counter = 0;
}

void Pulsecounter::countPulses()
{
   char header[255] = "I ";
   char header2[255] = "O ";
   char buf[255] = "C ";
   int inputPort = -1;
   int inputIdx = -1;
   imask_t imaskc = noOutputData.currentInputMask;
   imask_t imaskp = noOutputData.previousInputMask;
   static int lineCounter = 0;
   for (int a = 0; a < pulseCounterCount; a++)
   {
      PulseCounterType &pc = pulseCounters[a];
      if (Pulsecounter::inputHasRisingEdge(pc))
      {
         inputPort = pc.numInputPort;
         inputIdx = a;
         pc.counter++;
      }
      sprintf(header + strlen(header), "%3d ", (int)pc.numInputPort);
      sprintf(header2 + strlen(header2), "%3d ", (int)pc.numOutPort);
      sprintf(buf + strlen(buf), "%3d ", (int)pc.counter);
   }
   if (lineCounter++ % 10 == 0)
   {
      ESP_LOGI(TAG, "%s", header);
      ESP_LOGI(TAG, "%s", header2);
   }
   if (inputIdx == -1)
      ESP_LOGI(TAG, "%s No Pulsecounter configured for %4x %4x", buf, imaskp, imaskc);
   else
      ESP_LOGI(TAG, "%s %d/%d %4x %4x", buf, inputPort, inputIdx, imaskp, imaskc);
}

void Pulsecounter::reset()
{
   resetRequest = true;
}

std::string Pulsecounter::getStatusJson()
{
   std::string rc = "[";
   time_t now = time(NULL);
   char buf[128];
   bool cutComma = false;
   for (int a = 0; a < pulseCounterCount; a++)
      if (pulseCounters[a].numInputPort != noInputPort)
      {
         sprintf(buf, "{ \"input\": %d, \"output\":%d, \"last\": %d, \"secondsAgo\": %ld },",
                 (int)pulseCounters[a].numInputPort, (int)pulseCounters[a].numOutPort, (int)pulseCounters[a].counter,
                 now - pulseCounters[a].lastSecond);
         rc += buf;
         cutComma = true;
      }
   if (cutComma)
      rc = rc.substr(0, rc.length() - 1);
   rc += "]";
   return rc;
}
uint32_t Pulsecounter::getCounts(uint8_t outputPort, uint8_t inputPort)
{

   for (int a = 0; a < pulseCounterCount; a++)
      if (pulseCounters[a].numInputPort == inputPort && pulseCounters[a].numOutPort == outputPort)
      {
         // ESP_LOGI(TAG, "getCounts %d %lu\n", inputPort, (unsigned long)pulseCounters[a].counter);
         return pulseCounters[a].counter;
      }

   return 0;
}

void readInput()
{
   if (I2c::get() == nullptr)
   {
      loge(TAG, "Unable to initialize I2C! Terminating...");
      return;
   }

   while (runReadInputThread)
   {
      if (Pulsecounter::readInputsRisingEdge())
      {
         Pulsecounter::countPulses();
      }
      for (int a = 0; a < sizeof(outputData) / sizeof(outputData[0]); a++)
      {
         if (outputData[a].currentCount)
            outputData[a].currentCount--;
         else
            outputData[a].currentCount = outputData[a].maxCount;
      }
      // S0 interface requires less than 30ms .
      // 20ms makes shure to recognizes it
      std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeInMillis));
   }
   I2c::deleteInstance();
   ESP_LOGI(TAG, "Terminating Pulsecounter Thread\n");
}

#ifdef NATIVE
extern OutputData *Pulsecounter::getOutputData()
{
   return outputData;
}
extern NoOutputData *Pulsecounter::getNoOutputData()
{
   return &noOutputData;
}
#endif

#ifndef MOCK_PTHREAD
#include <esp_pthread.h>
#endif

void Pulsecounter::init()
{
   int numOutputs = sizeof(outputData) / sizeof(outputData[0]);
   for (int a = 0; a < numOutputs; a++)
   {
      outputData[a].maxCount = 0;
      outputData[a].currentCount = 0;
      outputData[a].previousInputMask = 0;
      outputData[a].currentInputMask = 0;
   }
   noOutputData.previousInputMask = 0;
   noOutputData.currentInputMask = 0;
   pulseCounterCount = 0;
   for (int a = 0; a < sizeof(pulseCounters) / sizeof(pulseCounters[0]); a++)
   {
      pulseCounters[a].numOutPort = a;
      pulseCounters[a].numInputPort = 0xFF;
      pulseCounters[a].counter = 0;
   }

#ifndef NATIVE
   omask_t currentMask = I2c::get()->readOutputPorts();
   ESP_LOGI(TAG, "Initial State of output ports 0x%x", (unsigned)currentMask);
#endif
}
void Pulsecounter::startThread()
{
#ifndef MOCK_PTHREAD
   auto cfg = esp_pthread_get_default_config();
   esp_pthread_set_cfg(&cfg);
#endif
   ESP_LOGI(TAG, "Starting Pulsecounter Thread\n");

   assert(readInputThread == NULL);
   runReadInputThread = true;
   readInputThread = new std::thread(readInput);
}

void Pulsecounter::stopThread()
{
   ESP_LOGI(TAG, "Stopping Pulsecounter Thread\n");
   runReadInputThread = false;
   // This will stop the thread gracefully
   readInputThread = NULL;
}

void Pulsecounter::joinThread()
{
   if (readInputThread)
   {
      (*readInputThread).join();
      delete readInputThread;
      readInputThread = NULL;
   }
}
#ifdef NATIVE
void setPulseCount(uint8_t outputPort, uint8_t inputPort, uint32_t count)
{
   for (int a = 0; a < sizeof(pulseCounters) / sizeof(pulseCounters[0]); a++)
      if (pulseCounters[a].numInputPort == inputPort && pulseCounters[a].numOutPort == outputPort)
         pulseCounters[a]
             .counter = count;
}
#endif