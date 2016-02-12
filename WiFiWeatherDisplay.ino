// WiFi Enabled Weather Display
// Arduino Nano communicates with an ESP8266-01 WiFi chip that requests weather info from OpenWeatherMap API
// and displays the info as an infographic on a single 8x8 Red LED Matrix. 
// Last Modified: February 10th 2016
// Author: Joey Conenna

#include <LinkedList.h>
#include "LedControl.h"

// Instantiate a LinkedList that will hold bytes of cloud animation
LinkedList<byte> buffer = LinkedList<byte>();

// Empty byte to prefill buffer
byte empty = B00000000;

// SSID and password
String ssid     = "xxxxx";  
String password = "xxxxx";  

// TCP infortmation for connecting to OpenWeatherMap
String host     = "api.openweathermap.org"; // Open Weather Map API
const int httpPort   = 80;
String uri     = "/data/2.5/weather?zip=xxxxx,us&appid=APIKEY&units=imperial"; // Replace "xxxxx" with your zip code and your API Key with "APIKEY".


// Instantiate LedControl Object to use for SPI communcation with LED array
// Pin 11 is connected to the DataIn
// Pin 13 is connected to the CLK
// Pin 10 is connected to LOAD/CS
LedControl lc = LedControl(11, 13, 10, 1);

// Byte arrays to hold LED Matrix animations

// Clear day animations frames

byte clearDay[3][8]     =   { {B00000000, B00000000, B00011000, B00111100, B00111100, B00011000, B00000000, B00000000},
                              {B00000000, B01000010, B00011000, B00111100, B00111100, B00011000, B01000010, B00000000},
                              {B10010001, B01000010, B00011000, B00111100, B00111100, B00011000, B01000010, B10001001}
                            };
                            
byte clearNight[2][8]   =   { {B00000000, B00001000, B10000001, B10000001, B11000011, B11111111, B01111110, B00111100},
                              {B00001000, B00011100, B10001001, B10000001, B11000011, B11111111, B01111110, B00111100}
                            };

byte moderateRain[3][8] =   { {B00001100, B00011110, B00111110, B00011111, B01011111, B00011111, B10011110, B00001100},
                              {B00001100, B00011110, B01011110, B00011111, B10011111, B00011111, B00111110, B00001100},
                              {B00001100, B00011110, B10011110, B00011111, B00111111, B00011111, B01011110, B00001100}
                            };

byte heavyRain[3][8]    =   { {B10001100, B01011110, B00111110, B01011111, B10011111, B00111111 , B01011110, B10001100},
                              {B00101100, B10011110, B01011110, B10011111, B00111111, B01011111 , B10011110, B00101100},
                              {B01001100, B00111110, B10011110, B00111111, B01011111, B10011111,  B00111110, B01001100}
                            };

byte lightning[3][8]  =   { {B00001100, B00011110, B00011110, B00011111, B00011111, B00011111, B00011110, B00001100},
                              {B00001100, B00011110, B10111110, B01011111, B00011111, B00011111, B00011110, B00001100},
                              {B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111, B11111111}
                            };

// cloudDriver byte arrays
byte scatteredClouds[16] = {B00001100, B00011110, B00011110, B00011111, B00011111, B00011111, B00011110, B00001100, B00000000, B01100000, B11110000, B11110000, B11111000, B11111000, B11110000, B01100000};
byte fewClouds[16]       = {B00000100, B00000110, B00000110, B00000100, B00000000, B00010000, B00011000, B00011000, B00011000, B00010000, B00000000, B00100000, B01110000, B01110000, B01110000, B00100000};
byte brokenClouds[16]    = {B01000010, B11100111, B11100111, B11100111, B01000010, B00010000, B00111001, B10111011, B10111001, B00010000, B01000010, B11100111, B11100111, B11100111, B01000010, B00000000};

// Byte arrays for numbers 
byte numUnderHundred[10][3] = {{B00111110, B00100010, B00111110},
                               {B00000000, B00111110, B00000000},
                               {B00111010, B00101010, B00101110},
                               {B00101010, B00101010, B00111110},
                               {B00001110, B00001000, B00111110},
                               {B00101110, B00101010, B00111010},
                               {B00111110, B00101010, B00111010},
                               {B00000010, B00000010, B00111110},
                               {B00111110, B00101010, B00111110},
                               {B00001110, B00001010, B00111110},
                              };

byte numOverHundred[10][3]  = {{B11111000, B10001000, B11111001},
                               {B00000000, B11111000, B00000001},
                               {B11101000, B10101000, B10111001},
                               {B10101000, B10101000, B11111001},
                               {B00111000, B00100000, B11111001},
                               {B10111000, B10101000, B11101001},
                               {B11111000, B10101000, B11101001},
                               {B00001000, B00001000, B11111001},
                               {B11111000, B10101000, B11111001},
                               {B00111000, B00101000, B11111001}
                              };

byte hundredOnes[5]  = {B00000111, B00000000, B00000111, B00000101, B00000111};
byte hundredTeens[5] = {B00000111, B00000000, B00000000, B00000111, B00000000};


// Frame displays "H%" before humidity number 
byte HU[8] = {B00111110, B00001000, B00111110, B00000000, B00100100, B00010000, B00001000, B00100100};

// Variabes to hold weather obtained from the API
int weatherID    = 0;
int currentTemp  = 0;
int humidity     = 0;
long currentTime = 0;
long sunrise     = 0; 
long sunset      = 0;

// Setup function, runs once at startup
void setup()
{

  // Initialize cloud buffer to empty
  for (int i = 0; i < 8; i++)
    buffer.add(empty);

  // Max7219 Initializaion

  // Turn off power saving mode
  lc.shutdown(0, false);

  // Set the brightness level
  lc.setIntensity(0, 0); // Set second 0 up to 10 for brighter LEDs

  // Clear Display
  lc.clearDisplay(0);
  
  // Set scan limit
  lc.setScanLimit(0, 10);

  // Start ESP8266 Serial Communication
  Serial.begin(115200);   
  while (!Serial) ;   
  Serial.println("AT");  // Check Serial connection to ESP8266
  delay(5000);       
  if (!Serial.find("OK")); // Check ESP8266 response
  }


// Main loop, runs over and over
void loop()
{	
  while(1) // Inner loop to allow continue; calls where neccesary
  {
    
    if(!connectWiFi()) // If WiFi can't connect, try again
    {
      delay(5000);
  	  continue;
    }
    
    delay(5000);        
    if (!Serial.find("OK")); // Check ESP8266 response
  
    // Open TCP connection to host:
    Serial.println("AT+CIPSTART=\"TCP\",\"" + host + "\"," + httpPort);
    delay(50);        
    if (!Serial.find("OK")); // Check ESP8266 response
  
    // Construct our HTTP call
    String httpPacket = "GET " + uri + " HTTP/1.1\r\nHost: " + host + "\r\n\r\n";
    int length = httpPacket.length();
  
    // Send message length
    Serial.print("AT+CIPSEND=");
    Serial.println(length);
    delay(10); 
    if (!Serial.find(">")); // Check ESP8266 response
    
    // Send http request
    Serial.print(httpPacket);
    delay(10); 
    if (!Serial.find("SEND OK\r\n")); // Check ESP8266 response
  
    if (Serial.find("\r\n\r\n")) // Find end of the http header
    { 
      delay(5);

      // Get relevant info from Incoming Serial response
      
      weatherID   = serialFind("\"id\":", ',').toInt();
      currentTemp = serialFind("\"temp\":", ',').toInt();
      humidity    = serialFind("\"humidity\":", ',').toInt();
      currentTime = convertToLong(serialFind("\"dt\":", ','));
      sunrise     = convertToLong(serialFind("\"sunrise\":", ','));
      sunset      = convertToLong(serialFind("\"sunset\":", ','));
  
      // Close TCP Connection
      Serial.print("AT+CIPCLOSE");

      // Display information
      for (unsigned int j = 0; j < 30; j++)
      {
        displayWeather(weatherID);
        displayTemp(currentTemp);
        displayImage(HU);
        delay(2000);
        displayHumidity(humidity);
      }
     }// if
    }// while

} // loop

/* Functions */

// Connects to WiFi
boolean connectWiFi() {
  Serial.println("AT+CWMODE=1");
  delay(2000);
  String cmd = "AT+CWJAP=\"";
  cmd += ssid;
  cmd += "\",\"";
  cmd += password;
  cmd += "\"";
  Serial.println(cmd);
  delay(5000);
  
  if (Serial.find("OK")) 
   return true;
   
  else 
   return false;
  
}


// Searches Serial Buffer for a term, reads in a String after term until terminator is found and returns String
String serialFind(char term[], char terminator)
{
  String outputString = "";
  int timer = 0;
  int i;
  
  while (!Serial.find(term)) 
    {
      char c = Serial.read(); // find the part we are interested in.
      
      if(timer < 100)
      {
        timer++;
        delay(2);
      }
      
      else 
        break;
     }

    while (i < 60000) { // 1 minute timeout checker
      if (Serial.available())
      {
        char c = Serial.read();
        if (c == terminator) break; // break out of our loop because we got all we need
        outputString += c; // append to our output string
        i = 0; // reset our timeout counter
      }
      i++;
    }
    
   return outputString;
}


// Converts String object to long int
long convertToLong(String t)
{
  char longBuffer[32]; // make this at least big enough for the whole string
  t.toCharArray(longBuffer, sizeof(longBuffer));
  return atol(longBuffer);

}


// Displays weather animation depending on weatherID code sent from OpenWeatherMap API
// I live in FL, so no Snow or Blizzard compatibility here :P
void displayWeather(int code)
{

  if (code == 200 || (code <= 232 && code >= 230)) // thunderstorm with light rain
  {
    for (unsigned int i = 0; i < 3; i ++)
    {
      for (unsigned int j = 0; j < 3; j ++)
      {
        displayImage(moderateRain[j]);
        delay(400);
      }

      for (unsigned int j = 0; j < 2; j ++)
        for (unsigned int k = 1; k < 3; k ++)
        {
          displayImage(lightning[k]);
          if (k == 1)
            delay(200);
          else
            delay(30);
        }
    }
  }

  else if (code == 201) // thunderstorm with moderate rain
  {
    for (unsigned int i = 0; i < 4; i ++)
    {
      for (unsigned int j = 0; j < 4; j ++)
        for (unsigned int k = 0; k < 3; k ++)
        {
          displayImage(moderateRain[k]);
          delay(100);
        }

      for (unsigned int j = 0; j < 2; j ++)
        for (unsigned int k = 1; k < 3; k ++)
        {
          displayImage(lightning[k]);
          if (k == 1)
            delay(200);
          else
            delay(30);
        }
    }
  }

  else if (code == 202) // thunderstorm with heavy rain
  {
    for (unsigned int i = 0; i < 4; i ++)
    {

      for (unsigned int j = 0; j < 4; j ++)
        for (unsigned int k = 0; k < 3; k ++)
        {
          displayImage(heavyRain[k]);
          delay(100);
        }

      for (unsigned int j = 0; j < 2; j ++)
        for (unsigned int k = 1; k < 3; k ++)
        {
          displayImage(lightning[k]);
          if (k == 1)
            delay(200);
          else
            delay(30);
        }
    }
  }

  else if (code >= 210 && code <= 221) // lightning storm
  {
    for (unsigned int i = 0; i < 4; i ++)
    {
      for (unsigned int j = 0; j < 3; j ++)
      {
        displayImage(lightning[j]);
        if (j == 0)
          delay(700);
        else if (j == 1)
          delay(200);
        else
          delay(30);
      }

      for (unsigned int j = 1; j < 3; j ++)
      {
        displayImage(lightning[j]);
        if (j == 1)
          delay(200);
        else
          delay(30);
      }
    }
  }

  else if ((code >= 300 && code <= 321) || code == 500 || code == 520) // display light rain or drizzle
  {
    for (unsigned int i = 0; i < 5; i ++)
      for (unsigned int j = 0; j < 3; j ++)
      {
        displayImage(moderateRain[j]);
        delay(400);
      }

  }

  else if (code == 501 || code == 521) // display moderate rain
  {
    for (unsigned int i = 0; i < 20; i ++)
      for (unsigned int j = 0; j < 3; j ++)
      {
        displayImage(moderateRain[j]);
        delay(100);
      }
  }

  else if ((code >= 502 && code <= 511) || code == 522 || code == 531 || code == 771 || code == 906 || code == 901 || code == 902 || code > 958) // display heavy rain
  {
    for (unsigned int i = 0; i < 20; i ++)
      for (unsigned int j = 0; j < 3; j ++)
      {
        displayImage(heavyRain[j]);
        delay(100);
      }
  }

  else if (code == 801) // display few clouds
    cloudDriver(fewClouds);

  else if (code == 802) // display scattered clouds
    cloudDriver(scatteredClouds);

  else if (code == 803 || code == 804) // display overcast or broken clouds
    cloudDriver(brokenClouds);

  else
    dayOrNight();

}


// If clear, check if it is day or night and display appropriate animation.
void dayOrNight()
{
  if((currentTime >= sunrise) && (currentTime <= sunset))
    displayClearDay();
    
  else 
    displayClearNight();
}


// Displays clear day animation
void displayClearDay()
{
  // display clear day
  for (unsigned int i = 0; i < 5; i ++)
    for (unsigned int j = 0; j < 3; j ++)
    {
      displayImage(clearDay[j]);
      delay(400);
    }
}


// Displays clear night animation
void displayClearNight()
{
  // display clear night
  for (unsigned int i = 0; i < 5; i ++)
    for (unsigned int j = 0; j < 2; j ++)
    {
      displayImage(clearNight[j]);
      delay(600);
    }
}


// Runs cloud animations
void cloudDriver(byte b[])
{
  for (unsigned int i = 0; i < 2; i++)
  {
    for (unsigned int j = 0 ; j < 16; j++)
    {
      passInByte(b[j]);
      displayBuffer();
      delay(187);
    }

    passInByte(empty);
    displayBuffer();
    delay(187);
  }
  clearBuffer();
}


// Turns on the entire LED array according to the byte states in the image. Used for weather animations (except cloudy varieties), and numbers.
void displayImage(byte image[])
{
  for (unsigned int i = 0; i < 8; i++)
    lc.setRow(0, i, image[i]);
}


// Takes a byte and pushes it only the end of the buffer linked list, and then pops off the first byte from the
// linked list. Used for cloudDriver.
void passInByte(byte b)
{
  buffer.shift();
  buffer.add(b);
}


// Turns on the entire LED array according to the byte states in the buffer. Used for cloudDriver.
void displayBuffer()
{

  for (unsigned int i = 0; i < 8; i++)
  {
    lc.setRow(0, i, buffer.get(i) );
  }
}


// Clears cloud buffer
void clearBuffer()
{
  for (unsigned int i = 0; i < 16; i++)
  {
    buffer.shift();
    buffer.add(B00000000);
  }
}


// Displays temperature on 8x8 LED Matrix
void displayTemp(int temp)
{
  if (temp < 100)
    displayUnderHundred(temp);
  else
    displayOverHundred(temp);
  delay(6000);
}


// Displays a temperature under 100
void displayUnderHundred(int temp)
{

  if (temp < 10)
    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i, numUnderHundred[0][i]);
  else
    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i, numUnderHundred[temp / 10][i]);

  lc.setRow(0, 3, B00000000);

  for (unsigned int i = 0; i < 3 ; i++)
    lc.setRow(0, i + 4, numUnderHundred[temp % 10][i]);

  lc.setRow(0, 7, B00000001);
}


// Displays a temperature over 100 up to 119 (I am optimistic about Global Warming trends).
void displayOverHundred(int temp)
{

  if (temp < 110)
    for (unsigned int i = 0; i < 5 ; i++)
      lc.setRow(0, i, hundredOnes[i]);
  else
    for (unsigned int i = 0; i < 5 ; i++)
      lc.setRow(0, i, hundredTeens[i]);
  for (unsigned int i = 0; i < 3 ; i++)
    lc.setRow(0, i + 5, numOverHundred[temp % 10][i]);

}


// Displays a humidity value up to 99%
void displayHumidity(int h)
{
  if(h == 100)
    h = 99;
    
  if (h < 10)
    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i, numUnderHundred[0][i]);
  else
    for (unsigned int i = 0; i < 3 ; i++)
      lc.setRow(0, i, numUnderHundred[h / 10][i]);

  lc.setRow(0, 3, B00000000);

  for (unsigned int i = 0; i < 3 ; i++)
    lc.setRow(0, i + 4, numUnderHundred[h % 10][i]);

  lc.setRow(0, 7, B00000000);
  delay(6000);
}
