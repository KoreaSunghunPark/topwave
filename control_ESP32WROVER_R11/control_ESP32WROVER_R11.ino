//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial


#include "BluetoothSerial.h"
#include <HardwareSerial.h> 

#include <esp_bt.h>
#include <esp_bt_device.h>

#include <Arduino.h>
#include <WiFi.h>
// #include <WiFiMulti.h>

// webserverd
#include <WebServer.h>

// HTTP OTA
#include <HTTPClient.h>
#include <HTTPUpdate.h>

//mdns
#include <ESPmDNS.h>

// OTA지원을 위한 버젼 관리
String version="FBOX_R06";
String nextVersion="FBOX_R07.bin";
// String version="ctrl_esp32wrover_r04";
// String nextVersion="ctrl_esp32wrover_r05.bin";

unsigned long previousMillis = 0;
unsigned long CheckTimeOTA = 300000;   // 5min
// unsigned long CheckTimeOTA = 60000; // 60sec

// WiFiMulti wifiMulti;

//how many clients should be able to telnet to this ESP32
#define MAX_SRV_CLIENTS 3

char* password = "33553355";
char ssid[64];
// mdns
const char* host = "foodbox";

// const char* ssid = "topwave";
// const char* password = "33553355";


// telnet
WiFiServer server(23);

WiFiClient serverClients[MAX_SRV_CLIENTS];
// http
WebServer httpserver(80);

/*
 * Login page
 */

const char* loginIndex = 
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>FOODBOX</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<td>Username:</td>"
        "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='3355')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";
 
/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";



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

int close_check = 0;
int open_check = 0;
int door_check = 0;

uint8_t motorDrivingCount=5;
// unsigned long CheckTimeDoor = 60000;   // 1min
unsigned long CheckTimeDoor = 300000;   // 5min
unsigned long doorOpenMillis = 0;


void serialProc();
void chk_interrupt();
void check_door(); 


// gpio 포트 선언
const int LED1_DOOR = 15, LED2_LOCK = 4, LED3_SCAN = 5;    // default LED1_DOOR = 15,  old bd 2
const int TLIMIT = 13, BLIMIT = 14;                       // default TLIMIT = 13, old bd 12
// const int 12VOUT_CUT = 18;
const int SOL_LOCK1 = 19, SOL_LOCK2 = 23;
const int AC_M_ON = 27, AC_M_FWD = 33;
const int DOOR_IN = 32;    // input


// Timer 설정
volatile bool interruptCounter=false;
hw_timer_t *timer = NULL;

void IRAM_ATTR onTimer() {
  interruptCounter=true;
  }

// wifi setup - auto scan & connection
void wifi_scan_connection();

// write to telnet
void debuglog_telnet(char* msg);

// bluetooth MAC
void printDeviceAddress();

void setup() {
  // interrupt_init
  timer = timerBegin(0, 80, true);    // 80MHz, division 80 = 1MHz
  timerAttachInterrupt(timer, &onTimer, true);  
  timerAlarmWrite(timer, 100000, true);    // default 100000 = 100ms, count 1000000 = 1sec, 1000=1msec
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
    
  SerialBT.begin(version); //Bluetooth device name  블루투스 시리얼 통신 선언
  Serial.println("The device started, now you can pair it with bluetooth!");
  printDeviceAddress();
    
  esp_power_level_t min,max;  
  esp_bredr_tx_power_set(ESP_PWR_LVL_P6, ESP_PWR_LVL_P9);   // +6 dbm ~ +9 dbm
  delay(1000);
  esp_bredr_tx_power_get(&min,&max);
  Serial.printf("Tx power(index): min %d max %d\r\n",min,max);
        

  // wifi setup - auto scan & connection
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  Serial.print("Wating 30 seconds for booting foodbox AP..");
  for(uint8_t i=0; i<30; i++) {
        Serial.print('.');
        delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Setup done");

  wifi_scan_connection();   // foodbox AP에 자동 접속

  
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://foodbox.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("mDNS responder started");

// http server(80)
 /*return index page which is stored in serverIndex */
  httpserver.on("/", HTTP_GET, []() {
    httpserver.sendHeader("Connection", "close");
    httpserver.send(200, "text/html", loginIndex);
  });
  httpserver.on("/serverIndex", HTTP_GET, []() {
    httpserver.sendHeader("Connection", "close");
    httpserver.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  httpserver.on("/update", HTTP_POST, []() {
    httpserver.sendHeader("Connection", "close");
    httpserver.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = httpserver.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  httpserver.begin();
  Serial.println("OTA Http WebUpdater is enabled!");

  

/*
//  wifi setup - multi connection
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
*/

  //start UART and the server
  // Serial2.begin(9600);
  server.begin();
  server.setNoDelay(true);

  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");

  // OTA
  ota();
 
}   // end of setup()

void ota() {
  
  Serial.println("");
  Serial.println("firmware version check for OTA");
  Serial.print("Current version: ");   
  Serial.println(version);

  Serial.print("Target: ");
  String updateAddr="http://www.topwave.co.kr/esp32/"+nextVersion;
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
  httpserver.handleClient();

  serialProc();

  // OTA & motor driving for prevent freezing
  unsigned long currentMillis = millis();
  if((currentMillis - previousMillis) > CheckTimeOTA) {     // 5 minutes
    previousMillis = currentMillis;
    ota();
     
  // motor driving for prevent freezing
    if(motorDrivingCount < 5) {
      motorDrivingCount++;
      // debuglog_telnet("motor driving for prevent freezing: ");
    
      Serial.print("motor driving for prevent freezing: ");
      Serial.println(motorDrivingCount);
      Rfid_Scanning(5);
    }
  }


  // door check routine. open? close?
  if (interruptCounter) { 
        // Serial.println("100 msec");
        interruptCounter=false;
        check_door();
  }

  // telnet connecton 
  if (WiFi.status() == WL_CONNECTED) {
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

//    for(i = 0; i < MAX_SRV_CLIENTS; i++){
//      if (serverClients[i] && serverClients[i].connected()){
//       if(serverClients[i].available()){
//          //get data from the telnet client and push it to the UART
//          while(serverClients[i].available()) Serial2.write(serverClients[i].read());
//        }
//      }
//      else {
//        if (serverClients[i]) {
//          serverClients[i].stop();
//        }
//      }
//    }

    } // if (wifiMulti.run() == WL_CONNECTED)

          
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
          debuglog_telnet("Firmware update\r\n");
          ota();
          // report_to_server();   
        }
          
        //명령 serArray[0]이 'BR'
        if(serArray[0].equalsIgnoreCase("BR"))
        {
          // 텔넷으로 로그 출력

          debuglog_telnet("Board read\r\n"); 
          report_to_server();   
        }

        //명령 serArray[0]이 'UD'
        if(serArray[0].equalsIgnoreCase("UD"))
        {
          // 텔넷으로 로그 출력
          debuglog_telnet("Unlock Door\r\n"); 
       
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
          debuglog_telnet("Lock Door\r\n"); 
       
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
         // int speed = serArray[1].toInt();
          
          // 텔넷으로 로그 출력
          
  
          scan = '1';
          report_to_server();
          debuglog_telnet("RFID Scan\r\n");
          Rfid_Scanning(serArray[1].toInt());
       //   debuglog_telnet("RFID Scanning is completed.\r\n");
       //   scan = '0';
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

void Rfid_Scanning(int speed)
{
    digitalWrite(LED3_SCAN, LOW); // LED3 on
    
    unsigned long scanStartTime = millis();
    
    if(speed !=0 ) {  // first normal scanning
      Serial.println("   normal scanning...\r\n");
      // debuglog_telnet("   normal scanning...\r\n");
      delay(2000);
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction
      delay(10);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on
    //  while(digitalRead(TLIMIT) == LOW);      // Top position?
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?
               
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, HIGH); // CCW(DOWN) direction
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on
   
    //  while(digitalRead(BLIMIT) == LOW);
      while((digitalRead(BLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Bottom position? 
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction
    } 
    else {  // retrying scan in slow speed
      Serial.println("   retrying scan in slow speed...\r\n");
      debuglog_telnet("   retrying scan in slow speed...\r\n");    
      delay(3000);      // wait 3s for operating RFID Reader
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction
      delay(10);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?
      
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?

      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(500);
      digitalWrite(AC_M_ON, LOW); // CW(UP) direction
      while((digitalRead(TLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Top position?
    
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, HIGH); // CCW(DOWN) direction
      delay(1000);
      digitalWrite(AC_M_ON, LOW); // AC Motor Power on

      while((digitalRead(BLIMIT) == LOW) && ((millis() - scanStartTime) < 20000 ));      // Bottom position?
      digitalWrite(AC_M_ON, HIGH); // AC Motor Power off
      delay(10);
      digitalWrite(AC_M_FWD, LOW); // CW(UP) direction  

      digitalWrite(LED3_SCAN, HIGH); // LED3 off
    }
    // operaton is succeed?   
    if( (millis() - scanStartTime) <= 20000 ) {   // 20000 msec
      scan = '0';
      Serial.println("RFID Scanning is completed.\r\n");
      // debuglog_telnet("RFID Scanning is completed.\r\n");  
    } else
    {
      // scan = '1';
      Serial.println("Motor operation is fail!!\r\n");
      // debuglog_telnet("Motor operation is fail!!\r\n");
    }
   
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
        // check current time when door is opend  
        doorOpenMillis = millis();
     }
    } // end of open_check
    if (close_check > 3) {  //close
      digitalWrite(LED1_DOOR, LOW); // LED1 on
      if ( door == '0') {    // "0," == opened
        door = '1';
        report_to_server();
        Serial.println("Door is closed!\r\n");
        // check how long door is opened?
        unsigned long doorCloseMillis = millis();
        if((doorCloseMillis - doorOpenMillis) > CheckTimeDoor) {     // over 5 minutes?
          Serial.println("starting prevent freezing action!\r\n");
          motorDrivingCount = 0;
        }
      }
    }  // end of close_check 
    door_check = 0;
    open_check = 0;
    close_check = 0;

  } else
    door_check++;
}

void wifi_scan_connection()
{
  int mynetwork = 0;
  int status = WL_IDLE_STATUS;
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
      if(WiFi.SSID(i).substring(0,7) == "foodbox") {
        mynetwork = i;
        break;
      }
     }
      // connect foodbox AP     
      strcpy(ssid, WiFi.SSID(mynetwork).c_str()); 
      Serial.print("Connection to:");
      Serial.print(ssid);
      Serial.print("/");
      Serial.print(WiFi.RSSI(mynetwork));
      Serial.print("/");
      Serial.println(password);
      // Serial.println(WiFi.SSID(mynetwork));
          
      status = WiFi.begin(ssid, password);
      
      Serial.print("Connecting to WiFi ..");
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
      }
      Serial.println(WiFi.localIP());

  
  }
  Serial.println("");
}

void debuglog_telnet(char* msg)
{
        uint8_t i;
        for(i = 0; i < MAX_SRV_CLIENTS; i++){
            if (serverClients[i] && serverClients[i].connected()){
              serverClients[i].write(msg);
              delay(1);
            }
          }
}

void printDeviceAddress() {
 
  const uint8_t* point = esp_bt_dev_get_address();
 
  for (int i = 0; i < 6; i++) {
 
    char str[3];
 
    sprintf(str, "%02X", (int)point[i]);
    Serial.print(str);
 
    if (i < 5){
      Serial.print(":");
    }
 
  }
  Serial.println("");
}
