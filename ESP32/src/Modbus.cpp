#include "Modbus.h"
#include <Arduino.h>
#include "Main.h"

// CRC-16-Modbus constants
const int32_t CRC_POLYNOMIAL_I32 = 0xA001;
const int32_t CRC_INITIAL_VALUE_I32 = 0xFFFF;

Modbus::Modbus()
{
    this->serial_pHS = NULL;
}

Modbus::Modbus(HardwareSerial &serial_pHS)
{
    this->serial_pHS = &serial_pHS;
}

bool Modbus::initialize(bool enableLogging_b)
{
     this->logEnabled_b = enableLogging_b;
     return true;
}

void Modbus::setSerialTimeout(uint16_t timeout_u16)
{
  timeout_u32 = timeout_u16;
}

uint8_t Modbus::readByteFromRxBuffer(int32_t index_i32)
{
  return rawRxBuffer_au8[index_i32 + 3];
}

int32_t Modbus::readBlockFromRxBuffer(int32_t index_i32)
{
   return  (((uint16_t)dataRxBuffer_au8[index_i32 * 2] << 8) | dataRxBuffer_au8[index_i32 * 2 + 1]);
}

int32_t Modbus::readCoilFromDevice(int32_t registerAddress_i32)
{
    return readCoilFromDevice(slaveId_i32, registerAddress_i32);
}

int32_t Modbus::readCoilFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32)
{
   if(sendRequestAndReceiveResponse(slaveId_i32, COIL_REGISTER_U8, registerAddress_i32, 1))
   {
    uint8_t x_u8 = readByteFromRxBuffer(0);
    return bitRead(x_u8, 0);
   }
   else
   {
    return -1;
   }
}

int32_t Modbus::readDiscreteInputFromDevice(int32_t registerAddress_i32)
{
   return readDiscreteInputFromDevice(slaveId_i32, registerAddress_i32);
}

int32_t Modbus::readDiscreteInputFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32)
{
   if(sendRequestAndReceiveResponse(slaveId_i32, DISCRET_REGISTER_U8, registerAddress_i32, 1))
   {
    uint8_t x_u8 = readByteFromRxBuffer(0);
    return bitRead(x_u8, 0);
   }
   else
   {
    return -1;
   }
}

void Modbus::readDeviceParameter(uint16_t slaveId_u16, uint16_t parameterAddress_u16)
{
  uint8_t rawBuffer_au8[2];
  uint8_t length_u8;
  int16_t registerArray_ai16[4];
  registerArray_ai16[0] = -1;
  registerArray_ai16[1] = -1;
  registerArray_ai16[2] = -1;
  registerArray_ai16[3] = -1;

  if(sendRequestAndReceiveResponse(slaveId_u16, 0x03, parameterAddress_u16,  2) > 0)
  {
    getRawRxBuffer(rawBuffer_au8, length_u8);
    registerArray_ai16[0] = convertRxBufferToInt16(0);
  }
  
  int16_t returnValue_i16 = registerArray_ai16[0];

  if (logEnabled_b)
  {
    ActiveSerial->print("Parameter address: ");
    ActiveSerial->print(parameterAddress_u16);
    ActiveSerial->print(",    actual:");
    ActiveSerial->println(returnValue_i16);
  }

  delay(50);
}

bool Modbus::writeAndVerifyDeviceParameter(uint16_t slaveId_u16, int16_t parameterAddress_i16, int32_t value_i32)
{
  bool registerWritten_b = false;
  bool registerValueAsTarget_b = false;

  for (uint8_t tryIndex_u8 = 0; tryIndex_u8 < 10; tryIndex_u8++)
  {
    if (true == registerValueAsTarget_b)
    {
      break;
    }

    delay(10);

    uint8_t rawBuffer_au8[2];
    uint8_t length_u8;
    int16_t registerArray_ai16[4];
    registerArray_ai16[0] = -1;
    registerArray_ai16[1] = -1;
    registerArray_ai16[2] = -1;
    registerArray_ai16[3] = -1;
    
    if(sendRequestAndReceiveResponse(slaveId_u16, 0x03, parameterAddress_i16,  2) > 0)
    {
      getRawRxBuffer(rawBuffer_au8, length_u8);
      registerArray_ai16[0] = convertRxBufferToInt16(0);
    }
    
    int16_t returnValue_i16 = registerArray_ai16[0];

    int32_t targetValue_i32 = value_i32;

    if(returnValue_i16 != targetValue_i32)
    {
      delay(30);
      if (logEnabled_b)
      {
        ActiveSerial->print("Parameter adresse: ");
        ActiveSerial->print(parameterAddress_i16);
        ActiveSerial->print(",    actual: ");
        ActiveSerial->print(returnValue_i16);
        ActiveSerial->print(",    target: ");
        ActiveSerial->println(targetValue_i32);
      }

      writeHoldingRegisterToDevice(slaveId_u16, parameterAddress_i16, targetValue_i32); 

      registerWritten_b = true;
    }
    else
    {
      registerValueAsTarget_b = true;
    }
  }

  return registerWritten_b;
}

int32_t Modbus::readHoldingRegisterFromDevice(int32_t registerAddress_i32)
{
  return readHoldingRegisterFromDevice(slaveId_i32, registerAddress_i32, 1);
}

int32_t Modbus::readHoldingRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32, int32_t block_i32)
{
  if(block_i32 > 2){block_i32 = 2;}
  if(sendRequestAndReceiveResponse(slaveId_i32, HOLDING_REGISTER_U8, registerAddress_i32, block_i32))
  {
    if(block_i32 == 2)
    {
      uint32_t high_u32 = (uint32_t)readBlockFromRxBuffer(0);
      uint32_t low_u32 = (uint32_t)readBlockFromRxBuffer(1);
      return (int32_t)((high_u32 << 16) | low_u32);
    }
    else
    {
      return readBlockFromRxBuffer(0);
    }
  }
  else
  {
    return -1;
  }
}

int32_t Modbus::readInputRegisterFromDevice(int32_t registerAddress_i32)
{
   return readInputRegisterFromDevice(slaveId_i32, registerAddress_i32, 1);
}

int32_t Modbus::readInputRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32, int32_t block_i32)
{
  if(block_i32 > 2){block_i32 = 2;}
  if(sendRequestAndReceiveResponse(slaveId_i32, INPUT_REGISTER_U8, registerAddress_i32, block_i32))
  {
    if(block_i32 == 2)
    {
      uint32_t high_u32 = (uint32_t)readBlockFromRxBuffer(0);
      uint32_t low_u32 = (uint32_t)readBlockFromRxBuffer(1);
      return (int32_t)((high_u32 << 16) | low_u32);
    }
    else
    {
      return readBlockFromRxBuffer(0);
    }
  }
  else
  {
    return -1;
  }
}

int32_t Modbus::sendRequestAndReceiveResponse(int32_t slaveId_i32, int32_t functionCode_i32, int32_t registerAddress_i32, int32_t numberOfRegisters_i32)
{
    int32_t crc_i32;
    txBuffer_au8[0] = slaveId_i32;
    txBuffer_au8[1] = functionCode_i32;
    txBuffer_au8[2] = registerAddress_i32 >> 8;
    txBuffer_au8[3] = registerAddress_i32;
    txBuffer_au8[4] = numberOfRegisters_i32 >> 8;
    txBuffer_au8[5] = numberOfRegisters_i32;
    crc_i32 = this->computeCrc(txBuffer_au8, 6);
    txBuffer_au8[6] = crc_i32;
    txBuffer_au8[7] = crc_i32 >> 8;
 
    size_t bufferCapacity_st = this->serial_pHS->availableForWrite();
    this->serial_pHS->write(txBuffer_au8, 8);
    delay(1);
    uint8_t timeoutCounter_u8 = 10;
    while( (bufferCapacity_st != this->serial_pHS->availableForWrite() ) && (timeoutCounter_u8 > 0) ) 
    { 
      delay(1);
      timeoutCounter_u8--;
    }

    uint32_t startTime_u32 = millis();
    rawRxBufferLength_i32   = 0;
    dataRxBufferLength_i32 = 0;
    int32_t echoMatchCount_i32 = 0;
    int32_t receivedByte_i32;
    uint8_t receiveState_u8 = 0;

    bool allDataReceived_b = false;
    while( (false == allDataReceived_b) && ((millis() - startTime_u32) < timeout_u32))
    {
       delay(1);
       
       while(this->serial_pHS->available())
       {
            startTime_u32 = millis();
            receivedByte_i32 = this->serial_pHS->read();

            if(receiveState_u8 == 0)
            {
              if(txBuffer_au8[echoMatchCount_i32] == receivedByte_i32){ echoMatchCount_i32++; } else { echoMatchCount_i32 = 0; }
              if(echoMatchCount_i32 == 2)
              { 
                receiveState_u8 = 1; 
              }
            }
            else if(receiveState_u8 == 1)
            {
             rawRxBuffer_au8[0] = txBuffer_au8[0];
             rawRxBuffer_au8[1] = txBuffer_au8[1];
             rawRxBuffer_au8[2] = receivedByte_i32;
             rawRxBufferLength_i32 = 3;
             receiveState_u8 = 2;
            } 
            else if(receiveState_u8 == 2)
            {
             this->rawRxBuffer_au8[rawRxBufferLength_i32++] =  receivedByte_i32;

             if(rawRxBufferLength_i32 >= rawRxBuffer_au8[2] + 5)
             { 
                allDataReceived_b = true;
                break; 
              }
            }
       }
    }

    if(rawRxBufferLength_i32 > 2)
    {
        int32_t receivedCrc_i32 = ((uint16_t)rawRxBuffer_au8[rawRxBufferLength_i32 - 1] << 8) | rawRxBuffer_au8[rawRxBufferLength_i32 - 2];
        int32_t computedCrc_i32 = computeCrc(rawRxBuffer_au8, rawRxBufferLength_i32 - 2);

        if(receivedCrc_i32 == computedCrc_i32)
        {
            dataRxBufferLength_i32 = rawRxBuffer_au8[2];
            return dataRxBufferLength_i32;
        }
        else
        { 
            return -1; 
        }
    }
    else
    {
        return -1;
    }
}

int32_t Modbus::readCoilRegisterFromDevice(int32_t registerAddress_i32)
{
    return readCoilRegisterFromDevice(1,  registerAddress_i32, 1);
}

int32_t Modbus::readCoilRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32)
{
    return readCoilRegisterFromDevice(slaveId_i32, registerAddress_i32, 1);
}

int32_t Modbus::readCoilRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32, int32_t numberOfBits_i32)
{
   if(sendRequestAndReceiveResponse(slaveId_i32, COIL_REGISTER_U8, registerAddress_i32, numberOfBits_i32))
   {
    return readByteFromRxBuffer(0);
   }
   else
   {
    return -1;
   }
}

int32_t Modbus::readDiscreteRegisterFromDevice(int32_t registerAddress_i32)
{
    return readDiscreteRegisterFromDevice(1, registerAddress_i32, 1);
}

int32_t Modbus::readDiscreteRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32)
{
    return readDiscreteRegisterFromDevice(slaveId_i32, registerAddress_i32, 1);
}

int32_t Modbus::readDiscreteRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32, int32_t numberOfBits_i32)
{
    if(sendRequestAndReceiveResponse(slaveId_i32, DISCRET_REGISTER_U8, registerAddress_i32, numberOfBits_i32))
    {
        return readByteFromRxBuffer(0);
    }
    else
    {
        return -1;
    }
}

int32_t Modbus::readHoldingRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32)
{
    return 0; // Not implemented
}

int32_t Modbus::readInputRegisterFromDevice(int32_t slaveId_i32, int32_t registerAddress_i32)
{
    return 0; // Not implemented
}

int16_t Modbus::convertRxBufferToInt16(int32_t index_i32)
{
    int32_t address_i32 = (index_i32 * 2) + 3;
    return (int16_t)((uint16_t)rawRxBuffer_au8[address_i32] << 8 | rawRxBuffer_au8[address_i32+1]);
}

void Modbus::getRawRxBuffer(uint8_t *rawBuffer_pu8, uint8_t &rawBufferLength_u8)
{
   for(int32_t i_i32 = 0; i_i32 < rawRxBufferLength_i32; i_i32++)
    {
      rawBuffer_pu8[i_i32] = rawRxBuffer_au8[i_i32];
    }
     rawBufferLength_u8 = this->rawRxBufferLength_i32;
}

void Modbus::getRawTxBuffer(uint8_t *rawBuffer_pu8, uint8_t &rawBufferLength_u8)
{
   for(int32_t i_i32 = 0; i_i32 < 8; i_i32++)
    {
      rawBuffer_pu8[i_i32] = txBuffer_au8[i_i32];
    }
     rawBufferLength_u8 = 8;
}

int32_t Modbus::computeCrc(uint8_t *buffer_pu8, int32_t bufferLength_i32)
{
  int32_t crc_i32 = CRC_INITIAL_VALUE_I32;
  uint8_t pos_u8, i_u8;
 
  for (pos_u8 = 0; pos_u8 < bufferLength_i32; pos_u8++)
  {
    crc_i32 ^= (uint32_t)buffer_pu8[pos_u8];
 
    for (i_u8 = 8; i_u8 != 0; i_u8--)
    {
      if ((crc_i32 & 0x0001) != 0)
      {
        crc_i32 >>= 1;
        crc_i32 ^= CRC_POLYNOMIAL_I32;
      }
      else
      {
        crc_i32 >>= 1;
      }
    }
  }
  return crc_i32;  
}

int32_t Modbus::writeHoldingRegisterToDevice(int32_t registerAddress_i32, uint16_t value_u16)
{
    return writeHoldingRegisterToDevice(slaveId_i32, registerAddress_i32, value_u16);
}
    
int32_t Modbus::writeHoldingRegisterToDevice(int32_t slaveId_i32, int32_t registerAddress_i32, uint16_t value_u16)
{
    int32_t crc_i32;
	
    txBuffer_au8[0] = slaveId_i32;
    txBuffer_au8[1] = WRITE_HOLDING_REGISTER_U8;
    txBuffer_au8[2] = registerAddress_i32 >> 8;
    txBuffer_au8[3] = registerAddress_i32;
    txBuffer_au8[4] = value_u16 >> 8;
    txBuffer_au8[5] = value_u16;
    crc_i32 = this->computeCrc(txBuffer_au8, 6);
    txBuffer_au8[6] = crc_i32;
    txBuffer_au8[7] = crc_i32 >> 8;
	
  size_t bufferCapacity_st = this->serial_pHS->availableForWrite();
  delay(1);
  this->serial_pHS->write(txBuffer_au8, 8);
  delay(1);
  uint8_t timeOutCounter_u8 = 10;
  while( (bufferCapacity_st != this->serial_pHS->availableForWrite() ) && (timeOutCounter_u8 > 0) ) 
  { 
    delay(1);
    timeOutCounter_u8--;
  }

  delay(1);

  uint32_t startTime_u32 = millis();
  int32_t echoMatchCount_i32 = 0;
  int32_t receivedByte_i32;
  
  bool responseReceived_b = false;
  while( ( (millis() - startTime_u32) < timeout_u32)  && (false == responseReceived_b))
  {
      delay(1);
      startTime_u32 = millis();
      while(this->serial_pHS->available())
      {
        receivedByte_i32 = this->serial_pHS->read();
        if(txBuffer_au8[echoMatchCount_i32] == receivedByte_i32)
        {
            echoMatchCount_i32++;
        }
        else
        {
            echoMatchCount_i32 = 0;
        }

        if (echoMatchCount_i32 == 8)
        {
          responseReceived_b = true;
          break;
        }
      }
  }

  delay(5);

  return responseReceived_b;
}

int32_t Modbus::writeCoilToDevice(int32_t registerAddress_i32, uint8_t value_u8)
{
    // Not implemented
    return -1;
}

int32_t Modbus::writeCoilToDevice(int32_t slaveId_i32, int32_t registerAddress_i32, uint8_t value_u8)
{
    // Not implemented
    return -1;
}
