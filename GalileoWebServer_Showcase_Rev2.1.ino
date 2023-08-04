/*
***************************************************************************
  Web Server for Intel IoT Greenhouse
 ***************************************************************************
 */

/*
***************************************************************************
Include statements for required libraries
***************************************************************************
*/
#include <SPI.h>
#include <Ethernet.h> // Enables internet access via built-in ethernet port
#include <Servo.h> // Enables servo controls
#include "LPD8806.h" // Enable LED controls

/*
***************************************************************************
Intialize global variables
***************************************************************************
*/

String version = "Rev2.1.2"; // Update version info to make sure the correct sketch is loaded
unsigned long currentTime = 0;

/*
***************************************************************************
Setup ethernet web server variables
***************************************************************************
*/

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0x98, 0x4F, 0xEE, 0x05, 0x62, 0x41 };
// IPAddress ip(10,34,50,155);
byte ip[] = { 10, 34, 50, 155 };
byte gateway[] = { 10, 34, 50, 1 };
byte subnet[] = { 255, 255, 255, 0 };

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

String controlString; // Captures out URI querystring

/*
***************************************************************************
Intialize LED variables
***************************************************************************
*/

// Number of RGB LEDs in strand:
int nLEDs = 26;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 12;
int clockPin = 13;

// First parameter is the number of LEDs in the strand.  The LED strips
// are 32 LEDs per meter but you can extend or cut the strip.  Next two
// parameters are SPI data and clock pins:
LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

int redLEDValue = 0; // Holds the Red LED value
int greenLEDValue = 64; // Holds the Green LED value
int blueLEDValue = 126; // Holds the Blue LED value
int redLEDUp = 0; // determines whether to increase or decrease the Red LED value
int greenLEDUp = 0; // determines whether to increase or decrease the Green LED value
int blueLEDUp = 0; // determines whether to increase or decrease the Blue LED value

unsigned long previousLEDUpdate = 1000; // Initial time for updating the LED values
unsigned long previousLEDRefresh = 1000; // Initial time for visually refreshing the LEDs

/*
***************************************************************************
Intialize servo variables
***************************************************************************
*/

// Initialize door & window position variables
int position_door = 0;
int position_left_window = 0;
int position_right_window = 0;

// Initialize door & window servos
Servo servo_door;
Servo servo_left_window;
Servo servo_right_window;

// Set PWM PINs for servo control
int servo_door_pin = 6;
int servo_left_window_pin = 3;
int servo_right_window_pin = 5;

unsigned long previousTimeDoor = 1000; // Initial time for door delay
unsigned long previousTimeLeftWindow = 1000; // Initial time for left window delay
unsigned long previousTimeRightWindow = 1000; // Initial time for right window delay

/*
***************************************************************************
Intialize relay variables
***************************************************************************
*/

byte relayPin[4] = {2,7,8,10}; // Set relay pin info
volatile byte relayState[4] = {LOW, LOW, LOW, LOW}; // Initialize relay state variables
//D2 -> RELAY1
//D7 -> RELAY2
//D8 -> RELAY3
//D10 -> RELAY4

unsigned long previousTimerelay1 = 1000; // Initial time for Relay 1
unsigned long previousTimerelay2 = 1000; // Initial time for Relay 2
unsigned long previousTimerelay3 = 1000; // Initial time for Relay 3
unsigned long previousTimerelay4 = 1000; // Initial time for Relay 4

/*
***************************************************************************
System setup
***************************************************************************
*/

void setup() {

    // Start up the LED strip
    strip.begin();

    // Update the strip, to start they are all 'off'
    strip.show();

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  //Start servo control
  servo_door.attach(servo_door_pin);
  servo_left_window.attach(servo_left_window_pin);
  servo_right_window.attach(servo_right_window_pin);

  // Set initial servo positions
  servo_door.write(135);
  servo_left_window.write(85);
  servo_right_window.write(110);

  // Set all relay pins to output
  for(int i = 0; i < 4; i++)  pinMode(relayPin[i],OUTPUT);

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

/*
***************************************************************************
Main loop
***************************************************************************
*/

void loop() {

  unsigned long currentTime = millis(); // Universal time reference
  // Serial.println( "Main Time: " + String(currentTime) );

  ledAttract(currentTime); // This function causes the LEDs to change color every 5 seconds

  // Check temperature
  // If high temp open windows and turn on fan
  // If low temp close windows and turn off fan
  tempCheckAutomation(currentTime);

  // Check pir motion sensor
  // If active open the door
  // If inactive close the door
  pirCheckAutomation(currentTime);

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        //read the HTTP request
        if (controlString.length() < 100) {
        // write characters to string
        controlString += c;
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          webContent(client); // This function provides the content sent back to a web request
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(5);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");

    manualControls(currentTime); // This function allows for manual control of servos and relays

    //clearing string for next read
    controlString="";
  }
}

/*
***************************************************************************
A Function for the Main Web Page
***************************************************************************
*/

void webContent(EthernetClient client) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  // add a meta refresh tag, so the browser pulls again every 5 seconds:
  client.println("<meta http-equiv=\"refresh\" content=\"5\">");
  client.println(version);
  client.println("<br />");
  client.println("<br />");
  client.println("<br />");
  client.println("<h1 style=\"color: Green; font-family: arial; text-align: center;\">");
  client.print("Intel IoT Greenhouse - REVIVED!");
  client.println("</h1>");
  client.println("<br />");
  int tempReading = analogRead(2);
  float tempValue = -0.00277358191992865 * pow(tempReading ,2) + 2.12540388128839 * tempReading + -183.108886107634;
  client.println("<br />");
  client.println("<h1 style=\"color: black; font-family: arial; text-align: center;\">");
  client.print("Temperature: ");
  if ( tempValue > 80 ) { client.print("<span style=\"color:red;\">"); }
  else { client.print("<span style=\"color:black;\">"); }
  client.println(tempValue);
  client.println("</span>");
  client.println("<br />");
  client.print("Motion: ");
  int pirReading = analogRead(3);
  if ( pirReading > 300 ) { client.print("<span style=\"color:red;\">"); client.println("activated"); }
  else { client.print("<span style=\"color:black;\">"); client.println("no motion"); }
  client.println("</span>");
  client.println("</h1>");
  client.println("<br />");
  client.println("<h2 style=\"color: blue; font-family: arial; text-align: center;\"><a href=\"/?RELAY1ON\"\">Mist Maker On</a> - <a href=\"/?RELAY1OFF\"\">Mist Maker Off</a></h2>");
  // client.println("<br />");
  client.println("<h2 style=\"color: blue; font-family: arial; text-align: center;\"><a href=\"/?WHITELED\"\">White LEDs</a> - <a href=\"/?REDLED\"\">Red LEDs</a> - <a href=\"/?GREENLED\"\">Green LEDs</a> - <a href=\"/?BLUELED\"\">Blue LEDs</a> - <a href=\"/?LEDOFF\"\">Turn off LEDs</a></h2>");
  client.println("</html>");
}

/*
***************************************************************************
A Function for Manual Controls for Servos and Relays
***************************************************************************
*/

void manualControls(unsigned long currentTime) {
    if(controlString.indexOf("?DOOR0") > -1) { close_door(currentTime); } //checks for DOOR0 and closes door
    if(controlString.indexOf("?DOOR90") > -1) { open_door(currentTime); } //checks for DOOR90 and opens door
    if(controlString.indexOf("?LEFTWINDOW0") > -1) { close_left_window(currentTime); } //checks for LEFTWINDOW0 and closes left window
    if(controlString.indexOf("?LEFTWINDOW45") > -1) { open_left_window(currentTime); } //checks for LEFTWINDOW45 and opens left window
    if(controlString.indexOf("?RIGHTWINDOW0") > -1) { close_right_window(currentTime); } //checks for RIGHTWINDOW0 and closes right window
    if(controlString.indexOf("?RIGHTWINDOW45") > -1) { open_right_window(currentTime); } //checks for RIGHTWINDOW45 and opens right window
    if(controlString.indexOf("?RELAY1ON") > -1) {
      if ( currentTime - previousTimerelay1 > 2000 ) {
        digitalWrite(relayPin[0],HIGH);
        relayState[0] = HIGH;
      }
    } //checks for RELAY1ON and turns relay 1 on
    if(controlString.indexOf("?RELAY1OFF") > -1) {
      if ( currentTime - previousTimerelay1 > 2000 ) {
        digitalWrite(relayPin[0],LOW);
        relayState[0] = LOW;
      }
    } //checks for RELAY1OFF and turns relay 1 off
    if(controlString.indexOf("?RELAY2ON") > -1) {
      if ( currentTime - previousTimerelay2 > 2000 ) {
        digitalWrite(relayPin[1],HIGH);
        relayState[1] = HIGH;
      }
    } //checks for RELAY2ON and turns relay 2 on
    if(controlString.indexOf("?RELAY2OFF") > -1) {
      if ( currentTime - previousTimerelay2 > 2000 ) {
        digitalWrite(relayPin[1],LOW);
        relayState[1] = LOW;
      }
    } //checks for RELAY2OFF and turns relay 2 off
    if(controlString.indexOf("?RELAY3ON") > -1) {
      if ( currentTime - previousTimerelay3 > 2000 ) {
        digitalWrite(relayPin[2],HIGH);
        relayState[2] = HIGH;
      }
    } //checks for RELAY3ON and turns relay 3 on
    if(controlString.indexOf("?RELAY3OFF") > -1) {
      if ( currentTime - previousTimerelay3 > 2000 ) {
        digitalWrite(relayPin[2],LOW);
        relayState[2] = LOW;
      }
    } //checks for RELAY3OFF and turns relay 3 off
    if(controlString.indexOf("?RELAY4ON") > -1) {
      if ( currentTime - previousTimerelay4 > 2000 ) {
        digitalWrite(relayPin[3],HIGH);
        relayState[3] = HIGH;
      }
    } //checks for RELAY4ON and turns relay 4 on
    if(controlString.indexOf("?RELAY4OFF") > -1) {
      if ( currentTime - previousTimerelay4 > 2000 ) {
        digitalWrite(relayPin[3],LOW);
        relayState[3] = LOW;
      }
    } //checks for RELAY4OFF and turns relay 4 off
    if(controlString.indexOf("?WHITELED") > -1) { colorWipe(strip.Color( 127, 127, 127), 20); Serial.println("White LED turned on."); } // White
    if(controlString.indexOf("?REDLED") > -1) { colorWipe(strip.Color( 127,   0,   0), 20); Serial.println("Red LED turned on."); } // Red
    if(controlString.indexOf("?BLUELED") > -1) { colorWipe(strip.Color(   0, 127,   0), 20); Serial.println("Blue LED turned on."); } // Blue
    if(controlString.indexOf("?GREENLED") > -1) { colorWipe(strip.Color(   0,   0, 127), 20); Serial.println("Green LED turned on."); } // Green
    if(controlString.indexOf("?LEDOFF") > -1) { colorWipe(strip.Color(   0,   0,   0), 20); Serial.println("LEDs turned off."); } // Off
}

/*
***************************************************************************
A Function to Control the Fan and Windows Based on Temperature
***************************************************************************
*/

void tempCheckAutomation(unsigned long currentTime) {
  int tempReading = analogRead(2);
  float tempValue = -0.00277358191992865 * pow(tempReading ,2) + 2.12540388128839 * tempReading + -183.108886107634;
  if ((tempValue > 85.00)  && ((relayState[3]) == LOW)) {
    if ( currentTime - previousTimerelay4 > 2000 ) {
      digitalWrite(relayPin[3],HIGH);
      relayState[3] = HIGH;
    }
  }
  else;
  if ((tempValue < 80.00)  && ((relayState[3]) == HIGH)) {
    if ( currentTime - previousTimerelay4 > 2000 ) {
      digitalWrite(relayPin[3],LOW);
      relayState[3] = LOW;
    }
  }
  if ((tempValue > 90.00) && (position_left_window < 15)) {
    open_left_window(currentTime);
    open_right_window(currentTime);
  }
  else;
  if ((tempValue < 85.00) && (position_left_window > 0)) {
    close_left_window(currentTime);
    close_right_window(currentTime);
  }
}

/*
***************************************************************************
A Function to Control the Door Based on Movement Detection
***************************************************************************
*/

void pirCheckAutomation(unsigned long currentTime) {
  int pirReading = analogRead(3);
  if ((pirReading < 300) && (position_door > 0)) { close_door(currentTime); }
  if ((pirReading > 300) && (position_door < 15)) { open_door(currentTime); }
}

/*
***************************************************************************
A Function to Open the Door
***************************************************************************
*/

void open_door(unsigned long currentTime) {
  if ( currentTime - previousTimeDoor > 2000 ) {
    position_door = 90;
    servo_door.write(45);
  }
}

/*
***************************************************************************
A Function to Close the Door
***************************************************************************
*/

void close_door(unsigned long currentTime) {
  if ( currentTime - previousTimeDoor > 2000 ) {
    position_door = 0;
    servo_door.write(135);
  }
}

/*
***************************************************************************
A Function to Open the Left Window
***************************************************************************
*/

void open_left_window(unsigned long currentTime) {
  if ( currentTime - previousTimeLeftWindow > 2000 ) {
    position_left_window = 45;
    servo_left_window.write(130);
  }
}

/*
***************************************************************************
A Function to Close the Left Window
***************************************************************************
*/

void close_left_window(unsigned long currentTime) {
  if ( currentTime - previousTimeLeftWindow > 2000 ) {
    position_left_window = 0;
    servo_left_window.write(85);
  }
}

/*
***************************************************************************
A Function to Open the Right Window
***************************************************************************
*/

void open_right_window(unsigned long currentTime) {
  if ( currentTime - previousTimeRightWindow > 2000 ) {
    position_right_window = 45;
    servo_right_window.write(65);
  }
}

/*
***************************************************************************
A Function to Close the Right Window
***************************************************************************
*/

void close_right_window(unsigned long currentTime) {
  if ( currentTime - previousTimeRightWindow > 2000 ) {
    position_right_window = 0;
    servo_right_window.write(110);
  }
}

/*****************************************************************************/
/*****************************************************************************/

/*
***************************************************************************
A Function to Change the LED Color Every 5 Seconds
***************************************************************************
*/

void ledAttract(unsigned long currentTime) {
  if ( (currentTime - previousLEDUpdate) > 200 ) {
    if ( redLEDValue < 1 ) { redLEDUp = 1; }
    if ( redLEDValue >= 126 ) { redLEDUp = 0; }
    if ( greenLEDValue < 1 ) { greenLEDUp = 1; }
    if ( greenLEDValue >= 126 ) { greenLEDUp = 0; }
    if ( blueLEDValue < 1 ) { blueLEDUp = 1; }
    if ( blueLEDValue >= 126 ) { blueLEDUp = 0; }

    if ( redLEDUp == 1 ) { redLEDValue = ++redLEDValue; }
    else { redLEDValue = --redLEDValue; }
    if ( greenLEDUp == 1 ) { greenLEDValue = ++greenLEDValue; }
    else { greenLEDValue = --greenLEDValue; }
    if ( blueLEDUp == 1 ) { blueLEDValue = ++blueLEDValue; }
    else { blueLEDValue = --blueLEDValue; }
    previousLEDUpdate = currentTime;
  }
  if ( (currentTime - previousLEDRefresh) > 5000 ) {
    colorWipe(strip.Color(   redLEDValue,   blueLEDValue,   greenLEDValue), 20);
    // Serial.println( "Time: " + String(currentTime) );
    // Serial.println( "Update: " + String(previousLEDUpdate) + " Refresh: " + String(previousLEDRefresh) );
    // Serial.println( "Red: " + String(redLEDValue) + " Green: " + String(greenLEDValue) + "Blue: " + String(blueLEDValue) );
    previousLEDRefresh = currentTime;
  }
}

void rainbow(uint8_t wait) {
  int i, j;
   
  for (j=0; j < 384; j++) {     // 3 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel( (i + j) % 384));
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Slightly different, this one makes the rainbow wheel equally distributed 
// along the chain
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;
  
  for (j=0; j < 384 * 5; j++) {     // 5 cycles of all 384 colors in the wheel
    for (i=0; i < strip.numPixels(); i++) {
      // tricky math! we use each pixel as a fraction of the full 384-color wheel
      // (thats the i / strip.numPixels() part)
      // Then add in j which makes the colors go around per pixel
      // the % 384 is to make the wheel cycle around
      strip.setPixelColor(i, Wheel( ((i * 384 / strip.numPixels()) + j) % 384) );
    }  
    strip.show();   // write all the pixels out
    delay(wait);
  }
}

// Fill the dots progressively along the strip.
void colorWipe(uint32_t c, uint8_t wait) {
  int i;

  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// Chase one dot down the full strip.
void colorChase(uint32_t c, uint8_t wait) {
  int i;

  // Start by turning all pixels off:
  for(i=0; i<strip.numPixels(); i++) strip.setPixelColor(i, 0);

  // Then display one pixel at a time:
  for(i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c); // Set new pixel 'on'
    strip.show();              // Refresh LED states
    strip.setPixelColor(i, 0); // Erase pixel, but don't refresh!
    delay(wait);
  }

  strip.show(); // Refresh to turn off last pixel
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();
     
      delay(wait);
     
      for (int i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 384; j++) {     // cycle all 384 colors in the wheel
    for (int q=0; q < 3; q++) {
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, Wheel( (i+j) % 384));    //turn every third pixel on
        }
        strip.show();
       
        delay(wait);
       
        for (int i=0; i < strip.numPixels(); i=i+3) {
          strip.setPixelColor(i+q, 0);        //turn every third pixel off
        }
    }
  }
}
/* Helper functions */

//Input a value 0 to 384 to get a color value.
//The colours are a transition r - g -b - back to r

uint32_t Wheel(uint16_t WheelPos)
{
  byte r, g, b;
  switch(WheelPos / 128)
  {
    case 0:
      r = 127 - WheelPos % 128;   //Red down
      g = WheelPos % 128;      // Green up
      b = 0;                  //blue off
      break; 
    case 1:
      g = 127 - WheelPos % 128;  //green down
      b = WheelPos % 128;      //blue up
      r = 0;                  //red off
      break; 
    default:
      b = 127 - WheelPos % 128;  //blue down 
      r = WheelPos % 128;      //red up
      g = 0;                  //green off
      break; 
  }
  return(strip.Color(r,g,b));
}
/*****************************************************************************/
/*****************************************************************************/
