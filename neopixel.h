/*********
  Mijaz Mukundan

  Built on the excellent tutorial at

  2) For neopixels: https://learn.adafruit.com/multi-tasking-the-arduino-part-3/put-it-all-together-dot-dot-dot

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Neopixel Code
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
// Pattern types supported:
enum pattern
{
  STATIC,
  RAINBOW_CYCLE,
  THEATER_CHASE,
  COLOR_WIPE,
  SCANNER,
  FADE,
  HEART
};
// Patern directions supported:
enum direction
{
  FORWARD,
  REVERSE
};

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
public:
  // Member Variables:
  pattern ActivePattern; // which pattern is running
  direction Direction;   // direction to run the pattern

  unsigned long Interval;   // milliseconds between updates
  unsigned long lastUpdate; // last update of position

  uint32_t Color1, Color2; // What colors are in use
  uint16_t TotalSteps;     // total number of steps in the pattern
  uint16_t Index;          // current step within the pattern

  void (*OnComplete)(); // Callback on completion of pattern

  // Constructor - calls base-class constructor to initialize strip
  NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
      : Adafruit_NeoPixel(pixels, pin, type)
  {
    OnComplete = callback;
  }

  // Update the pattern
  void Update()
  {
    if ((millis() - lastUpdate) > Interval) // time to update
    {
      lastUpdate = millis();
      switch (ActivePattern)
      {
      case RAINBOW_CYCLE:
        RainbowCycleUpdate();
        break;
      case THEATER_CHASE:
        TheaterChaseUpdate();
        break;
      case COLOR_WIPE:
        ColorWipeUpdate();
        break;
      case SCANNER:
        ScannerUpdate();
        break;
      case FADE:
        FadeUpdate();
        break;
      case STATIC:
        StaticUpdate();
        break;
      default:
        break;
      }
    }
  }

  // Increment the Index and reset at the end
  void Increment()
  {
    if (Direction == FORWARD)
    {
      Index++;
      if (Index >= TotalSteps)
      {
        Index = 0;
        if (OnComplete != NULL)
        {
          OnComplete(); // call the comlpetion callback
        }
      }
    }
    else // Direction == REVERSE
    {
      --Index;
      if (Index <= 0)
      {
        Index = TotalSteps - 1;
        if (OnComplete != NULL)
        {
          OnComplete(); // call the comlpetion callback
        }
      }
    }
  }

  // Reverse pattern direction
  void Reverse()
  {
    if (Direction == FORWARD)
    {
      Direction = REVERSE;
      Index = TotalSteps - 1;
    }
    else
    {
      Direction = FORWARD;
      Index = 0;
    }
  }

  // Update the Rainbow Cycle Pattern
  void RainbowCycleUpdate()
  {
    for (int i = 0; i < numPixels(); i++)
    {
      setPixelColor(i, Wheel(((i * 256 / numPixels()) + Index) & 255));
    }
    show();
    Increment();
  }

  // Update the Theater Chase Pattern
  void TheaterChaseUpdate()
  {
    for (int i = 0; i < numPixels(); i++)
    {
      if ((i + Index) % 3 == 0)
      {
        setPixelColor(i, Color1);
      }
      else
      {
        setPixelColor(i, Color2);
      }
    }
    show();
    Increment();
  }

  // Update the Color Wipe Pattern
  void ColorWipeUpdate()
  {
    setPixelColor(Index, Color1);
    show();
    Increment();
  }

  // Update the Scanner Pattern
  void ScannerUpdate()
  {
    for (int i = 0; i < numPixels(); i++)
    {
      if (i == Index) // Scan Pixel to the right
      {
        setPixelColor(i, Color1);
      }
      else if (i == TotalSteps - Index) // Scan Pixel to the left
      {
        setPixelColor(i, Color1);
      }
      else // Fading tail
      {
        setPixelColor(i, DimColor(getPixelColor(i)));
      }
    }
    show();
    Increment();
  }

  // Update the Fade Pattern
  void FadeUpdate()
  {
    // Calculate linear interpolation between Color1 and Color2
    // Optimise order of operations to minimize truncation error
    uint8_t red = ((Red(Color1) * (TotalSteps - Index)) + (Red(Color2) * Index)) / TotalSteps;
    uint8_t green = ((Green(Color1) * (TotalSteps - Index)) + (Green(Color2) * Index)) / TotalSteps;
    uint8_t blue = ((Blue(Color1) * (TotalSteps - Index)) + (Blue(Color2) * Index)) / TotalSteps;

    ColorSet(Color(red, green, blue));
    show();
    Increment();
  }

  void StaticUpdate()
  {
    ColorSet(Color1);
    show();
    Increment();
  }

  // Calculate 50% dimmed version of a color (used by ScannerUpdate)
  uint32_t DimColor(uint32_t color)
  {
    // Shift R, G and B components one bit to the right
    uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
    return dimColor;
  }

  // Set all pixels to a color (synchronously)
  void ColorSet(uint32_t color)
  {
    for (int i = 0; i < numPixels(); i++)
    {
      setPixelColor(i, color);
    }
    show();
  }

  // Returns the Red component of a 32-bit color
  uint8_t Red(uint32_t color)
  {
    return (color >> 16) & 0xFF;
  }

  // Returns the Green component of a 32-bit color
  uint8_t Green(uint32_t color)
  {
    return (color >> 8) & 0xFF;
  }

  // Returns the Blue component of a 32-bit color
  uint8_t Blue(uint32_t color)
  {
    return color & 0xFF;
  }

  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  uint32_t Wheel(byte WheelPos)
  {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
      return Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else if (WheelPos < 170)
    {
      WheelPos -= 85;
      return Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    else
    {
      WheelPos -= 170;
      return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
  }
};


//This is a helper function to convert the animation enumerator from string input
int setpattern(String mypattern);

//This helper function converts a hex color string input to 32 bit int required by neopixel library
uint32_t getColorFromHexString(String color);

//This helper function sets the number of steps based on the animation chosen
int setsteps(String mypattern, int numpixels);

// Set pattern based on the chosen animation style
int setpattern(String inputpattern)
{
  int mypattern;

  if (inputpattern == "RAINBOW_CYCLE")
  {
    mypattern = RAINBOW_CYCLE;
  }

  else if (inputpattern == "THEATER_CHASE")
  {
    mypattern = THEATER_CHASE;
  }

  else if (inputpattern == "COLOR_WIPE")
  {
    mypattern = COLOR_WIPE;
  }

  else if (inputpattern == "SCANNER")
  {
    mypattern = SCANNER;
  }

  else if (inputpattern == "FADE")
  {
    mypattern = FADE;
  }

  else if (inputpattern == "STATIC")
  {
    mypattern = STATIC;
  }

  else
  {
    mypattern = STATIC;
  }

  return mypattern;
}

// Decide step size based on selected animation style
int setsteps(String inputpattern, int numpixels)
{
  int steps;
  if (inputpattern == "RAINBOW_CYCLE")
  {
    steps = 255;
  }

  else if (inputpattern == "THEATER_CHASE")
  {
    steps = numpixels;
  }

  else if (inputpattern == "COLOR_WIPE")
  {
    steps = numpixels;
  }

  else if (inputpattern == "SCANNER")
  {
    steps = (numpixels - 1) * 2;
  }

  else if (inputpattern == "FADE")
  {
    steps = 255;
  }

  else if (inputpattern == "STATIC")
  {
    steps = 255;
  }

  else
  {
    steps = 255;
  }

  return steps;
}

// Converts Hexadecimal string to 32 bit integer which is recognised by Neopixel library
uint32_t getColorFromHexString(String color)
{
  int r = (int)strtol(color.substring(1, 3).c_str(), 0, 16);
  int g = (int)strtol(color.substring(3, 5).c_str(), 0, 16);
  int b = (int)strtol(color.substring(5, 7).c_str(), 0, 16);

  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}