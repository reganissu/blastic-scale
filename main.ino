/*
  Simple POST client for ArduinoHttpClient library
  Connects to server once every five seconds, sends a POST request
  and a request body

  created 14 Feb 2016
  modified 22 Jan 2019
  by Tom Igoe
  
  this example is in the public domain
 */

///////////////////// NETWORKING Initialization /////////////////////////
#include <ArduinoHttpClient.h>
#include <WiFiS3.h>
///trying to improve networking
//#include "WiFiSSLClient.h"
//#include "IPAddress.h"
////// end improving mod

#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// WiFi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

char serverAddress[] = "docs.google.com";  // server address
int port = 80;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;

String YOURGoogleForm = "/forms/u/0/d/e/1FAIpQLSeI3jofIWqtWghblVPOTO1BtUbE8KmoJsGRJuRAu2ceEMIJFw/formResponse";
String URLstring = ""; 
///////////////////////END NETWORKING Initialization ///////////////////////////////////

////////////////////// BLASTIC DISPLAY Initialization //////////////////////////////////

///////////////////////
// To use ArduinoGraphics APIs, please include BEFORE Arduino_LED_Matrix

#include "ArduinoGraphics.h"

#include "Arduino_LED_Matrix.h"


ArduinoLEDMatrix matrix;
///////////////////////////////



#include "HX711.h"

#define DOUT  3
#define CLK  2

HX711 scale;

float calibration_factor = -14500; //-7050 worked for my 440lb max scale setup
///////////////// END BLASTIC DISPLAY initialization//////////////////////////////

void setup() {
  Serial.begin(9600);
 
 ///////////// Networking Setup ////////////////////

  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  /////////////END Networking Setup ////////////////////

  //////////// Scale and Display Setup /////////////////
  matrix.begin();
  
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  //////////// END Scale and Display Setup /////////////////




}

void loop() {

 ///////////////// Scale Display Loop /////////

 scale.set_scale(calibration_factor); //Adjust to this calibration factor

 // Make it scroll!

  matrix.beginDraw();
  matrix.stroke(0xFFFFFFFF);
  //matrix.textScrollSpeed(50);
  // add the text
 // mod  const char text[] = "    Nina & Giacomo    ";
  matrix.textFont(Font_4x6);
  matrix.beginText(0, 1, 0xFFFFFF);
  matrix.println(-scale.get_units(), 1);
  //matrix.endText(SCROLL_LEFT);
  matrix.endText();
  matrix.endDraw();

///////////////// END Scale Display Loop /////////

 // URLstring=(-scale.get_units(10)); Right for ESP8266 but Wrong for Arduino R4, use dtostrf Float to String conversion instead

 char ScaleUnits[4];
dtostrf(-scale.get_units(10),4,2,ScaleUnits);

URLstring = String(ScaleUnits);


//////////////  NETWORKING Loop //////////
  Serial.println("making POST request");
  String contentType = "application/x-www-form-urlencoded";
  String postData = ("entry.826036805=4+LDPE&entry.649832752=AndroProvo7&entry.458823532=Provo+Provo7&entry.1219969504=" + URLstring);

  client.post(YOURGoogleForm, contentType, postData);

  // read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);

  Serial.println("Wait 15 seconds");
  delay(15000);
}
