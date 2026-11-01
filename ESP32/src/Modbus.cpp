#include "Modbus.h"
#include <Arduino.h>
#include "Main.h"

// CRC-16-Modbus lookup table (polynomial 0xA001 reversed)
static const uint16_t g_crc16Table_au16[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

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
  if (block_i32 > 2)
  {
    block_i32 = 2;
  }

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
  if (block_i32 > 2)
  {
    block_i32 = 2;
  }

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
    txBuffer_au8[0] = (uint8_t)slaveId_i32;
    txBuffer_au8[1] = (uint8_t)functionCode_i32;
    txBuffer_au8[2] = (registerAddress_i32 >> 8) & 0xFF;
    txBuffer_au8[3] = registerAddress_i32 & 0xFF;
    txBuffer_au8[4] = (numberOfRegisters_i32 >> 8) & 0xFF;
    txBuffer_au8[5] = numberOfRegisters_i32 & 0xFF;
    int32_t crc_i32 = computeCrc(txBuffer_au8, 6);
    txBuffer_au8[6] = crc_i32 & 0xFF;
    txBuffer_au8[7] = (crc_i32 >> 8) & 0xFF;
 
    while(this->serial_pHS->available()) {
        this->serial_pHS->read();
    }

    this->serial_pHS->write(txBuffer_au8, 8);
    this->serial_pHS->flush();

    uint32_t startTime_u32 = millis();
    rawRxBufferLength_i32   = 0;
    dataRxBufferLength_i32 = 0;
    int32_t echoMatchCount_i32 = 0;
    int32_t receivedByte_i32;
    uint8_t receiveState_u8 = 0;

    bool allDataReceived_b = false;
    while( (false == allDataReceived_b) && ((millis() - startTime_u32) < timeout_u32))
    {
       while(this->serial_pHS->available())
       {
            receivedByte_i32 = this->serial_pHS->read();

            if(receiveState_u8 == 0)
            {
              if (txBuffer_au8[echoMatchCount_i32] == receivedByte_i32)
              {
                echoMatchCount_i32++;
              }
              else
              {
                echoMatchCount_i32 = 0;
              }
              if(echoMatchCount_i32 == 2)
              { 
                receiveState_u8 = 1; 
              }
            }
            else if(receiveState_u8 == 1)
            {
             rawRxBuffer_au8[0] = txBuffer_au8[0];
             rawRxBuffer_au8[1] = txBuffer_au8[1];
             rawRxBuffer_au8[2] = (uint8_t)receivedByte_i32;
             rawRxBufferLength_i32 = 3;
             receiveState_u8 = 2;
            } 
            else if(receiveState_u8 == 2)
            {
             this->rawRxBuffer_au8[rawRxBufferLength_i32++] = (uint8_t)receivedByte_i32;

             if(rawRxBufferLength_i32 >= rawRxBuffer_au8[2] + 5)
             { 
                allDataReceived_b = true;
                break; 
              }
            }
       }
       if (allDataReceived_b) break;
       delay(1);
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

int32_t Modbus::computeCrc(const uint8_t *buffer_pu8, int32_t bufferLength_i32)
{
    uint16_t crc_u16 = 0xFFFF;
    for (int32_t pos_i32 = 0; pos_i32 < bufferLength_i32; pos_i32++)
    {
        uint8_t tableIndex_u8 = (crc_u16 ^ buffer_pu8[pos_i32]) & 0xFF;
        crc_u16 = (crc_u16 >> 8) ^ g_crc16Table_au16[tableIndex_u8];
    }
    return (int32_t)crc_u16;
}

int32_t Modbus::writeHoldingRegisterToDevice(int32_t registerAddress_i32, uint16_t value_u16)
{
    return writeHoldingRegisterToDevice(slaveId_i32, registerAddress_i32, value_u16);
}
    
int32_t Modbus::writeHoldingRegisterToDevice(int32_t slaveId_i32, int32_t registerAddress_i32, uint16_t value_u16)
{
    txBuffer_au8[0] = (uint8_t)slaveId_i32;
    txBuffer_au8[1] = WRITE_HOLDING_REGISTER_U8;
    txBuffer_au8[2] = (registerAddress_i32 >> 8) & 0xFF;
    txBuffer_au8[3] = registerAddress_i32 & 0xFF;
    txBuffer_au8[4] = (value_u16 >> 8) & 0xFF;
    txBuffer_au8[5] = value_u16 & 0xFF;
    int32_t crc_i32 = computeCrc(txBuffer_au8, 6);
    txBuffer_au8[6] = crc_i32 & 0xFF;
    txBuffer_au8[7] = (crc_i32 >> 8) & 0xFF;
	
    while(this->serial_pHS->available()) {
        this->serial_pHS->read();
    }

    this->serial_pHS->write(txBuffer_au8, 8);
    this->serial_pHS->flush();

    uint32_t startTime_u32 = millis();
    int32_t echoMatchCount_i32 = 0;
    int32_t receivedByte_i32;
  
    bool responseReceived_b = false;
    while( ( (millis() - startTime_u32) < timeout_u32)  && (false == responseReceived_b))
    {
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
        if (responseReceived_b) break;
        delay(1);
    }

    return responseReceived_b;
}

int32_t Modbus::writeHoldingRegistersToDevice(int32_t slaveId_i32, int32_t registerAddress_i32, uint16_t* values_u16, uint8_t count_u8)
{
    uint8_t localTxBuffer[32]; // Max 10 registers supported
    localTxBuffer[0] = (uint8_t)slaveId_i32;
    localTxBuffer[1] = 0x10; // FC16 Preset Multiple Registers
    localTxBuffer[2] = (registerAddress_i32 >> 8) & 0xFF;
    localTxBuffer[3] = registerAddress_i32 & 0xFF;
    localTxBuffer[4] = (count_u8 >> 8) & 0xFF;
    localTxBuffer[5] = count_u8 & 0xFF;
    localTxBuffer[6] = count_u8 * 2;
    
    for (uint8_t i = 0; i < count_u8; i++) {
        localTxBuffer[7 + i*2] = (values_u16[i] >> 8) & 0xFF;
        localTxBuffer[8 + i*2] = values_u16[i] & 0xFF;
    }
    
    uint8_t length = 7 + count_u8 * 2;
    int32_t crc_i32 = computeCrc(localTxBuffer, length);
    localTxBuffer[length] = crc_i32 & 0xFF;
    localTxBuffer[length+1] = (crc_i32 >> 8) & 0xFF;
    
    // 1. Flush RX buffer to remove any garbage before transmitting
    while(this->serial_pHS->available()) {
        this->serial_pHS->read();
    }

    // 2. Transmit the packet directly into hardware UART TX FIFO
    this->serial_pHS->write(localTxBuffer, length + 2);
    this->serial_pHS->flush();

    // 3. Read exact 8-byte response and verify CRC
    uint32_t startTime_u32 = millis();
    uint8_t rxBuffer[8];
    uint8_t rxCount = 0;
    
    bool responseReceived_b = false;
    while( ( (millis() - startTime_u32) < timeout_u32)  && (false == responseReceived_b))
    {
        while(this->serial_pHS->available())
        {
            rxBuffer[rxCount++] = (uint8_t)this->serial_pHS->read();
            
            // Modbus FC16 Exception Response is exactly 5 bytes long (SlaveID, 0x90, ExceptionCode, CRC_L, CRC_H)
            if (rxCount == 5 && rxBuffer[1] == 0x90) {
                // Exception detected, abort waiting for 8 bytes to avoid 100ms timeout penalty!
                responseReceived_b = false; 
                break;
            }
            
            if (rxCount == 8) {
                int32_t receivedCrc = ((uint16_t)rxBuffer[7] << 8) | rxBuffer[6];
                int32_t computedCrc = computeCrc(rxBuffer, 6);
                
                if (rxBuffer[0] == slaveId_i32 && rxBuffer[1] == 0x10 && receivedCrc == computedCrc) {
                    responseReceived_b = true;
                }
                break;
            }
        }
        if (responseReceived_b || rxCount == 8 || (rxCount == 5 && rxBuffer[1] == 0x90)) break; 
        delay(1);
    }
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

