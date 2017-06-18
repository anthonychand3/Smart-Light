// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            2

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      10

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


#include <aJSON.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#define PubNub_BASE_CLIENT WiFiClient
#define PUBNUB_DEFINE_STRSPN_AND_STRNCASECMP 
#include <PubNub.h>


static char ssid[] = "Harrison";   // your network SSID (name)
static char pass[] = "chickencurry";  // your network password


const static char pubkey[] = "pub-c-6528095d-bc26-4768-a903-ac0a85174f81";         // your publish key 
const static char subkey[] = "sub-c-40e3d906-4ee7-11e7-bf50-02ee2ddab7fe";         // your subscribe key
const static char channel[] = "hello_world"; // channel to use




void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Serial set up");

  // attempt to connect using WPA2 encryption:
  Serial.println("Attempting to connect to WPA network...");
  WiFi.begin(ssid, pass);

  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop
  while (WiFi.status() != WL_CONNECTED)
  {
     delay(500);
     Serial.print(".");
  }
  //print a new line, then print WiFi connected and the IP address
  Serial.println("");
  Serial.println("WiFi connected");
  // Print the IP address
  Serial.println(WiFi.localIP());

  PubNub.begin(pubkey, subkey);
  Serial.println("PubNub set up");
  
  pixels.begin(); // This initializes the NeoPixel library.

}

void SetLed(int rgb[])
{
  for(int i=0;i<NUMPIXELS;i++)
  {

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(rgb[0],rgb[1],rgb[2])); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(delayval); // Delay for a period of time (in milliseconds).

  }
}



aJsonObject *createMessage()
{
  aJsonObject *msg = aJson.createObject();

  aJsonObject *sender = aJson.createObject();
  aJson.addStringToObject(sender, "name", "Arduino");
  aJson.addItemToObject(msg, "sender", sender);

  int analogValues[6];
  for (int i = 0; i < 6; i++) {
    analogValues[i] = analogRead(i);
  }
  aJsonObject *analog = aJson.createIntArray(analogValues, 6);
  aJson.addItemToObject(msg, "analog", analog);
  return msg;
}



//* Process message like: { "nameValuePairs": { "0": 245, "1": 245, "2": 220 } } */
void processPwmInfo(aJsonObject *item)
{
 aJsonObject *pwm = aJson.getObjectItem(item, "nameValuePairs");
  if (!pwm) {
    Serial.println("no pwm data");
    return;
  }

  const static int pins[] = { 0, 1, 2 };
  const static int pins_n = 3;
  int rgb[3];
  for (int i = 0; i < pins_n; i++) {
    char pinstr[4];
    snprintf(pinstr, sizeof(pinstr), "%d", pins[i]);

    aJsonObject *pwmval = aJson.getObjectItem(pwm, pinstr);
    if (!pwmval) continue; /* Value not provided, ok. */
    if (pwmval->type != aJson_Int) {
      Serial.print(" invalid data type ");
      Serial.print(pwmval->type, DEC);
      Serial.print(" for pin ");
      Serial.println(pins[i], DEC);
      continue;
    }

    Serial.print(" setting pin ");
    Serial.print(pins[i], DEC);
    Serial.print(" to value ");
    Serial.println(pwmval->valueint, DEC);
    rgb[i] = pwmval->valueint;
    Serial.println(rgb[i]);
    //analogWrite(pins[i], pwmval->valueint);
  }
  for(int i=0;i<NUMPIXELS;i++)
  {

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(rgb[0],rgb[1],rgb[2])); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    //delay(delayval); // Delay for a period of time (in milliseconds).

  }
}

void dumpMessage(Stream &s, aJsonObject *msg)
{
  int msg_count = aJson.getArraySize(msg);
  for (int i = 0; i < msg_count; i++)
  {
    aJsonObject *item, *sender, *analog, *value;
    s.print("Msg #");
    s.println(i, DEC);

    item = aJson.getArrayItem(msg, i);
    if (!item) { s.println("item not acquired"); delay(1000); return; }

    processPwmInfo(item);

  }
}


void loop()
{

  WiFiClient *client;
  aJsonObject *msg = createMessage();

    /* Subscribe and load reply */

  Serial.println("waiting for a message (subscribe)");
  client = PubNub.subscribe(channel);
  if (!client) 
  {
    Serial.println("subscription error");
    delay(1000);
    return;
  }

  /* Parse */

  aJsonClientStream stream(client);
  msg = aJson.parse(&stream);
  client->stop();
  if (!msg) { Serial.println("parse error"); delay(1000); return; }
  dumpMessage(Serial, msg);
  aJson.deleteItem(msg);

  delay(1000);

}
