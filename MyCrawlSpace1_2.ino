/*

 This sketch connects an DHT22 tempurature/humiditiy sensor  to Xively (http://www.xively.com)
 (the less expensive DHT11 does _not_ do <0C)
 using a Wiznet Ethernet shield. You can use the Arduino Ethernet shield, or the Adafruit Ethernet shield, either one will work, as long as it's got
 a Wiznet Ethernet module on board.
 
 Derived from; http://arduino.cc/en/Tutorial/XivelyClient
 
 Circuit:
 * one digital DHT22 sensor attached to digital in 2, a second one attached to pin 3
 * Ethernet shield attached to pins 10, 11, 12, 13
 
 Revision history:
 1.0 - ya! it works.
 1.1  cleaned up a bit. 25,984 bytes
 1.2  added  getSensorHT function getting T and H by reference. fixed time reporting. 23,000 bytes
 
 ToDo:
 use sleepmode instead of delay.  some ideas at  https://github.com/n0m1/Sleep_n0m1
 add telnet console maybe  http://arduino.cc/en/Reference/EthernetServer
 
 */
#include <SPI.h>
#include <Ethernet.h>
#include <dht.h>
#include <HttpClient.h>
#include <Xively.h>
#include "./XivelyDetails.h"

dht DHT;

//debuging flags
const boolean sendToXivey = true;
//delay between loops
const unsigned long postingInterval = 600000 ;   //600000 would result in a ten minute interval
//pins
const byte sensorApin =2;
const byte sensorBpin =3;
const int ledPin = 13;
int ledState = LOW;             // ledState used to set the LED

char buffer[14];  //buffer for float to string conversion
const String delim = "\t,";  //output delimiter

unsigned long currentTime  = 0; //this variable will be overwritten by millis() each iteration of loop
unsigned long pastTime     = 0; //no time has passed yet
unsigned long timePassed     = 0; //no time has passed yet

//xivey datastream definition
char myHStreamA[] = "SensorAHumidity";
char myTStreamA[] = "SensorATemperature";
char myHStreamB[] = "SensorBHumidity";
char myTStreamB[] = "SensorBTemperature";
const int bufferSize = 140;                   // size of the array
char bufferValue[bufferSize];                 // the array of chars itself
// Define the strings for our datastream IDs
XivelyDatastream datastreams[] = {
  XivelyDatastream(myHStreamA, strlen(myHStreamA), DATASTREAM_FLOAT),
  XivelyDatastream(myTStreamA, strlen(myTStreamA), DATASTREAM_FLOAT),
  XivelyDatastream(myHStreamB, strlen(myHStreamB), DATASTREAM_FLOAT),
  XivelyDatastream(myTStreamB, strlen(myTStreamB), DATASTREAM_FLOAT)
  };
  // Finally, wrap the datastreams into a feed
  XivelyFeed feed(FEED_ID, datastreams, 4 /* number of datastreams */);

// assign a MAC address for the ethernet controller.
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
EthernetClient client;
XivelyClient xivelyclient(client);
// fill in an available IP address on your network here in case dhcp fails
IPAddress defaultIP(10,0,1,20);
const char header[] = "Pin\t,DHTStatus\t,Humidity (%)\t,Temperature (C)\t,TimeSinceBoot (s)" ;

void setup() {
  //initPins
  pinMode(sensorApin, INPUT);
  pinMode(sensorBpin, INPUT);
  pinMode(ledPin, OUTPUT);  

  // Open serial communications and wait for port to open:
  Serial.begin(9600);    
  while (!Serial) {  
    ; // wait for serial port to connect. Needed for Leonardo only  
  }
  Serial.println("Setup Initializing...");
  // initLeds(); // set up the Status LEDs
  flashLED(5,500);
  // give the ethernet module time to boot up:
  Serial.print("Getting on the Ethernet...");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println( "Failed to configure Ethernet using DHCP.  Defaulting to IPAddress"  );
    Ethernet.begin(mac, defaultIP);    // DHCP failed, so use a fixed IP address:
  }
  else{
    Serial.print( "DHCP got "  ); 
    Serial.println(Ethernet.localIP());
  }
  flashLED(5,500);
  digitalWrite(ledPin, HIGH); 

  Serial.print("Results will be posted at https://xively.com/feeds/" );
  Serial.println( FEED_ID );

  //initDHT11
  Serial.print("DHT LIB version: ");
  Serial.println(DHT_LIB_VERSION);
  Serial.println("Setup all Done.");
} //end-of-setup


void loop() {  
  pastTime    = currentTime; //currentTime at this point is the current time from the previous iteration, this should now be pastTime
  currentTime = millis()/1000;    //currentTime is now the current time (again).  divide by 1000 becasue we dont need that kind of precision.
  timePassed = currentTime - pastTime; //this is roll-over proof, if currentTime is small, and pastTime large, the result rolls over to a small positive value, the time that has passed

  //  updateSensorState();
  Serial.println( header );
  int result = 0; 
  float t1 = 0 ; 
  float h1 = 0;

  result = getSensorHT(sensorApin, &h1, &t1);
  datastreams[0].setFloat(h1) ; 
  datastreams[1].setFloat(t1);
  Serial.println( sensorApin +  delim + result + delim + dtostrf( h1 ,5,2,buffer) + delim + dtostrf( t1 ,5,2,buffer) +delim+ currentTime );

  result = getSensorHT(sensorBpin, &h1, &t1);
  datastreams[2].setFloat( h1);
  datastreams[3].setFloat( t1);
  Serial.println( sensorBpin +  delim + result + delim + dtostrf( h1 ,5,2,buffer) + delim + dtostrf( t1 ,5,2,buffer) +delim+ currentTime );

  delay(1000); 

  //flash LED while the loop is going
  flashLED(20,100);

  if (sendToXivey) { //send value to xively
    Serial.print("Uploading it to Xively: ");
    int ret = xivelyclient.put(feed, xivelyKey);
    //return message
    Serial.print(" xivelyclient.put returned " );
    Serial.println(ret);
  }

  delay( postingInterval );
}
//---- End-of-loop


int getSensorHT( int pin, float* h, float* t)
{ //call like this; result = getSensorHT(sensorApin, &h1, &t1);
  //read from the designated sensor 
  int result = DHT.read22(pin);
  *h = (float)DHT.humidity,2;
  *t = (float)DHT.temperature,2;
  return result;
}


void flashLED( int maxflashes, int flashdelay) 
{
  int ledState = HIGH;
  digitalWrite(ledPin, ledState);
  for (int flashcount = 0; flashcount < maxflashes*2; flashcount++) { 
    digitalWrite(ledPin, !digitalRead(ledPin));
    digitalWrite(ledPin, ledState);
    delay(flashdelay);  
  }
}  


//
// END OF FILE
//




