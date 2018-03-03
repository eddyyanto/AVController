/*
  Project      : AV & Dataton WatchOut Controller
  Author       : Eddy Yanto
  Created      : 28 November 2012
  Description  : A system interface for an iOS device and button control panel
                 to control amplifier (using IR LED), computers (using relay), 
                 WatchOut cluster and projectors (using TCP sockets)
  Change notes :
  - 2015-02-10 : Changed IR interace to ethernet control for Marantz amplifier for better reliability
  - 2015-03-11 : Added another physical button for trigger Duo show
 */

#include <SPI.h>
#include <Ethernet.h>
#include <IRremote.h>

// Change to 0 to enable logging on serial monitor
int enableLog = 0;

// Keep track of uptime
long currentMillis = 0;

// Local ethernet IP and mac setting
byte localMac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
byte localIp[] = { 192, 168, 1, 48 };

// Local server: used to receive commands from iOS device
EthernetServer localServer = EthernetServer(3038);
EthernetClient connectedClient;

// Watchout cluster master
byte watchOutServer[] = { 192, 168, 1, 51 };
const int watchOutPort = 3039;
EthernetClient sharedClient;

// Projectors list
// Note: projectors and WatchOut share the same EthernetClient (sharedClient)
byte projectors[][4] = {
  { 192, 168, 1, 1 },
  { 192, 168, 1, 2 },
  { 192, 168, 1, 3 },
  { 192, 168, 1, 4 },
  { 192, 168, 1, 5 },
  { 192, 168, 1, 6 },
  { 192, 168, 1, 7 },
  { 192, 168, 1, 8 }
};
const int projectorPort = 4352;

// Amplifier
IRsend irsend;
byte marantzAmplifier[] = { 192, 168, 1, 10 };
const int marantzAmplifierPort = 23;

// Keep track of computers and amplifier status
int isDevicesOn = 0;

void setup()
{
  Serial.begin(9600);

  // Relay
  pinMode(2, OUTPUT);

  // IR led
  // defaults to pin 3

  // 4 buttons
  pinMode(4, INPUT);   // Ophir Duo show — button
  pinMode(5, OUTPUT);  // Ophir Duo show — led
  pinMode(6, INPUT);   // System OFF — button
  pinMode(7, OUTPUT);  // System OFF — led
  pinMode(8, INPUT);   // Marina One show — button
  pinMode(9, OUTPUT);  // Marina One show — led
  pinMode(A2, INPUT);  // System ON — button
  pinMode(A3, OUTPUT); // System ON — led

  Ethernet.begin(localMac, localIp);  

  localServer.begin();
  _log("Control started.");
}

void loop()
{
  // If iOS device connects, read the command and trigger
  connectedClient = localServer.available();
  if (connectedClient == true) {
    char incoming = connectedClient.read();

    _log("Connected client: " + connectedClient);

    switch(incoming){
    case 'A': // Turn OFF system
      _log("Received A: Turn OFF.");
      turnOffAll();
      delay(100);
      break;

    case 'B': // Turn ON system
      _log("Received B: Turn ON.");
      turnOnAll();
      delay(100);
      break;

    case 'C':
      _log("Received C: Trigger Marina One show.");
      triggerOneShow();
      delay(100);

    case 'd':
      _log("Received D: Trigger Ophir Duo Show.");
      triggerDuoShow();

    default:
      break;
    }
    // Close connected client so that they don't build up (only 4 max concurrent connection is allowed by Ethernet Shield)
    // connectedClient.stop(); // Taken care on iOS side
  }

  // ON Button (GREEN)
  if(digitalRead(A2) == HIGH){
    digitalWrite(A3, HIGH);
    turnOnAll();
    delay(100);
  }
  else{
    digitalWrite(A3, LOW); 
  }

  // OFF button (RED)
  if(digitalRead(6) == HIGH){
    digitalWrite(7, HIGH);
    turnOffAll();
    delay(100);
  }
  else{
    digitalWrite(7, LOW); 
  }

  // PLAY Button (BLUE)
  if(digitalRead(8) == HIGH){
    digitalWrite(9, HIGH);
    _log("Marina One button triggered.");
    triggerOneShow();
    delay(5000);
  }
  else{
    digitalWrite(9, LOW); 
  }

  // Duo Button (BLUE)
  if(digitalRead(4) == HIGH){
    digitalWrite(5, HIGH);
    _log("Ophir Duo button triggered.");
    triggerDuoShow();
    delay(5000);
  }
  else{
    digitalWrite(5, LOW); 
  }
}

void triggerOneShow(){
  EthernetClient externalClient;
  if(sharedClient.connect(watchOutServer, watchOutPort)){
    delay(100);
    sharedClient.flush();
    sharedClient.println("\r");
    sharedClient.println("authenticate 1\r\n");
    delay(100);
    sharedClient.println("gotoControlCue \"one\"\r\n");
    sharedClient.println("run\r\n");
    delay(100);
    _log("WatchOut command sent"); 
  }
  else{
    _log("WatchOut connection failed");
  }
  sharedClient.flush();
  sharedClient.stop();
}

void triggerDuoShow(){
  EthernetClient externalClient;
  if(sharedClient.connect(watchOutServer, watchOutPort)){
    delay(100);
    sharedClient.flush();
    sharedClient.println("\r");
    sharedClient.println("authenticate 1\r\n");
    delay(100);
    sharedClient.println("gotoControlCue \"duo\"\r\n");
    sharedClient.println("run\r\n");
    delay(100);
    _log("WatchOut command sent"); 
  }
  else{
    _log("WatchOut connection failed");
  }
  sharedClient.flush();
  sharedClient.stop();
}

void turnOnAll(){
  isDevicesOn = 1;
  turnOnProjectors();
  turnOnAmplifier();
}

void turnOffAll(){
  isDevicesOn = 0;
  turnOffProjectors();
  turnOffAmplifier();
}

// This function is phased out and replaced with
// turnOnAmplifier and turnOffAmplifier which uses Ethernet control
//void turnOnOffAmplifier(){
//  irsend.sendRC5(0x40C, 12); //amplifier
//  irsend.sendRC5(0x40C, 12); //amplifier
//}

void turnOnOffComputer(){
  digitalWrite(2, HIGH);
  delay(500); // Adjust accordingly since we're simulating power button push
  digitalWrite(2, LOW);
}

void turnOnAmplifier(){
  _log("Turning ON Amplifier started.");
  if(sharedClient.connect(marantzAmplifier, marantzAmplifierPort)){
    delay(200);
    sharedClient.println("SIDVD");
    sharedClient.stop();
    _log(" success");
  }
  else{
    _log(" failed");
    sharedClient.flush();
    sharedClient.stop();
  }
  _log("Turning ON amplifier ended.");
}

void turnOffAmplifier(){
  _log("Turning OFF Amplifier started.");
  if(sharedClient.connect(marantzAmplifier, marantzAmplifierPort)){
    delay(200);
    sharedClient.println("PWSTANDBY");
    delay(1000);
    sharedClient.stop();
    delay(1000);
    _log(" success");
  }
  else{
    _log(" failed");
    sharedClient.flush();
    sharedClient.stop();
  }
  _log("Turning OFF amplifier ended.");
}

void turnOnProjectors(){
  _log("Turning ON projectors started.");
  for(int i = 0; i < 8; i++){
    if(sharedClient.connect(projectors[i], projectorPort)){
      delay(200);
      sharedClient.println("%1POWR 1\r");
      delay(1000);
      sharedClient.stop();
      delay(1000);
      _log(String(i) + " success");
    }
    else{
      _log(String(i) + " failed");
      sharedClient.flush();
      sharedClient.stop();
    }
  }
  _log("Turning ON projectors ended.");
}

void turnOffProjectors(){
  _log("Turning OFF projectors started.");
  for(int i = 0; i < 8; i++){
    if(sharedClient.connect(projectors[i], projectorPort)){
      delay(200);
      sharedClient.println("%1POWR 0\r");
      delay(1000);
      sharedClient.flush();
      sharedClient.stop();
      delay(1000);
      _log(String(i) + " success");
    }
    else{
      _log(String(i) + " failed");
      sharedClient.flush();
      sharedClient.stop();
    }
  }
  _log("Turning OFF projectors ended.");
}

void _log(String message){
  if(enableLog){
    Serial.println(message);
  }
}

void serialEvent() {
  while (Serial.available()) {
    enableLog = 1;
    Serial.println("Serial logging enabled.");
    Serial.read();
  }
}