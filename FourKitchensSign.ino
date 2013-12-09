#include <Adafruit_NeoPixel.h>
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(40, PIN, NEO_GRB + NEO_KHZ800);

// Lets set up some colors, and a global color place holder.
uint32_t white = strip.Color(255, 255, 255);
uint32_t theGreen = strip.Color(70, 255, 20);
uint32_t currentColor = strip.Color(0, 0, 0);

// General Variables.
uint32_t startBrightness = 50;
uint32_t currentBrightness = 0;
long previousMillis = 0;
long changeInterval = 200;

// State Machie
enum operatingState { NORMAL = 0, COLOR};
operatingState opState = NORMAL;

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

void setup() {

  //Serial and Bridge Startup
  Serial.begin(9600);

  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  if (client) {
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  // State Machine.
  switch (opState) {
  case NORMAL:
    // Breathing animation.
    breath(5);
    break;
  case COLOR:
    steadyColor(currentColor);
    break;
  }

  // Poll every 50ms
  delay(50); 
}

// process incoming commands.
void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');

  client.println(command);
  // is "rainbow" command?
  if (command == "rainbow") {
    rainbowCommand(client);
  }
  else if (command == "sparkle") {
    sparkleCommand(client);
  }
  else if (command == "color") {
    colorCommand(client);
  }
  else if (command == "normal") {
    opState = NORMAL;
    client.println(F("Back to normal"));
  }
  else {
    client.println(F("Invalid Command."));
  }
}

void rainbowCommand(YunClient client) {
  int delay;

  delay = client.parseInt();
  rainbow(delay);
  client.print(F("Delay: "));
  client.print(delay);
  client.print(F(". All the way Double rainbow!"));

}

void sparkleCommand(YunClient client) {
  int color, repeats, minLeds, maxLeds;
  
  repeats = client.parseInt();
  if  (client.read() == '/') {
    minLeds = client.parseInt();
    client.read();
    maxLeds = client.parseInt();
    client.read();
    color = client.parseInt();
  }
  else {
    minLeds = 1;
    maxLeds = 20;
  }
 
  sparkle(color, repeats, minLeds, maxLeds);
  
  client.print(F("Sparkled: "));
  client.print(repeats);
  client.println(F(" times."));
  client.print(F("At least: "));
  client.print(minLeds);
  client.print(F(" but no more than: "));
  client.print(maxLeds);
  client.println(F(" leds at the same time."));
  client.println(color);
}

void colorCommand(YunClient client) {
  int r = 0, g = 0, b = 0;

  r = client.parseInt();
  if (client.read() == '/') {
    g = client.parseInt();
    client.read();
    b = client.parseInt();
  }
  currentColor = strip.Color(r, g, b);
  steadyColor(currentColor);
  client.print(F("Current color is: "));
  client.println(currentColor);
  opState = COLOR;
}


// Animation that "sparkles" Random LEDs.
void sparkle(int color, int repeats, int minLeds, int maxLeds) {

  for (uint16_t j=0; j<repeats; j++) {
    // Reset to current brightness.
    for(uint16_t e=0; e<strip.numPixels(); e++) {
      strip.setPixelColor(e, currentBrightness, currentBrightness, currentBrightness);
    }
    for (uint16_t i=0; i< random(minLeds, maxLeds); i++) {
      int led = random(strip.numPixels());
      if (color == 0) {
        strip.setPixelColor(led, random(0,255), random(0,255), random(0, 255));
      }
      if (color == 1) {
        strip.setPixelColor(led,theGreen);
      }

      else {
        strip.setPixelColor(led, color);
      }     
    }
    strip.setBrightness(255);
    strip.show();
    delay(random(10, 50));
  }

}

// SteadyColor
void steadyColor(uint32_t color) {
  for (uint16_t e=0; e<strip.numPixels(); e++) {
    strip.setPixelColor(e, color);
  }
  strip.show();
}

// Animation similar to the apple sleep pulse.
// Uses millis() for the animation instead of delay 
// so that it's more resilient to interupts.

void breath(long animationInterval) {
  
  static uint32_t brightness = startBrightness;

  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis > animationInterval/2) {
    for(uint16_t e=0; e<strip.numPixels(); e++) {
      strip.setPixelColor(e, white);
    }
    brightness = cycleBrightness(10, 125, brightness);
    currentBrightness = brightness;
    strip.setBrightness(brightness);
    previousMillis = currentMillis;
  }
  else {
    strip.setPixelColor(1, 30,0,0);
  }
  strip.show();
  delay(50); 
}

// Used by the breath to cycle the brightness.
uint32_t cycleBrightness(uint32_t minBrightness, uint32_t maxBrightness, uint32_t currentBrightness) {
  
  static uint32_t oldBrightness;
  uint32_t newBrightness;
  
  if (currentBrightness <= oldBrightness) {
    newBrightness = currentBrightness;
    newBrightness--;
  }
  else {
    newBrightness = currentBrightness;
    newBrightness++;
  }

  if (newBrightness >= maxBrightness) {
    newBrightness = maxBrightness;
    newBrightness--;
  }
  if (newBrightness <= minBrightness) {
    newBrightness = minBrightness;
    newBrightness += 2;
  }
  
  oldBrightness = currentBrightness;
  return newBrightness;
  
}

//  These are the animations from the AdaFruit Strand Test sketch.
//  example usages:
//    colorWipe(strip.Color(123,123,123),50);
//    colorWipe(strip.Color(240,240,240), 50); // white
//    colorWipe(strip.Color(255, 0, 0), 50); // Red
//    colorWipe(strip.Color(0, 255, 0), 50); // Green
//    colorWipe(strip.Color(0, 0, 255), 50); // Blue
//    rainbow(20);
//    rainbowCycle(20);

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}



