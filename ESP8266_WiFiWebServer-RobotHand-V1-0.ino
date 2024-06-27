//#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#define WIFIMODE_CONNECT 0
#define WIFIMODE_HOTSPOT 1

Adafruit_PWMServoDriver pwm1;
const uint8_t myLED = LED_BUILTIN;
long counter = 0;
uint8_t sensor = A0;

#define SERVOMIN  170 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  570 // this is the 'maximum' pulse length count (out of 4096)

#define PINKEY 0
#define RING 1
#define MIDDLE 2
#define INDEX 3
#define THUMB 4
#define WRIST1 5
#define WRIST2 6

String ipToString(IPAddress ip) {
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

void setFinger(uint8_t f, String pos) {  // Servo min/max calibration is set here!
  int val = 300;
  if (f == PINKEY) {
    if (pos == "IN") val = SERVOMAX-10; else val = SERVOMIN;
  }
  else if (f == RING) {
    if (pos == "IN") val = SERVOMAX-20; else val = SERVOMIN;
  }
  else if (f == MIDDLE) {
    if (pos == "IN") val = SERVOMAX-60; else val = SERVOMIN;
  }
  else if (f == INDEX) {
    if (pos == "IN") val = SERVOMAX-30; else val = SERVOMIN;
  }
  else if (f == THUMB) {
    if (pos == "IN") val = SERVOMIN; else val = SERVOMAX-50;
  }
  pwm1.setPWM(f, 0, val);
}

void setHand(uint8_t f0, uint8_t f1,uint8_t f2,uint8_t f3,uint8_t f4) { // PINKEY, RING, MIDDLE, INDEX, THUMB
  if (f0>0) setFinger(PINKEY, "OUT"); else setFinger(PINKEY, "IN");
  if (f1>0) setFinger(RING, "OUT"); else setFinger(RING, "IN");
  if (f2>0) setFinger(MIDDLE, "OUT"); else setFinger(MIDDLE, "IN");
  if (f3>0) setFinger(INDEX, "OUT"); else setFinger(INDEX, "IN");
  if (f4>0) setFinger(THUMB, "OUT"); else setFinger(THUMB, "IN");
}

void setWrist(uint8_t w, uint8_t perc) {
  float aux = (SERVOMAX - SERVOMIN) / 100.0;
  aux = SERVOMIN + perc * aux;
  Serial.print("Wrist "); Serial.print(w); Serial.print(": "); Serial.println((int)aux);
  pwm1.setPWM(w, 0, (int)aux);
}

IPAddress myIPaddress;
WiFiServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(9600);
  Serial.println("Start: wait 5 sec...");
  delay(5000);
  Wire.begin(D3,D4);
  pwm1 = Adafruit_PWMServoDriver(0x40);
  delay(10);
  Serial.print("myLED: "); Serial.println(myLED);

  Serial.println("16 channel PWM test! setPWMFreq(60)");
  pwm1.begin();
  pwm1.setPWMFreq(60);  // This is the maximum PWM frequency
  delay(10);

  pinMode(myLED, OUTPUT);
  digitalWrite(myLED, 0);
  pinMode(sensor, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  pinMode(D8, INPUT);

  setHand(1,1,1,1,1);
//  setWrist(WRIST1,100);
//  setWrist(WRIST2,30);

  pinMode(D2, INPUT_PULLUP);
  delay(100);
  int wifimode;
  if (digitalRead(D2) == HIGH) wifimode = WIFIMODE_CONNECT; else wifimode = WIFIMODE_HOTSPOT;
  
  if (wifimode==WIFIMODE_CONNECT) {
    ESP8266WiFiMulti wifiMulti;
    wifiMulti.addAP("GalaxyS8plusz", "wifi1234");
    wifiMulti.addAP("DELL-G5", "wifi1234");
    wifiMulti.addAP("Ittatuti", "Cisco-nem1szeru");
    Serial.println();
    Serial.print("Connecting Station to ");
    while (wifiMulti.run() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("WiFi connected: SSID=");
    Serial.println(WiFi.SSID());
    myIPaddress = WiFi.localIP();
    Serial.print("Station IP address: ");
  }
  else if (wifimode==WIFIMODE_HOTSPOT) {
    Serial.println();
    Serial.println("Setting up Access Point");
    WiFi.mode(WIFI_AP_STA);
    IPAddress apIP(42, 42, 42, 42);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("RobotHand", "wifi1234");
    myIPaddress = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
  }
  Serial.println(myIPaddress);
  
  // Start the server
  server.begin();
  Serial.println("Server(80) started");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket(81) started");
  for (int i=0; i<4; i++) { digitalWrite(myLED, 1); delay(50); digitalWrite(myLED, 0); delay(50); }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT){
    String command = String((const char *)payload);
    Serial.println(command);
    if (command.indexOf("/d0/0") != -1) setFinger(PINKEY, "OUT");
    else if (command.indexOf("/d0/1") != -1) setFinger(PINKEY, "IN");
  }
}

long i = 0;
uint8_t val = 1;
uint8_t shoot = 0;
uint8_t binary = 0;
uint8_t binaryCnt = 0;

void loop() {
  webSocket.loop();
  
  if (val==0) {
    for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {
      for (int j=0; j<5; j++) pwm1.setPWM(j, 0, pulselen);
    }
    delay(500);
    for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {
      for (int j=0; j<5; j++) pwm1.setPWM(j, 0, pulselen);
    }
    delay(500);
  }
  if (shoot==1) {
    int aRD5 = analogRead(D5);
    int aRD6 = analogRead(D6);
    int aRD7 = analogRead(D7);
    int aRD8 = analogRead(D8);
/*    Serial.print(aRD5); Serial.print(" ");
    Serial.print(aRD6); Serial.print(" ");
    Serial.print(aRD7); Serial.print(" ");
    Serial.print(aRD8); Serial.println(" ");
*/    
    if (aRD5 > 200) {
      setFinger(INDEX, "IN");
    }
    if (aRD6 > 200) {
      setFinger(MIDDLE, "IN");
    }
    if (aRD7 > 200) {
      setFinger(RING, "IN");
    }
    if (aRD8 > 200) {
      setFinger(PINKEY, "IN");
    }
  }

  if (binary==1) {
    uint8_t bc = binaryCnt;
    if (bc>15) { setFinger(PINKEY, "OUT"); bc -= 16; } else setFinger(PINKEY, "IN");
    if (bc>7)  { setFinger(RING, "OUT"); bc -= 8;  } else setFinger(RING, "IN");
    if (bc>3)  { setFinger(MIDDLE, "OUT"); bc -= 4;  } else setFinger(MIDDLE, "IN");
    if (bc>1)  { setFinger(INDEX, "OUT"); bc -= 2;  } else setFinger(INDEX, "IN");
    if (bc>0)  { setFinger(THUMB, "OUT"); bc -= 1;  } else setFinger(THUMB, "IN");
    binaryCnt = (binaryCnt + 1) % 32;
    delay(1000);
  }
  
//  Serial.println(i++);
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
//  Serial.println("new client");
//  while(!client.available()){ delay(1); }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  if (req.indexOf("/gpio/0") != -1)
    val = 0;
  else if (req.indexOf("/gpio/1") != -1)
    val = 1;
  else if (req.indexOf("/d0/0") != -1)
    setFinger(PINKEY, "OUT");
  else if (req.indexOf("/d0/1") != -1)
    setFinger(PINKEY, "IN");
  else if (req.indexOf("/d1/0") != -1)
    setFinger(RING, "OUT");
  else if (req.indexOf("/d1/1") != -1)
    setFinger(RING, "IN");
  else if (req.indexOf("/d2/0") != -1)
    setFinger(MIDDLE, "OUT");
  else if (req.indexOf("/d2/1") != -1)
    setFinger(MIDDLE, "IN");
  else if (req.indexOf("/d3/0") != -1)
    setFinger(INDEX, "OUT");
  else if (req.indexOf("/d3/1") != -1)
    setFinger(INDEX, "IN");
  else if (req.indexOf("/d4/0") != -1)
    setFinger(THUMB, "OUT");
  else if (req.indexOf("/d4/1") != -1)
    setFinger(THUMB, "IN");
  else if (req.indexOf("/like/like") != -1) {
    setHand(0,0,0,0,1);
  }
  else if (req.indexOf("/spider/man") != -1) {
    setHand(1,0,0,1,1);
  }
  else if (req.indexOf("/kpo/ko") != -1) {
    setHand(0,0,0,0,0);
  }
  else if (req.indexOf("/kpo/papir") != -1) {
    setHand(1,1,1,1,1);
  }
  else if (req.indexOf("/kpo/ollo") != -1) {
    setHand(0,0,1,1,0);
  }
  else if (req.indexOf("/count/from1to5") != -1) {
    setHand(0,0,0,0,0);
    delay(1000);
    setFinger(THUMB, "OUT");
    delay(1000);
    setFinger(INDEX, "OUT");
    delay(1000);
    setFinger(MIDDLE, "OUT");
    delay(1000);
    setFinger(RING, "OUT");
    delay(1000);
    setFinger(PINKEY, "OUT");
  }
  else if (req.indexOf("/shoot/start") != -1) {
    shoot = 1;
    setHand(1,1,1,1,0);
  }
  else if (req.indexOf("/shoot/stop") != -1) {
    shoot = 0;
    setHand(1,1,1,1,1);
  }
  else if (req.indexOf("/binary/start") != -1) {
    binary = 1;
  }
  else if (req.indexOf("/binary/stop") != -1) {
    binary = 0;
  }
  else if (req.indexOf("/binary/reset") != -1) {
    binaryCnt = 0;
  }
  else if (req.indexOf("/wrist/w1v0") != -1)
    setWrist(WRIST1, 0);
  else if (req.indexOf("/wrist/w1v25") != -1)
    setWrist(WRIST1, 25);
  else if (req.indexOf("/wrist/w1v50") != -1)
    setWrist(WRIST1, 50);
  else if (req.indexOf("/wrist/w1v75") != -1)
    setWrist(WRIST1, 75);
  else if (req.indexOf("/wrist/w1v100") != -1)
    setWrist(WRIST1, 100);
  else if (req.indexOf("/wrist/w2v0") != -1)
    setWrist(WRIST2, 0);
  else if (req.indexOf("/wrist/w2v25") != -1)
    setWrist(WRIST2, 25);
  else if (req.indexOf("/wrist/w2v50") != -1)
    setWrist(WRIST2, 50);
  else if (req.indexOf("/wrist/w2v75") != -1)
    setWrist(WRIST2, 75);
  else if (req.indexOf("/wrist/w2v100") != -1)
    setWrist(WRIST2, 100);
  else {
//    Serial.println("invalid request");
    client.stop();
//    Serial.println("Client disonnected");
    return;
  }

  digitalWrite(myLED, val);
  
  client.flush();

//  String IP = "http://csurgay.no-ip.biz:1018";
  String IP = "http://"+ipToString(myIPaddress);
  
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
  s += "<!DOCTYPE html>\r\n";
  s += "<html><head><title><ESP8266 12F V.3 WebServer test</title></head><body><table width=100%>";
  s += "<tr><td>Counter is now ";
  s += String(counter++) + "</td></tr>";
  s += "<tr><td>LED is now ";
  s += (val)?"high":"low"; s+= "\n"; s+= "</td></tr>";
  s += "<tr><td>Sensor input:" + String(analogRead(sensor)) + "</td></tr>";
  s += "<tr><td>"+IP + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/gpio/0'>Led ON</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/gpio/1'>Led OFF</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/d0/0'>PinkeyOUT</a>";
  s += "     <a href='"+IP+"/d0/1'>PinkeyIN</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/d1/0'>RingOUT</a>";
  s += "     <a href='"+IP+"/d1/1'>RingIN</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/d2/0'>MiddleOUT</a>";
  s += "     <a href='"+IP+"/d2/1'>MiddleIN</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/d3/0'>IndexOUT</a>";
  s += "     <a href='"+IP+"/d3/1'>IndexIN</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/d4/0'>ThumbOUT</a>";
  s += "     <a href='"+IP+"/d4/1'>ThumbIN</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/kpo/ko'>-=KO=-</a>";
  s += "     <a href='"+IP+"/kpo/papir'>-=PAPIR=-</a>";
  s += "     <a href='"+IP+"/kpo/ollo'>-=OLLO=-</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/count/from1to5'>-=COUNT 1to5=-</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/shoot/start'>-=SHOOT START=-</a>";
  s += "     <a href='"+IP+"/shoot/stop'>-=STOP=-</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/like/like'>-=LIKE=-</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/spider/man'>-=SPIDERMAN=-</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/binary/start'>-=BINARY START=-</a>";
  s += "        <a href='"+IP+"/binary/stop'>-=STOP=-</a>";
  s += "        <a href='"+IP+"/binary/reset'>-=RESET=-</a>" + "</td></tr>";
  s += "<tr><td><a href='"+IP+"/wrist/w1v0'> W1-0 </a>";
  s += "        <a href='"+IP+"/wrist/w1v25'> W1-25 </a>";
  s += "        <a href='"+IP+"/wrist/w1v50'> W1-50 </a>";
  s += "        <a href='"+IP+"/wrist/w1v75'> W1-75 </a>";
  s += "        <a href='"+IP+"/wrist/w1v100'> W1-100 </a>";
  s += "        <a href='"+IP+"/wrist/w2v0'> W2-0 </a>";
  s += "        <a href='"+IP+"/wrist/w2v25'> W2-25 </a>";
  s += "        <a href='"+IP+"/wrist/w2v50'> W2-50 </a>";
  s += "        <a href='"+IP+"/wrist/w2v75'> W2-75 </a>";
  s += "        <a href='"+IP+"/wrist/w2v100'> W2-100 </a>" + "</td></tr>";
  s += "</table></body></html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  client.stop();
//  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}
