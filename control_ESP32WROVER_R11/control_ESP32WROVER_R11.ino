//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial


#include "BluetoothSerial.h"
#include <HardwareSerial.h> 

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>
#include <HTTPUpdate.h>

// OTA지원을 위한 버젼 관리
String version="Current: ctrl_esp32wrover_r1";
String nextVersion="ctrl_esp32wrover_rxx.bin";

WiFiMulti wifiMulti;

//how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 3
const char* ssid = "topwave";
const char* password = "33553355";

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS];


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

HardwareSerial bluetooth1(2); //3개의 시리얼 중 2번 채널을 사용 ---- psh

BluetoothSerial SerialBT;

// 시리얼 통신 프로토콜 등 사용 변수 및 함수 선언
const char* cmd = "BS";

char door='1';
char lock='1';
char scan='0';

String serString="";
String Txbuffer="";

// int door_check_enable = 0;
int close_check = 0;
int open_check = 0;
int door_check = 0;

void serialProc();
void chk_interrupt();
void check_door();


// gpio 포트 선언
int LED1_DOOR = 2, LED2_LOCK = 4, LED3_SCAN = 5;
int TLIMIT = 12, BLIMIT = 14;
// int 12VOUT_CUT = 18;
int SOL_LOCK1 = 19, SOL_LOCK2 = 23;
int AC_M_ON = 27, AC_M_FWD = 33;
int DOOR_IN = 32;    // input

// Timer 설정
volatile bool interruptCounter=false;
hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
  interruptCounter=true;
  }


void setup() {
  // interrupt_init
  timer = timerBegin(0, 80, true);    // 80MHz, division 80 = 1MHz
  timerAttachInterrupt(timer, &onTimer, true);  
  timerAlarmWrite(timer, 100000, true);    // default 100ms, count 1000000 = 1sec, 1000=1msec
  timerAlarmEnable(timer);

  // gpio setup
  pinMode(LED1_DOOR, OUTPUT);
  pinMode(LED2_LOCK, OUTPUT);
  pinMode(LED3_SCAN, OUTPUT);
  pinMode(SOL_LOCK1, OUTPUT);
  pinMode(SOL_LOCK2, OUTPUT);
  pinMode(AC_M_ON, OUTPUT);
  pinMode(AC_M_FWD, OUTPUT);
//  pinMode(12VOUT_CUT, OUTPUT);
  pinMode(TLIMIT, INPUT);
  pinMode(BLIMIT, INPUT);
  pinMode(DOOR_IN, INPUT);

// gpio value default setup
  digitalWrite(AC_M_FWD, HIGH);
  digitalWrite(AC_M_ON, HIGH);

    
  // serial setup
  Serial.begin(115200);     // default 디버그용 시리얼
  bluetooth1.begin(115200, SERIAL_8N1, 21, 22); //추가로 사용할 시리얼. RX:21 / TX:22번 핀 사용
    
  SerialBT.begin("ESP32bkmoonW"); //Bluetooth device name  블루투스 시리얼 통신 선언
  Serial.println("The device started, now you can pair it with bluetooth!");


//  wifi setup
  Serial.println("\nConnecting");

  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting Wifi ");
  for (int loops = 10; loops > 0; loops--) {
    if (wifiMulti.run() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("WiFi connected ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      break;
    }
    else {
      Serial.println(loops);
      delay(1000);
    }
  }
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connect failed");
    delay(1000);
    ESP.restart();
  }

  //start UART and the server
  // Serial2.begin(9600);
  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  ota();
  
}

void ota() {
  
  Serial.println("firmware version check for OTA");   
  Serial.println(version);

  String updateAddr="http://192.168.0.27/"+nextVersion;

  Serial.println(updateAddr);
  
  t_httpUpdate_return ret = httpUpdate.update(serverClients[0], updateAddr);
  // Or:
  //t_httpUpdate_return ret = httpUpdate.update(client, "server", 80, "/file.bin");
  
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  
}

void loop() {
  uint8_t i;

  // put your main code here, to run repeatedly:

  serialProc();


  if (interruptCounter) { 
        // Serial.println("100 msec");
        interruptCounter=false;
        check_door();
  }
  
 /* if (door_check_enable == 1)
       check_door();
*/

  if (wifiMulti.run() == WL_CONNECTED) {
    //check if there are any new clients
    if (server.hasClient()){
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        //find free/disconnected spot
        if (!serverClients[i] || !serverClients[i].connected()){
          if(serverClients[i]) serverClients[i].stop();
          serverClients[i] = server.available();
          if (!serverClients[i]) Serial.println("available broken");
          Serial.print("New client: ");
          Serial.print(i); Serial.print(' ');
          Serial.println(serverClients[i].remoteIP());
          break;
        }
      }
      if (i >= MAX_SRV_CLIENTS) {
        //no free/disconnected spot so reject
        server.available().stop();
      }
    }
    //check clients for data
 /*
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      if (serverClients[i] && serverClients[i].connected()){
        if(serverClients[i].available()){
          //get data from the telnet client and push it to the UART
          while(serverClients[i].available()) Serial2.write(serverClients[i].read());
        }
      }
      else {
        if (serverClients[i]) {
          serverClients[i].stop();
        }
      }
    }
*/
    }
          
}

void serialProc()
{ 
  uint8_t i;
  
  //시리얼 데이터가 있는지 체크 
  //없으면 그냥 빠져나감.
  if(SerialBT.available() <= 0)
    return;
    
  //데이터 있으면 1바이트 읽어옴.
  char ch = SerialBT.read();
  // 변수에 추가함.
  serString += ch;      
  
  // 입력 받은 데이터가 '\r'  이면...
  if(ch == '\r')
  {       
    //여지까지 입력 받은 데이터가 있는지 체크 
    if(serString.length()>0)
    {

        //SerialBT.print(serString);                        
        
        String serArray[10];
        int tmpcnt=0;
        int idx;
    
        String tmpString=serString;
        
        //공백 제거 
        tmpString.trim();  
 
        // 데이터 파싱         
        while(tmpString.length() > 0)
        {
          // ','를 기준으로 짤라서 serArray에 저장.
          idx = tmpString.indexOf(",");    
          if(idx == -1)
          {
            //','없다면 마지막 데이터 저장후 빠져나감.
            serArray[tmpcnt] = tmpString;
            serArray[tmpcnt].trim();       
            tmpcnt++;                      
            break;
          }
          
          serArray[tmpcnt] = tmpString.substring(0, idx); 
          tmpString = tmpString.substring(idx+1); 
          tmpString.trim();
          serArray[tmpcnt].trim();   
                
          tmpcnt++;              
        }
              
        //명령 serArray[0]이 'FW'
        if(serArray[0].equalsIgnoreCase("FW"))
        {
          // 텔넷으로 로그 출력
          for(i = 0; i < MAX_SRV_CLIENTS; i++){
            if (serverClients[i] && serverClients[i].connected()){
              serverClients[i].write("Firmware update\r\n");
              delay(1);
            }
          }
          ota();
          // report_to_server();   
        }
          
        //명령 serArray[0]이 'BR'
        if(serArray[0].equalsIgnoreCase("BR"))
        {
          // 텔넷으로 로그 출력
          for(i = 0; i < MAX_SRV_CLIENTS; i++){
            if (serverClients[i] && serverClients[i].connected()){
              serverClients[i].write("Board Read\r\n");
              delay(1);
            }
          }
          report_to_server();   
        }

        //명령 serArray[0]이 'UD'
        if(serArray[0].equalsIgnoreCase("UD"))
        {
          // 텔넷으로 로그 출력
          for(i = 0; i < MAX_SRV_CLIENTS; i++){
            if (serverClients[i] && serverClients[i].connected()){
              serverClients[i].write("Unlock Door\r\n");
              delay(1);
            }
          }   
          digitalWrite(SOL_LOCK1, HIGH); // unlock SOL_LOCK1
          digitalWrite(SOL_LOCK2, HIGH); // unlock SOL_LOCK2
          lock = '0';
          digitalWrite(LED2_LOCK, HIGH); // LED2 off
          report_to_server();
        }

        //명령 serArray[0]이 'LD'
        if(serArray[0].equalsIgnoreCase("LD"))
        {
          // 텔넷으로 로그 출력
          for(i = 0; i < MAX_SRV_CLIENTS; i++){
            if (serverClients[i] && serverClients[i].connected()){
              serverClients[i].write("Lock Door\r\n");
              delay(1);
            }
          }   
          digitalWrite(SOL_LOCK1, LOW); // lock SOL_LOCK1
          digitalWrite(SOL_LOCK2, LOW); // lock SOL_LOCK2
          lock = '1';
          digitalWrite(LED2_LOCK, LOW); // LED2 on
          report_to_server();
        }
        
        //명령 serArray[0]이 'RS'  
        else if(serArray[0].equalsIgnoreCase("RS"))
        {
          //명령 serArray[1] 숫자로 변경  - id 
          int speed = serArray[1].toInt();
          // 텔넷으로 로그 출력
          for(i = 0; i < MAX_SRV_CLIENTS; i++){
            if (serverClients[i] && serverClients[i].connected()){
              serverClients[i].write("RFID Scanning...\r\n");
              delay(1);
            }
          }   

        scan = '1';
        report_to_server();
        
   //    Rfid_Scanning(atoi(serArray[1]));
        Rfid_Scanning();
        
        scan = '0';
        report_to_server();               
        }       

    
    }     // if(serString.length()>0)
    serString = "";
  }   // if(ch == '\r')
}


void report_to_server()
{
          Txbuffer = String(cmd) + "," + String(door) + "," + String(lock) + "," + String(scan) + "," + String('\r');
          int total = 0, sum = 0;
          int len = Txbuffer.length();
          
          for(int i=0; i<len; i++) 
            sum += Txbuffer[i];   
        
          total = sum;
          total = total & 0xff;
          total = ~total + 1;

          SerialBT.print(Txbuffer);
          SerialBT.write(total);
}

void Rfid_Scanning()
{
    digitalWrite(LED3_SCAN, LOW); // LED3 on
  //  if(speed !=0 ) {  // first normal scanning
      delay(2000);
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction
      delay(10);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on
      while(digitalRead(TLIMIT) == HIGH);      // Top position?
       
        
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while(digitalRead(TLIMIT) == HIGH);      // Top position?
    
        
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, HIGH); // CCW(DOWN) direction
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on

   
      while(digitalRead(BLIMIT) == HIGH); 
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction
  //  } 
    /*
    else {  // retrying scan in slow speed
    
      delay(3000);      // wait 3s for operating RFID Reader
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction
      delay(10);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on
      while(TLIMIT == HIGH);      // Top position?


      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while(TLIMIT == HIGH);      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while(TLIMIT == HIGH);      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while(TLIMIT == HIGH);      // Top position?
      
      
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, HIGH); // CCW(DOWN) direction
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on

      while(BLIMIT == HIGH); 
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction  

      digitalWrite(LED3_SCAN, HIGH); // LED3 off
    }
    */
}

/* void chk_interrupt()
{
  if (interruptCounter) { 
        interruptCounter=false;
        Serial.println("100 msec");
        door_check_enable = 1;
   }
}
*/

void check_door()
{
 // door_check_enable = 0;
 // Serial.println("checking door status:\r\n");
  
  if (digitalRead(DOOR_IN) == HIGH)
    open_check++;   // open
  else
    close_check++;  // close   

  if (door_check > 3) {  // 5 times when door_check is 0,1,2,3,4
    if (open_check > 3) { // open
      digitalWrite(LED1_DOOR, HIGH); // LED1 off
      if ( door == '1') {    // "1," == closed
        door = '0';
        report_to_server();
        Serial.println("Door is opened!\r\n");
      }
    }
    if (close_check > 3) {  //close
      digitalWrite(LED1_DOOR, LOW); // LED1 on
      if ( door == '0') {    // "0," == opened
        door = '1';
        report_to_server();
        Serial.println("Door is closed!\r\n");
      }
    }
    door_check = 0;
    open_check = 0;
    close_check = 0;

  } else
    door_check++;
}
