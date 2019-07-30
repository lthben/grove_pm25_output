//PM2_5 sensor to I2C port
//LCD to I2C
//RGB ring to D6

#include "Seeed_HM330X.h"
#include <Wire.h>

#ifdef  ARDUINO_SAMD_VARIANT_COMPLIANCE
#define SERIAL SerialUSB
#else
#define SERIAL Serial
#endif

HM330X sensor;
u8 buf[30];
u16 pm25val; //pm2.5 sensor value

const char *str[] = {"sensor num: ", "PM1.0 concentration(CF=1,Standard particulate matter,unit:ug/m3): ",
                     "PM2.5 concentration(CF=1,Standard particulate matter,unit:ug/m3): ",
                     "PM10 concentration(CF=1,Standard particulate matter,unit:ug/m3): ",
                     "PM1.0 concentration(Atmospheric environment,unit:ug/m3): ",
                     "PM2.5 concentration(Atmospheric environment,unit:ug/m3): ",
                     "PM10 concentration(Atmospheric environment,unit:ug/m3): ",
                    };

#include "rgb_lcd.h"
rgb_lcd lcd;

#include "Adafruit_NeoPixel.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(20, 6, NEO_GRB + NEO_KHZ800);



err_t print_result(const char* str, u16 value)
{
  if (NULL == str)
    return ERROR_PARAM;
  SERIAL.print(str);
  SERIAL.println(value);
  return NO_ERROR;
}

/*parse buf with 29 u8-data*/
err_t parse_result(u8 *data)
{
  u16 value = 0;
  err_t NO_ERROR;
  if (NULL == data)
    return ERROR_PARAM;
  for (int i = 1; i < 8; i++)
  {
    value = (u16)data[i * 2] << 8 | data[i * 2 + 1];
    print_result(str[i - 1], value);

    if (i == 6) //only interested in the PM2.5 environment reading
    {
      pm25val = value;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PM2.5 reading:");
      lcd.setCursor(0, 1);
      lcd.print(value);
    }
  }
}

err_t parse_result_value(u8 *data)
{
  if (NULL == data)
    return ERROR_PARAM;
  for (int i = 0; i < 28; i++)
  {
    SERIAL.print(data[i], HEX);
    SERIAL.print("  ");
    if ((0 == (i) % 5) || (0 == i))
    {
      SERIAL.println(" ");
    }
  }
  u8 sum = 0;
  for (int i = 0; i < 28; i++)
  {
    sum += data[i];
  }
  if (sum != data[28])
  {
    SERIAL.println("wrong checkSum!!!!");
  }
  SERIAL.println(" ");
  SERIAL.println(" ");
  return NO_ERROR;
}

//data is 0 to 19 corresponding to psi of 0 to 200
void colorWipe(uint8_t data, uint8_t wait)
{
  for (uint8_t i = 0; i <= data; i++)
  {
    if (i <= 4) strip.setPixelColor(i, strip.Color(0, 255, 0)); //green psi:0-50
    else if (i > 4 && i < 10) strip.setPixelColor(i, strip.Color(255, 255, 0)); //yellow psi:51-100
    else if (i >= 10 && i < 15) strip.setPixelColor(i, strip.Color(255, 128, 0)); //orange psi:101-150
    else strip.setPixelColor(i, strip.Color(255, 0, 0)); //red psi:>151

    strip.show();
    delay(wait);
  }
}

/*30s*/
void setup()
{
  SERIAL.begin(115200);
  delay(100);
  SERIAL.println("Serial start");
  if (sensor.init())
  {
    SERIAL.println("HM330X init failed!!!");
    while (1);
  }

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setRGB(255, 255, 255); //set RGB colour and brightness here
  lcd.setCursor(0, 0);

  lcd.print("Initialising ...");
  Serial.println("Waiting sensor to init...");
  delay(2000);//need 30s for sensor to stabilise

  Serial.println("Sensor ready.");
  lcd.clear();
  lcd.print("Sensor ready");
  delay(1000);

  strip.begin();
  strip.setBrightness(255);
  strip.show(); // Initialize all pixels to 'off'
}

void loop()
{
  if (sensor.read_sensor_value(buf, 29))
  {
    SERIAL.println("HM330X read result failed!!!");
  }
  parse_result_value(buf);
  parse_result(buf);
  SERIAL.println(" ");
  SERIAL.println(" ");
  SERIAL.println(" ");

  strip.clear();
  strip.show();//turn off

  delay(500);

  pm25val = constrain(pm25val, 0, 200);//set cap reading to 200 psi

  uint8_t data = map(pm25val, 0, 200, 0, 19);//convert 0-200 psi to 0-19 leds
  uint8_t wait = 100;

  colorWipe(data, wait);

  delay(5000);
}
