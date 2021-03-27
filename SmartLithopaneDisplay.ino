/*********
  Mijaz Mukundan

  Built on the excellent tutorial at

  1) For webserver: https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <WiFiUdp.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <NTPClient.h> //https://github.com/taranais/NTPClient
#include <Hash.h>
#include <FS.h>
#include <ESPAsyncWebServer.h>

//neopixel helper library
#include "neopixel.h"
//WiFi Configuration
#include "wifi_config.h"

String DateToRemember = "03-07"; //The date you want to set the pattern on in mm-dd format, it can be a birthday, aniversary etc.



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The following code sets up the web server for seleting animation, color and interval using inputs from a web page

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String default_display_color_1, default_display_color_2;
String default_animation;
int default_animation_frequency;

AsyncWebServer server(80);

const char *PARAM_DEFAULT_DISPLAY_COLOR_1 = "default_display_color_1";
const char *PARAM_DEFAULT_DISPLAY_COLOR_2 = "default_display_color_2";
const char *PARAM_DEFAULT_ANIMATION = "default_animation";
const char *PARAM_DEFAULT_ANIMATION_FREQUENCY = "default_frequency";

// HTML web page to handle 4 input fields (Color1, Color2, Animation, Animation Frequency)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Smart Lithopane Display</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>

  <h1>Smart Lithopane Display</h1>
  <h3>Default setup:</h3>
  <form action="/getdefault" target="hidden-form">
  Select Display Color 1:<input name="default_display_color_1" type="color" value="#ffffff" /><br /><br />
  Select Display Color 2:<input name="default_display_color_2" type="color" value="#000000" /><br /><br />
  Select Animation Style:<select name="default_animation">
  <option value="STATIC">Static</option>
  <option value="RAINBOW_CYCLE">Rainbow Cycle</option>
  <option value="THEATER_CHASE">Theatre Chase</option>
  <option value="COLOR_WIPE">Colour Wipe</option>
  <option value="SCANNER">Scanner</option>
  <option value="FADE">Fade</option>
  </select><br /><br />
  Cycle Frequency (between 1 and 10000 ms):<input max="10000" min="1" name="default_frequency" type="number" /> <br /><br />
  <input type="submit" value="Set Default" onclick="submitMessage()"/></form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

//Function to read file from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

//Function to write a file to SPIFFS
void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
}

// Replaces placeholder with stored values
String processor(const String &var)
{
  //Serial.println(var);
  if (var == "default_display_color_1")
  {
    return readFile(SPIFFS, "/default_display_color_1.txt");
  }
  else if (var == "default_display_color_2")
  {
    return readFile(SPIFFS, "/default_display_color_2.txt");
  }
  else if (var == "default_animation")
  {
    return readFile(SPIFFS, "/default_animation.txt");
  }
  else if (var == "default_animation_frequency")
  {
    return readFile(SPIFFS, "/default_frequency.txt");
  }
  return String();
}

// Date and time
WiFiUDP ntpUDP;

//Adjusted for Indian Standard Time @ +5:30 UTC
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);
String date_time;
String mmdd;
unsigned long datetimeUpdateInterval = 3 * 60 * 60 * 1000; //3hr interval
unsigned long lastDatetimeUpdate;


//Neopixel setup
//Defining Neopatterns for our ring
// GPIO 0 of ESP01 will be connected to DIN of the led ring

#define PIN 0       //GPIO 0 of ESP01
#define NUMPIXELS 8 //The number of LEDs on the neopixel module

// This is the function called on completion of one iteration of the animation
void onDisplayRingComplete();

// Here we create a NeoPattern object
NeoPatterns displayRing(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800, &onDisplayRingComplete);

void setup()
{
  Serial.begin(115200);
  // Initialize SPIFFS

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Set your Static IP address
  IPAddress local_IP(192, 168, 0, 109);
  // Set your Gateway IP address
  IPAddress gateway(192, 168, 0, 1);

  IPAddress subnet(255, 255, 255, 0);
  IPAddress primaryDNS(8, 8, 8, 8);   // optional
  IPAddress secondaryDNS(8, 8, 4, 4); // optional

  // WiFi mode station (connect to wifi router only
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);

  WiFi.begin(ssid_1, password_1);
  // Configures static IP address

  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("WiFi Failed!");
    return;
  }

  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID()); // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer

  if (!MDNS.begin("smartlithopanedisplay"))
  { // Start the mDNS responder for smartlithopanedisplay.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/getdefault", HTTP_GET, [](AsyncWebServerRequest *request) {
    String inputMessage;

    inputMessage = "No message sent";

    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(PARAM_DEFAULT_DISPLAY_COLOR_1))
    {
      inputMessage = request->getParam(PARAM_DEFAULT_DISPLAY_COLOR_1)->value();
      writeFile(SPIFFS, "/default_display_color_1.txt", inputMessage.c_str());
    }

    if (request->hasParam(PARAM_DEFAULT_DISPLAY_COLOR_2))
    {
      inputMessage = request->getParam(PARAM_DEFAULT_DISPLAY_COLOR_2)->value();
      writeFile(SPIFFS, "/default_display_color_2.txt", inputMessage.c_str());
    }

    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    if (request->hasParam(PARAM_DEFAULT_ANIMATION))
    {
      inputMessage = request->getParam(PARAM_DEFAULT_ANIMATION)->value();
      writeFile(SPIFFS, "/default_animation.txt", inputMessage.c_str());
    }

    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    if (request->hasParam(PARAM_DEFAULT_ANIMATION_FREQUENCY))
    {
      inputMessage = request->getParam(PARAM_DEFAULT_ANIMATION_FREQUENCY)->value();
      writeFile(SPIFFS, "/default_frequency.txt", inputMessage.c_str());
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });

  server.onNotFound(notFound);

  server.begin();

  displayRing.begin();

  //setting up date fetch
  timeClient.begin();

  timeClient.update();
  lastDatetimeUpdate = millis();

  date_time = timeClient.getFormattedDate();

  mmdd = date_time.substring(5, 10); //get the month and day

  Serial.println(mmdd);

  // Setup your animation style if the date matches, you can add more dates to remember
  if (mmdd == DateToRemember)
  {

    displayRing.ActivePattern = FADE;
    displayRing.Interval = 10;
    displayRing.Color1 = getColorFromHexString("#ffb6c1"); //Light Pink
    displayRing.Color2 = getColorFromHexString("#ff69b4"); //Hot Pink
    displayRing.TotalSteps = setsteps(default_animation,NUMPIXELS);
  }
  else
  {
    Serial.println("On display Ring Complete Called");
    default_display_color_1 = readFile(SPIFFS, "/default_display_color_1.txt");
    default_display_color_2 = readFile(SPIFFS, "/default_display_color_2.txt");
    default_animation = readFile(SPIFFS, "/default_animation.txt");
    default_animation_frequency = readFile(SPIFFS, "/default_frequency.txt").toInt();

    if (!default_animation)
    {
      default_animation = "STATIC";
    }
    if (!default_display_color_1)
    {
      default_display_color_1 = "#ffffff"; //White
    }
    if (!default_display_color_2)
    {
      default_display_color_2 = "#000000"; //Black
    }
    if (!default_animation_frequency)
    {
      default_animation_frequency = 1;
    }

    displayRing.ActivePattern = (pattern)setpattern(default_animation);
    displayRing.Interval = default_animation_frequency;
    displayRing.Color1 = getColorFromHexString(default_display_color_1);
    displayRing.Color2 = getColorFromHexString(default_display_color_2);
    displayRing.TotalSteps = setsteps(default_animation,NUMPIXELS);
  }
}

void loop()
{

  displayRing.Update();
}

void onDisplayRingComplete()
{
  Serial.println("ondisplayRingComplete() Called");

  if ((millis() - lastDatetimeUpdate) > datetimeUpdateInterval) // time to update
  {
    lastDatetimeUpdate = millis();
    //setting up date fetch
    timeClient.update();

    date_time = timeClient.getFormattedDate();

    mmdd = date_time.substring(5, 10); //get the month and day

    Serial.println("Time Update Complete");
  }

  // Setup your animation style if the date matches, you can add more dates to remember by an if else ladder
  if (mmdd == DateToRemember)
  {

    displayRing.ActivePattern = FADE;
    displayRing.Interval = 10;
    displayRing.Color1 = getColorFromHexString("#ffb6c1"); //Light Pink
    displayRing.Color2 = getColorFromHexString("#ff69b4"); //Hot Pink
    displayRing.TotalSteps = setsteps(default_animation, NUMPIXELS);
  }
  else
  {
    default_display_color_1 = readFile(SPIFFS, "/default_display_color_1.txt");
    default_display_color_2 = readFile(SPIFFS, "/default_display_color_2.txt");
    default_animation = readFile(SPIFFS, "/default_animation.txt");
    default_animation_frequency = readFile(SPIFFS, "/default_frequency.txt").toInt();

    if (!default_animation)
    {
      default_animation = "STATIC";
    }
    if (!default_display_color_1)
    {
      default_display_color_1 = "#ffffff";
    }
    if (!default_display_color_2)
    {
      default_display_color_2 = "#000000";
    }
    if (!default_animation_frequency)
    {
      default_animation_frequency = 1;
    }

    displayRing.ActivePattern = (pattern)setpattern(default_animation);
    displayRing.Interval = default_animation_frequency;
    displayRing.Color1 = getColorFromHexString(default_display_color_1);
    displayRing.Color2 = getColorFromHexString(default_display_color_2);
    displayRing.TotalSteps = setsteps(default_animation, NUMPIXELS);
  }
}
