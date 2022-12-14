/**************************************************************************//**
 * \brief EEPROM 24C01 / 24C02 library for Arduino
 * \author Copyright (C) 2012  Julien Le Sech - www.idreammicro.com
 * \version 1.0
 * \date 20120217
 *
 * This file is part of the EEPROM 24C01 / 24C02 library for Arduino.
 *
 * This library is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/
 ******************************************************************************/

/**************************************************************************//**
 * \file Eeprom24C01_02.cpp
 ******************************************************************************/

/******************************************************************************
 * Header file inclusions.
 ******************************************************************************/

#include <Arduino.h>
#include <Wire.h>

#include <Eeprom24C01_02.h>

/******************************************************************************
 * Private macro definitions.
 ******************************************************************************/

/**************************************************************************//**
 * \def EEPROM__PAGE_SIZE
 * \brief Size of a page in EEPROM memory.
 * This size is given by EEPROM memory datasheet.
 ******************************************************************************/
#define EEPROM__PAGE_SIZE         8

/**************************************************************************//**
 * \def EEPROM__RD_BUFFER_SIZE
 * \brief Size of input TWI buffer.
 * This size is equal to BUFFER_LENGTH defined in Wire library (32 bytes).
 ******************************************************************************/
#define I2C_BUFFER_LENGTH 32
#define EEPROM__RD_BUFFER_SIZE I2C_BUFFER_LENGTH

/**************************************************************************//**
 * \def EEPROM__WR_BUFFER_SIZE
 * \brief Size of output TWI buffer.
 * This size is equal to BUFFER_LENGTH - 1 byte reserved for address.
 ******************************************************************************/
#define EEPROM__WR_BUFFER_SIZE    (I2C_BUFFER_LENGTH - 1)

/******************************************************************************
 * Public method definitions.
 ******************************************************************************/

/**************************************************************************//**
 * \fn Eeprom24C01_02::Eeprom24C01_02(byte deviceAddress)
 *
 * \brief Constructor.
 *
 * \param   deviceAddress   EEPROM address on TWI bus.
 ******************************************************************************/
Eeprom24C01_02::Eeprom24C01_02
(
    byte deviceAddress
){
    m_deviceAddress = deviceAddress;
}

/**************************************************************************//**
 * \fn void Eeprom24C01_02::initialize()
 *
 * \brief Initialize library and TWI bus.
 *
 * If several devices are connected to TWI bus, this method mustn't be
 * called. TWI bus must be initialized out of this library using
 * Wire.begin() method.
 ******************************************************************************/
void
Eeprom24C01_02::initialize()
{
    Wire.begin();
}

/**************************************************************************//**
 * \fn void Eeprom24C01_02::writeByte(
 * byte address,
 * byte data)
 *
 * \brief Write a byte in EEPROM memory.
 *
 * \remarks A delay of 10 ms is required after write cycle.
 *
 * \param   address Address.
 * \param   data    Byte to write.
 ******************************************************************************/
void
Eeprom24C01_02::writeByte
(
    byte    address,
    byte    data
){
    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address);
    Wire.write(data);
    Wire.endTransmission();
}

/**************************************************************************//**
 * \fn void Eeprom24C01_02::writeBytes(
 * byte     address,
 * byte     length,
 * byte*    p_data)
 * 
 * \brief Write bytes in EEPROM memory.
 *
 * \param       address Start address.
 * \param       length  Number of bytes to write.
 * \param[in]   p_data  Bytes to write.
 ******************************************************************************/
void
Eeprom24C01_02::writeBytes
(
    byte    address,
    byte    length,
    byte*   p_data
){
    // Write first page if not aligned.
    byte notAlignedLength = 0;
    byte pageOffset = address % EEPROM__PAGE_SIZE;
    if (pageOffset > 0)
    {
        notAlignedLength = EEPROM__PAGE_SIZE - pageOffset;
        if (length < notAlignedLength)
        {
            notAlignedLength = length;
        }
        writePage(address, notAlignedLength, p_data);
        length -= notAlignedLength;
    }

    if (length > 0)
    {
        address += notAlignedLength;
        p_data += notAlignedLength;

        // Write complete and aligned pages.
        byte pageCount = length / EEPROM__PAGE_SIZE;
        for (byte i = 0; i < pageCount; i++)
        {
            writePage(address, EEPROM__PAGE_SIZE, p_data);
            address += EEPROM__PAGE_SIZE;
            p_data += EEPROM__PAGE_SIZE;
            length -= EEPROM__PAGE_SIZE;
        }

        if (length > 0)
        {
            // Write remaining uncomplete page.
            writePage(address, length, p_data);
        }
    }
}

/**************************************************************************//**
 * \fn byte Eeprom24C01_02::readByte(byte address)
 * 
 * \brief Read a byte in EEPROM memory.
 *
 * \param   address Address.
 *
 * \return Read byte.
 ******************************************************************************/
byte
Eeprom24C01_02::readByte
(
    byte address
){
    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(m_deviceAddress, (byte)1);
    byte data = 0;
    if (Wire.available())
    {
        data = Wire.read();
    }
    return data;
}

/**************************************************************************//**
 * \fn void Eeprom24C01_02::readBytes(
 * byte     address,
 * byte     length,
 * byte*    p_data)
 *
 * \brief Read bytes in EEPROM memory.
 *
 * \param       address Start address.
 * \param       length  Number of bytes to read.
 * \patam[in]   p_data  Byte array to fill with read bytes.
 ******************************************************************************/
void
Eeprom24C01_02::readBytes
(
    byte    address,
    byte    length,
    byte*   p_data
){
    byte bufferCount = length / EEPROM__RD_BUFFER_SIZE;
    for (byte i = 0; i < bufferCount; i++)
    {
        byte offset = i * EEPROM__RD_BUFFER_SIZE;
        readBuffer(address + offset, EEPROM__RD_BUFFER_SIZE, p_data + offset);
    }

    byte remainingBytes = length % EEPROM__RD_BUFFER_SIZE;
    byte offset = length - remainingBytes;
    readBuffer(address + offset, remainingBytes, p_data + offset);
}

/******************************************************************************
 * Private method definitions.
 ******************************************************************************/

/**************************************************************************//**
 * \fn void Eeprom24C01_02::writePage(
 * byte     address,
 * byte     length,
 * byte*    p_data)
 *
 * \brief Write page in EEPROM memory.
 *
 * \param       address Start address.
 * \param       length  Number of bytes (EEPROM__PAGE_SIZE bytes max).
 * \param[in]   p_data  Data.
 ******************************************************************************/
void
Eeprom24C01_02::writePage
(
    byte    address,
    byte    length,
    byte*   p_data
){
    // Write complete buffers.
    byte bufferCount = length / EEPROM__WR_BUFFER_SIZE;
    for (byte i = 0; i < bufferCount; i++)
    {
        byte offset = i * EEPROM__WR_BUFFER_SIZE;
        writeBuffer(address + offset, EEPROM__WR_BUFFER_SIZE, p_data + offset);
    }

    // Write remaining bytes.
    byte remainingBytes = length % EEPROM__WR_BUFFER_SIZE;
    byte offset = length - remainingBytes;
    writeBuffer(address + offset, remainingBytes, p_data + offset);
}

/**************************************************************************//**
 * \fn void Eeprom24C01_02::writeBuffer(
 * byte     address,
 * byte     length,
 * byte*    p_data)
 *
 * \brief Write bytes into memory.
 *
 * \param       address Start address.
 * \param       length  Number of bytes (EEPROM__WR_BUFFER_SIZE bytes max).
 * \param[in]   p_data  Data.
 ******************************************************************************/
void
Eeprom24C01_02::writeBuffer
(
    byte    address,
    byte    length,
    byte*   p_data
){
    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address);
    for (byte i = 0; i < length; i++)
    {
        Wire.write(p_data[i]);
    }
    Wire.endTransmission();
    
    // Write cycle time (tWR). See EEPROM memory datasheet for more details.
    delay(10);
}

/**************************************************************************//**
 * \fn void Eeprom24C01_02::readBuffer(
 * byte     address,
 * byte     length,
 * byte*    p_data)
 *
 * \brief Read bytes in memory.
 *
 * \param       address Start address.
 * \param       length  Number of bytes (EEPROM__RD_BUFFER_SIZE bytes max).
 * \param[in]   p_data  Buffer to fill with read bytes.
 ******************************************************************************/
void
Eeprom24C01_02::readBuffer
(
    byte    address,
    byte    length,
    byte*   p_data
){
    Wire.beginTransmission(m_deviceAddress);
    Wire.write(address);
    Wire.endTransmission();
    Wire.requestFrom(m_deviceAddress, length);
    for (byte i = 0; i < length; i++)
    {
        if (Wire.available())
        {
            p_data[i] = Wire.read();
        }
    }
}

