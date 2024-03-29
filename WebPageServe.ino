
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

// see C:\Program Files (x86)\Arduino\hardware\tools\avr\avr\include\avr for avr/interrupt.h
#include <MsTimer2.h>

//#define DEBUG_SEND

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "martel";
const char WiFiPSK[] = "mpahrilleinpe";

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read

// globals for LED and blinker state
int led = 0;
int blinker = 0;

MSTimer2 tim();

// Basic Web page
const char Head[] = (\
"HTTP/1.1 200 OK\r\n"\
"Content-Type: text/html\r\n\r\n"\
"<!DOCTYPE HTML>\r\n<html><head><title>\r\n"\
"Thing Dev Board Web Page</title></head><body>\r\n"\;
// use anchors instead of forms for this version
// buttons inside anchors look good
const char Bod[] =("\
<br />\
&nbsp;&nbsp;&nbsp;<a href=\"./?LED=1\"><button>Blue LED On</button></a><br /><br />\
&nbsp;&nbsp;&nbsp;<a href=\"./?LED=0\"><button>Blue LED Off</button></a><br /><br />\
");


const char Tail[] = "</body></html>\r\n";

WiFiServer server(80);

void setup() 
{
  initHardware();
  connectWiFi();
  server.begin();
  setupMDNS();
  Serial.println();
}

void loop() 
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Read the first line of the request
  Serial.println("Thing got request:");
  String req = client.readStringUntil('\r');
  Serial.print("<<<");
  Serial.print(req);
  Serial.println(">>>");
/* Other stuff thea the browser sends not relevant to what I'm doing  
  String rest = client.readString();
  Serial.println(rest);
 */
  client.flush();

  // Match the request
  if( req.indexOf("flavicon.ico") != -1) {
    // some kind of request for an icon to display on the browser tab?  Punt.
    Serial.println("favicon request");
    delay(1);
    Serial.println("Client disonnected");
    return;
  }
  if ( req.length() == 0 ) {
    Serial.println("empty request");
    delay(1);
    Serial.println("Client disonnected");
    return;
  }

  int index;
  String q = "?LED=";
  if ( (index = req.indexOf(q)) != -1 ){
    // an LED command
    char c = req.charAt( index + q.length());
    Serial.print( "LED command. ");
    Serial.println(req.substring(index+1));
    switch ( c ){
      case '0':
        led = 0;
        blinker = 0;
        digitalWrite(LED_PIN, 1-led);
        break; 
      case '1':
        led = 1;
        blinker = 0;
        digitalWrite(LED_PIN, 1-led);
        break;
      case 'B':
        if ( blinker ){
          blinker = 0;
          stopBlinker();
        }else{
          blinker = 1;
          startBlinker();         
        }
        break; 
      default:
        break; 
    }
    delay(1);
    Serial.println("Client disonnected");
  }
  /*
  int val = -1; // We'll use 'val' to keep track of both the
                // request type (read/set) and value if set.
  if (req.indexOf("/led/0") != -1)
    val = 1; // Will write LED high
  else if (req.indexOf("/led/1") != -1)
    val = 0; // Will write LED low
  else if (req.indexOf("/read") != -1)
    val = -2; // Will print pin reads
  // Otherwise request will be invalid. We'll say as much in HTML

  // Set GPIO5 according to the request
  if (val >= 0)
    digitalWrite(LED_PIN, val);
  */
  // Prepare the response. Start with the common header:
  //String s = "HTTP/1.1 200 OK\r\n";
  String s = "";
  s += Head;
  s += Bod;
  if ( blinker ){
    s += "&nbsp;&nbsp;&nbsp;<a href=\"./?LED=B\"><button>Blue LED Blink Off</button></a><br /><br />";
  }else{
    s += "&nbsp;&nbsp;&nbsp;<a href=\"./?LED=B\"><button>Blue LED Blink On</button></a><br /><br />";
  }
  s += Tail;

  // report response
#ifdef DEBUG_SEND
  Serial.println("\r\nSending <<<");
  Serial.println(s);
  Serial.println(">>> to web client\r\n");
#endif  
  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is destroyed
}

void connectWiFi()
{
  byte ledStatus = LOW;
  Serial.println();
  Serial.println("Connecting to: " + String(WiFiSSID));
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void initHardware()
{
  Serial.begin(115200);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  // Don't need to set ANALOG_PIN as input, 
  // that's all it can be.
  tim.set(1000L, toggleLED );
}

void setupMDNS()
{
  // Call MDNS.begin(<domain>) to set up mDNS to point to
  // "<domain>.local"
  if (!MDNS.begin("thing")) 
  {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

}

void startBlinker(){
  tim.start();
}

void stopBlinker(){
  tim.stop(); 
}

void toggleLED(){
  led = 1-led;
  digitalWrite(LED_PIN, 1-led);
}

