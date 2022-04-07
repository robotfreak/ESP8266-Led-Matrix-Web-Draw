/*******************************************************************
    Basic Websocket Example

    Written by Brian Lough
    https://www.youtube.com/channel/UCezJOfu7OtqGzd5xrP3q6WA

    June 5th, 2018 - Added !rect, !line & !circle - Seon Rozenblum (Unexpected maker)
 *******************************************************************/

#include "secret.h"

#include <Arduino.h>

/* Configuration of NTP */
#define MY_NTP_SERVER "de.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   

#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <WebSocketsServer.h>
#include <Hash.h>


// ----------------------------
// Standard Libraries - Already Installed if you have ESP8266 set up
// ----------------------------
#include <time.h>                       // time() ctime()
#ifdef ESP8266
#include <sys/time.h>                   // struct timeval
#endif

#include <Ticker.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------
#include "CronAlarms.h"

CronId id;


#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

Ticker display_ticker;

//------- Replace the following! ------
char ssid[] = WIFI_NAME;       // your network SSID (name)
char password[] = WIFI_PASS;  // your network key

WebSocketsServer webSocket = WebSocketsServer(81);

/* Globals */
time_t now;                         // this is the epoch
tm tm;                              // the structure tm holds time information in a more convient way

void showTime() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  Serial.print(">year:");
  Serial.print(tm.tm_year + 1900);  // years since 1900
  Serial.print("\tmonth:");
  Serial.print(tm.tm_mon + 1);      // January = 0 (!)
  Serial.print("\tday:");
  Serial.print(tm.tm_mday);         // day of month
  Serial.print("\thour:");
  Serial.print(tm.tm_hour);         // hours since midnight  0-23
  Serial.print("\tmin:");
  Serial.print(tm.tm_min);          // minutes after the hour  0-59
  Serial.print("\tsec:");
  Serial.print(tm.tm_sec);          // seconds after the minute  0-61*
  Serial.print("\twday");
  Serial.print(tm.tm_wday);         // days since Sunday 0-6
  if (tm.tm_isdst == 1)             // Daylight Saving Time flag
    Serial.print("\tDST");
  else
    Serial.print("\tstandard");
  Serial.println();
}


void display_drawPixel(int x , int y, int colour) {
  unsigned char color;
  if (colour) color = 'Y'; else color = 'B'; 
  Serial.printf("S,%c,%d,%d\n",color,x,y);
}

void display_drawText(int x, int y, int colour, String fontStr, String text) {
  unsigned char color, font;
  if (colour) color = 'Y'; else color = 'B'; 
  if (fontStr == "16pt") font = 'X'; 
  else if (fontStr == "8pt") font = 'L';
  else if (fontStr == "5pt") font = 'S';
  
  Serial.printf("P,%c,%d,%d,%c,%s\n",color,x,y,font,text.c_str());
}

void clearDisplay(int colour) {
  if (colour) Serial.printf("C,Y,0,0\n");
  else Serial.printf("C,B,0,0\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  char xArray[4];
  int x;
  int y;
  int w;
  int h;
  uint16_t colour;
  int commaCount = 0;
  String inPayload;
  String colourString;
  String textString;
  String fontString;

  int commas[] = {-1,-1,-1,-1}; // using 4 for now
  int command;
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf(">[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf(">[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      // Serial.printf("[%u] get Text: %s\n", num, payload);
      inPayload = String((char *) payload);
      Serial.print(">");
      Serial.println(inPayload);

      if (inPayload == "ALLOFF") {
        clearDisplay(0);
      } else if(inPayload == "ALLON") {
        clearDisplay(1);
      }
      else {

        // clear commas
        // need to makr this use size of
        for ( int i = 0; i < ELEMENTS(commas); i++ )
          commas[i] = -1;

        // grab all comma positions
        int commaIndex = 0;
        for (int i = 0; i < inPayload.length(); i++ )
        {
          if ( inPayload[i] == ',' )
            commas[ commaIndex++ ] = i;
        }
        
        /* commands
        0 = draw
        1 = rect
        2 = line
        3 = cirle
        4 = text 
        */

        // grab command
        int commandSeperator = inPayload.indexOf(":");
        command = inPayload.substring(0,commandSeperator).toInt();
        //Serial.print(">");
        //Serial.println(command);
        
        x = inPayload.substring(commandSeperator+1, commas[0]).toInt();
        y = inPayload.substring(commas[0] + 1, commas[1]).toInt();

        //Serial.print(">");
        //Serial.print(x);
        //Serial.print(",");
        //Serial.println(y);
      
        if ( command == 0 ) // draw pixel
        {
          colourString = inPayload.substring(commas[1] + 1);
          //Serial.print(">");
          //Serial.println(colourString);
          colour = strtol(colourString.c_str(), NULL, 0);
          //Serial.print(">");
          //Serial.println(colour);
          display_drawPixel(x , y, colour);
        }
        else if ( command == 1 ) // rect
        {
          w = inPayload.substring(commas[1] + 1, commas[2]).toInt();
          h = inPayload.substring(commas[2] + 1, commas[3]).toInt();
          colourString = inPayload.substring(commas[3] + 1);
          colour = strtol(colourString.c_str(), NULL, 0);

          //@@ display.drawRect( x, y, w, h, colour );

        }
        else if ( command == 2 ) // line
        {
          w = inPayload.substring(commas[1] + 1, commas[2]).toInt();
          h = inPayload.substring(commas[2] + 1, commas[3]).toInt();
          colourString = inPayload.substring(commas[3] + 1);
          colour = strtol(colourString.c_str(), NULL, 0);

          //@@ display.drawLine( x, y, w, h, colour );
        }
        else if ( command == 3 ) // circle
        {
          w = inPayload.substring(commas[1] + 1, commas[2]).toInt();
          colourString = inPayload.substring(commas[2] + 1);
          colour = strtol(colourString.c_str(), NULL, 0);

          //@@ display.drawCircle( x, y, w, colour );
        }
        else if ( command == 4 ) // text
        {
          colourString = inPayload.substring(commas[1] + 1);
          //Serial.print(">");
          //Serial.println(colourString);
          colour = strtol(colourString.c_str(), NULL, 0);
          //Serial.print(">");
          //Serial.println(colour);
          fontString = inPayload.substring(commas[2] + 1);
          textString = inPayload.substring(commas[3] + 1);
          display_drawText(x, y, colour, fontString, textString);
        }


        


        // Serial.print("X: ");
        // Serial.println(x);
        // Serial.print("Y: ");
        // Serial.println(y);
        // Serial.print("Colour: ");
        // Serial.println(colour, HEX);


      }


      // send message to client
      // webSocket.sendTXT(num, "message here");

      // send data to all connected clients
      // webSocket.broadcastTXT("message here");
      break;
    case WStype_BIN:
      Serial.printf(">[%u] get binary length: %u\n", num, length);
      hexdump(payload, length);

      // send message to client
      // webSocket.sendBIN(num, payload, length);
      break;
  }

}

int state = 0;

void showNews() {
  showTime();  
  //time_t tnow = time(nullptr);
  //Serial.print(">");
  //Serial.println(asctime(gmtime(&tnow)));
  Serial.println(">5 minute timer");
  Serial.printf("n,Y,%d,0\n", state);
  state++;
  if (state >= 4) state = 0;
}


/*@@
void display_updater()
{

  display.display(70);

}

void clearDisplay(){
  for(int i = 0; i < 64; i++){
    for(int j = 0; j < 32; j++){
      display.drawPixel(i , j, 0x0000);
    }
  }
}
*/
void setup() {

  Serial.begin(9600);

  configTime(MY_TZ, MY_NTP_SERVER); // set time zone NTP server
  //@@ display.begin(16);
  //@@ display.clearDisplay();

  //@@ display_ticker.attach(0.002, display_updater);
  yield();

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print(">Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(">");
  Serial.println(">WiFi connected");
  Serial.println(">IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.print(">");
  Serial.println(ip);

  Cron.create(" 0 */5 18-23 * * *", showNews, false);           // timer for every 5 minutes (18-23Uhr)

  showTime();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  //Serial.print(">");
  //Serial.println(asctime(gmtime(&tnow)));
  Cron.delay();// if the loop has nothing else to do, delay in ms

  webSocket.loop();

}
