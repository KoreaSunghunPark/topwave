//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
//#include <SoftwareSerial.h> //psh
#include <HardwareSerial.h> //psh

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

HardwareSerial bluetooth1(2); //3개의 시리얼 중 2번 채널을 사용 ---- psh

BluetoothSerial SerialBT;

//SoftwareSerial bluetooth1(21,22); //psh: RXD1->GPIO21, TXD1->GPIO22

void setup() {
  Serial.begin(115200);
  
//  bluetooth1.begin(115200);
  bluetooth1.begin(115200, SERIAL_8N1, 21, 22); //추가로 사용할 시리얼. RX:21 / TX:22번 핀 사용
    
  SerialBT.begin("ESP32test"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
}

void loop() {
  char data_bt;
  char data_sr;
  
  delay(20);
  
  // put your main code here, to run repeatedly:
  //bluetooth1.listen();

  //Serial Data to BT
  if (bluetooth1.available()) {
    data_sr = bluetooth1.read();
    
    SerialBT.write(data_sr);
    Serial.write(data_sr);    
  }  
 
  
  //BT data to Serial
  if (SerialBT.available()) {
    data_bt = SerialBT.read();
    
    bluetooth1.write(data_bt);  
    Serial.write(data_bt);
  }

  if (Serial.available()) {
    data_sr = Serial.read();
    
    bluetooth1.write(data_sr);      
    SerialBT.write(data_sr);        
  }  
  
}
