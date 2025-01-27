
/****************************************************
  AMS 5600 class for Arduino platform
  Author: Tom Denton
  Date: 15 Dec 2014 
  File: AMS_5600.cpp
  Version 1.00
  www.ams.com
   
  Description:  This class has been designed to
  access the AMS 5600 “potuino” shield.
*****************************************************/

// updated jan 2022 by isc - read two bytes together

// datasheet: https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf

#include "Arduino.h"
#include "AS5600_softwire.h"
#include "SoftWire.h"

/****************************************************
  Method: AMS_5600
  In: none
  Out: none
  Description: constructor class for AMS 5600
*****************************************************/
AMS_5600_SOFTWIRE::AMS_5600_SOFTWIRE(uint8_t sdaPin, uint8_t sclPin) : sw(sdaPin, sclPin) {
    char swTxBuffer[16];
    char swRxBuffer[16];
    sw.setTxBuffer(swTxBuffer, sizeof(swTxBuffer));
    sw.setRxBuffer(swRxBuffer, sizeof(swRxBuffer));
    sw.setDelay_us(5);
    sw.setTimeout(1000);
    sw.begin();
}
/*******************************************************
  Method: setOutPut
  In: 0 for digital PWM
      1 for analog (full range 0-100% of GND to VDD)
      2 for analog (reduced range 10-90%)
  Out: none
  Description: sets output mode in CONF register.
*******************************************************/
void AMS_5600_SOFTWIRE::setOutPut(uint8_t mode)
{
  int _conf_lo = _addr_conf+1; // lower byte address
  uint8_t config_status = readOneByte(_conf_lo);
  config_status &= 0b11001111; // bits 5:4 = 00, default
  if (mode == 0) {
    config_status |= 0b100000; // bits 5:4 = 10
  } else if (mode == 2) {
    config_status |= 0b010000; // bits 5:4 = 01
  }
  writeOneByte(_conf_lo, config_status);
}

/****************************************************
  Method: AMS_5600
  In: none
  Out: i2c address of AMS 5600
  Description: returns i2c address of AMS 5600
****************************************************/
int AMS_5600_SOFTWIRE::getAddress()
{
  return _ams5600_Address;
}

/*******************************************************
  Method: setMaxAngle
  In: new maximum angle to set OR none
  Out: value of max angle register
  Description: sets a value in maximum angle register.
  If no value is provided, method will read position of
  magnet.  Setting this register zeros out max position
  register.
*******************************************************/
word AMS_5600_SOFTWIRE::setMaxAngle(word newMaxAngle)
{
  word _maxAngle;
  if (newMaxAngle == -1)
    _maxAngle = getRawAngle();
  else
    _maxAngle = newMaxAngle;

  writeOneByte(_addr_mang, highByte(_maxAngle));
  delay(2);
  writeOneByte(_addr_mang+1, lowByte(_maxAngle));
  delay(2);

  word retVal = readTwoBytesSeparately(_addr_mang);
  return retVal;
}

/*******************************************************
  Method: getMaxAngle
  In: none
  Out: value of max angle register
  Description: gets value of maximum angle register.
*******************************************************/
word AMS_5600_SOFTWIRE::getMaxAngle()
{
  return readTwoBytesSeparately(_addr_mang);
}

/*******************************************************
  Method: setStartPosition
  In: new start angle position
  Out: value of start position register
  Description: sets a value in start position register.
  If no value is provided, method will read position of
  magnet.  
*******************************************************/
word AMS_5600_SOFTWIRE::setStartPosition(word startAngle)
{
  word _rawStartAngle;
  if (startAngle == -1)
    _rawStartAngle = getRawAngle();
  else
    _rawStartAngle = startAngle;

  writeOneByte(_addr_zpos, highByte(_rawStartAngle));
  delay(2);
  writeOneByte(_addr_zpos+1, lowByte(_rawStartAngle));
  delay(2);
  word _zPosition = readTwoBytesSeparately(_addr_zpos);

  return (_zPosition);
}

/*******************************************************
  Method: getStartPosition
  In: none
  Out: value of start position register
  Description: gets value of start position register.
*******************************************************/
word AMS_5600_SOFTWIRE::getStartPosition()
{
  return readTwoBytesSeparately(_addr_zpos);
}

/*******************************************************
  Method: setEndPosition
  In: new end angle position
  Out: value of end position register
  Description: sets a value in end position register.
  If no value is provided, method will read position of
  magnet.  
*******************************************************/
word AMS_5600_SOFTWIRE::setEndPosition(word endAngle)
{
  word _rawEndAngle;
  if (endAngle == -1)
    _rawEndAngle = getRawAngle();
  else
    _rawEndAngle = endAngle;

  writeOneByte(_addr_mpos, highByte(_rawEndAngle));
  delay(2);
  writeOneByte(_addr_mpos+1, lowByte(_rawEndAngle));
  delay(2);
  word _mPosition = readTwoBytesSeparately(_addr_mpos);

  return (_mPosition);
}

/*******************************************************
  Method: getEndPosition
  In: none
  Out: value of end position register
  Description: gets value of end position register.
*******************************************************/
word AMS_5600_SOFTWIRE::getEndPosition()
{
  word retVal = readTwoBytesSeparately(_addr_mpos);
  return retVal;
}

/*******************************************************
  Method: getRawAngle
  In: none
  Out: value of raw angle register
  Description: gets raw value of magnet position.
  start, end, and max angle settings do not apply
*******************************************************/
word AMS_5600_SOFTWIRE::getRawAngle()
{
  return readTwoBytesTogether(_addr_raw_angle);
}

/*******************************************************
  Method: getScaledAngle
  In: none
  Out: value of scaled angle register
  Description: gets scaled value of magnet position.
  start, end, or max angle settings are used to 
  determine value
*******************************************************/
word AMS_5600_SOFTWIRE::getScaledAngle()
{
  return readTwoBytesTogether(_addr_angle);
}

/*******************************************************
  Method: detectMagnet
  In: none
  Out: 1 if magnet is detected, 0 if not
  Description: reads status register and examines the 
  MD bit.
*******************************************************/
int AMS_5600_SOFTWIRE::detectMagnet()
{
  // Status bits: 0 0 MD ML MH 0 0 0 
  // MD high = magnet detected  
  int magStatus = readOneByte(_addr_status);
  return (magStatus & 0x20) ? 1 : 0;
}

/*******************************************************
  Method: getMagnetStrength
  In: none
  Out: 0 if magnet not detected
       1 if magnet is too weak
       2 if magnet is just right
       3 if magnet is too strong
  Description: reads status register and examines the 
  MH,ML,MD bits.
*******************************************************/
int AMS_5600_SOFTWIRE::getMagnetStrength()
{
  int retVal = 0; // no magnet
  // Status bits: 0 0 MD ML MH 0 0 0 
  // MD high = magnet detected  
  // ML high = AGC maximum overflow, magnet too weak
  // MH high = AGC minimum overflow, magnet too strong
  int magStatus = readOneByte(_addr_status);
  if (magStatus & 0x20) {
    retVal = 2;   // magnet detected
    if (magStatus & 0x10)
      retVal = 1; // too weak
    else if (magStatus & 0x08)
      retVal = 3; // too strong
  }
  
  return retVal;
}

/*******************************************************
  Method: get Agc
  In: none
  Out: value of AGC register
  Description: gets value of AGC register.
*******************************************************/
int AMS_5600_SOFTWIRE::getAgc()
{
  return readOneByte(_addr_agc);
}

/*******************************************************
  Method: getMagnitude
  In: none
  Out: value of magnitude register
  Description: gets value of magnitude register.
*******************************************************/
word AMS_5600_SOFTWIRE::getMagnitude()
{
  return readTwoBytesTogether(_addr_magnitude);
}

/*******************************************************
  Method: getConf
  In: none
  Out: value of CONF register 
  Description: gets value of CONF register.
*******************************************************/
word AMS_5600_SOFTWIRE::getConf()
{
  return readTwoBytesSeparately(_addr_conf);
}

/*******************************************************
  Method: setConf
  In: value of CONF register
  Out: none
  Description: sets value of CONF register.
*******************************************************/
void AMS_5600_SOFTWIRE::setConf(word _conf)
{
  writeOneByte(_addr_conf, highByte(_conf));
  delay(2);
  writeOneByte(_addr_conf+1, lowByte(_conf));
  delay(2);
}

/*******************************************************
  Method: getBurnCount
  In: none
  Out: value of zmco register
  Description: determines how many times chip has been
  permanently written to. 
*******************************************************/
int AMS_5600_SOFTWIRE::getBurnCount()
{
  return readOneByte(_addr_zmco);
}

/*******************************************************
  Method: burnAngle
  In: none
  Out: 1 success
      -1 no magnet
      -2 burn limit exceeded
      -3 start and end positions not set (useless burn)
  Description: burns start and end positions to chip.
  THIS CAN ONLY BE DONE 3 TIMES
*******************************************************/
int AMS_5600_SOFTWIRE::burnAngle()
{
  word _zPosition = getStartPosition();
  word _mPosition = getEndPosition();
  word _maxAngle = getMaxAngle();

  int retVal = 1;
  if (detectMagnet() == 1) {
    if (getBurnCount() < 3) {
      if ((_zPosition == 0) && (_mPosition == 0))
        retVal = -3;
      else
        writeOneByte(_addr_burn, 0x80);
    }
    else
      retVal = -2;
  } else
    retVal = -1;

  return retVal;
}

/*******************************************************
  Method: burnMaxAngleAndConfig
  In: none
  Out: 1 success
      -1 burn limit exceeded
      -2 max angle is to small, must be at or above 18 degrees
  Description: burns max angle and config data to chip.
  THIS CAN ONLY BE DONE 1 TIME
*******************************************************/
int AMS_5600_SOFTWIRE::burnMaxAngleAndConfig()
{
  word _maxAngle = getMaxAngle();

  int retVal = 1;
  if (getBurnCount() == 0) {
    if (_maxAngle * 0.087 < 18)
      retVal = -2;
    else
      writeOneByte(_addr_burn, 0x40);
  }
  else
    retVal = -1;

  return retVal;
}

/*******************************************************
  Method: readOneByte
  In: register to read
  Out: data read from i2c
  Description: reads one byte register from i2c
*******************************************************/
int AMS_5600_SOFTWIRE::readOneByte(int in_adr)
{
  int retVal = -1;
  sw.beginTransmission(_ams5600_Address);
  sw.write(in_adr);
  sw.endTransmission();
  sw.requestFrom(_ams5600_Address, (uint8_t) 1);
  while (sw.available() == 0)
    ;
  retVal = sw.read();

  return retVal;
}

/*******************************************************
  Method: readTwoBytesTogether
  In: two registers to read
  Out: data read from i2c as a word
  Description: reads two bytes register from i2c
*******************************************************/
word AMS_5600_SOFTWIRE::readTwoBytesTogether(int addr_in)
{

  // use only for Angle, Raw Angle and Magnitude

  // read 2 bytes together to prevent getting inconsistent
  //    data while the encoder is moving
  // according to the datasheet the address is automatically incremented
  //    but only for Angle, Raw Angle and Magnitude
  // the title says it's auto, but the paragraph after it
  //    says it does NOT
  // tested and it does auto increment
  
  // PAGE 13: https://ams.com/documents/20143/36005/AS5600_DS000365_5-00.pdf
  // Automatic Increment of the Address Pointer for ANGLE, RAW ANGLE and MAGNITUDE Registers
  // These are special registers which suppress the automatic
  // increment of the address pointer on reads, so a re-read of these
  // registers requires no I²C write command to reload the address
  // pointer. This special treatment of the pointer is effective only if
  // the address pointer is set to the high byte of the register.

  /* Read 2 Bytes */
  sw.beginTransmission(_ams5600_Address);
  sw.write(addr_in);
  sw.endTransmission();
  sw.requestFrom(_ams5600_Address, (uint8_t) 2);
  while (sw.available() < 2)
    ;
  
  int highByte = sw.read();
  int lowByte  = sw.read();

  // in case newer version of IC used the same address to
  //    store something else, get only the 3 bits
  //return ( ( highByte & 0b111 ) << 8 ) | lowByte;

  // but in case newer version has higher resolution
  //    we're good to go
  return ( highByte << 8 ) | lowByte;
}

/*******************************************************
  Method: readTwoBytesSeparately
  In: two registers to read
  Out: data read from i2c as a word
  Description: reads two bytes register from i2c
*******************************************************/
word AMS_5600_SOFTWIRE::readTwoBytesSeparately(int addr_in)
{
  int highByte = readOneByte(addr_in  );
  int lowByte  = readOneByte(addr_in+1);
  return ( highByte << 8 ) | lowByte;
}

/*******************************************************
  Method: writeOneByte
  In: address and data to write
  Out: none
  Description: writes one byte to a i2c register
*******************************************************/
void AMS_5600_SOFTWIRE::writeOneByte(int adr_in, int dat_in)
{
  sw.beginTransmission(_ams5600_Address);
  sw.write(adr_in);
  sw.write(dat_in);
  sw.endTransmission();
}

/**********  END OF AMS 5600 CLASS *****************/
