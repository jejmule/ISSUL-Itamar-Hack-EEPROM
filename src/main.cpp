
/******************************************************************************
 * Header file inclusions.
 ******************************************************************************/
#include <Eeprom24C01_02.h>

/**************************************************************************//**
 * \def EEPROM_ADDRESS
 * \brief Address of EEPROM memory on TWI bus.
 ******************************************************************************/
#define EEPROM_ADDRESS 0x50

/******************************************************************************
 * Private variable definitions.
 ******************************************************************************/

static Eeprom24C01_02 eeprom(EEPROM_ADDRESS);

//union to decode 4 byte unsigned int
union uint_union {
  byte bytes[4] ;
  unsigned int integer ;
} ;
uint_union id ;
uint_union lot ;
uint_union test ;
//current device state
byte state;
//store last id and lot to check if a new device is connected
unsigned int last_id, last_lot = 0;

void setup()
{
    // Initialize serial communication.
    Serial.begin(9600);
    Serial.setTimeout(500);
    
    // Initiliaze EEPROM library.
    eeprom.initialize();

    //init bluiltin led
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(LED_BUILTIN,LOW);
}

/**************************************************************************//**
 * \fn void loop()
 *
 * \brief
 ******************************************************************************/

void loop()
{
  //check if a device is attached by checking if 4 first bytes are null
  byte check = eeprom.readByte(0) ;
  if (check !=0 ) {
    //read serial, lot, test and state
    eeprom.readBytes(5,4,id.bytes);
    eeprom.readBytes(9,4,lot.bytes);
    eeprom.readBytes(124,4,test.bytes);
    state = eeprom.readByte(15);
    //Check if a new device is connected by comparing id and lot number
    if(last_lot != lot.integer || last_id != id.integer) {
      //update last id / lot
      last_id = id.integer ;
      last_lot = lot.integer ;
      //if the device is reprogrammed for the first time eeprom contains FF FF FF FF and is reprogrammed to 0
      if(test.integer == UINT32_MAX) {
        test.integer = 0;
        eeprom.writeBytes(124,4,test.bytes);
      }
      Serial.println("Attached device : Serial : "+String(id.integer)+"\tLot : "+String(lot.integer)+"\tState : "+String(bitRead(state,1))+"\tTests : "+String(test.integer));
      //reprogramed the device if state is 1, check if bit 1 is 0
      if(bitRead(state,1) == 0) {
        bitWrite(state,1,1);
        eeprom.writeByte(15,state);
        test.integer += 1 ;
        delay(10);
        eeprom.writeBytes(124,4,test.bytes);
        Serial.println("-> Reset device ... "+String(test.integer)+" test(s) performed");
      }
      digitalWrite(LED_BUILTIN,HIGH);
    }
  }
  //if no device is connected, reset last id and lot 
  else {
    last_id = 0 ; 
    last_lot = 0 ;
    digitalWrite(LED_BUILTIN,LOW);
  }
  if(Serial.available()>0){
    String input = Serial.readString();
    if(input == "r" ){
      Serial.println("read buffer");
      byte outputBytes[128] = { 0 };
      eeprom.readBytes(0,128,outputBytes);
      for (byte i = 0; i < 128; i++)
      {
          Serial.print(outputBytes[i],HEX);
          Serial.print(" ");
      }
      Serial.println(""); 
    }
    else if (input == "w" ){
      Serial.println("write buffer");
      byte outputBytes[4] = { 0xFF,0xFF,0xFF,0xFF};
      eeprom.writeBytes(124,4,outputBytes);
    }
    else {
      Serial.println("other input : "+input);
      eeprom.writeByte(15,input.toInt());
    }
  }
  //wait
  delay(200);
}
