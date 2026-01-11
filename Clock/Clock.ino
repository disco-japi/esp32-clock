#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DS3231.h>
#include <math.h>

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define clock_Address 0x68
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DS3231 rtc = DS3231();
int lastsec = rtc.getSecond();
bool h12;
bool hPM;
bool dot = true;

float sinTable[60];
float cosTable[60];

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2  

#define switchPin 34
#define analogPin 23

bool color() {
  if(digitalRead(switchPin) == 1){
    return 1;
  }
  return 0;
}


bool states[10][7]{
  {1,1,1,1,1,0,1},
  {0,0,0,1,1,0,0},
  {1,0,1,1,0,1,1},
  {1,0,0,1,1,1,1},
  {0,1,0,1,1,1,0},
  {1,1,0,0,1,1,1},
  {0,1,1,0,1,1,1},
  {1,0,0,1,1,0,0},
  {1,1,1,1,1,1,1},
  {1,1,0,1,1,1,0},
};

struct rect{
  int x0;
  int y0;
  int h;
  int w;
  bool on;
};

class NumberShow{
  private:
    struct rect rects[7]{
      {x0,8,7,17,1}, //Arriba
      {x0,8,27,5,1}, //Izquierda arriba
      {x0,30,27,5,1}, //Izquierda abajo
      {x0+13,8,27,5,1}, //Derecha arriba
      {x0+13,30,27,5,1}, //Derecha anajo
      {x0,30,6,17,0}, //Centro
      {x0,50,7,17,1} //Abajo
    };
  public:
    NumberShow(){x0 = 0;};
    NumberShow(int x0_){x0 = x0_;};
    int x0;
    void set_x0(int x0_){
      x0 = x0_;
      for (int i = 0; i <=7; i++){
        rects[i].x0 = x0_;
      }
      rects[3].x0 = x0_+11;
      rects[4].x0 = x0_+11;
    }
    void estimate(int value_){
      for (int i = 0; i <=7; i++){
        if(states[value_][i]){
          display.fillRoundRect(rects[i].x0, rects[i].y0, rects[i].w, rects[i].h, 6, color());
        }
      }
    }
  };

NumberShow numbers[6];

class rRect{
  private:
    int x0;
    int y0;
    int L;
    int B;
    int w;
  public:
    rRect(int x0_, int y0_, int L_, int B_, int w_){
      x0 = x0_;
      y0 = y0_;
      L = L_;
      B = B_;
      w = w_;
    };
    void draw(int second){
      for (int len = 0; len <= L + B; len ++){
        display.drawPixel(x0+(int)((len-B)*sinTable[second]),y0-(int)((len-B)*cosTable[second]),color());
      }
    }
};

rRect hour = rRect(64,32,20,3,3);
rRect sec = rRect(64,32,30,5,1);
rRect minute = rRect(64,32,25,4,2);

void setup() {
    for(int i = 0; i<=59; i++){
    sinTable[i] = sin(M_PI * (i/(float)30));
    cosTable[i] = cos(M_PI * (i/(float)30));
    Serial.println(cosTable[i]);
  }
  pinMode(switchPin, INPUT_PULLUP); 
  pinMode(analogPin, INPUT); 
  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  rtc.setClockMode(false);
  numbers[0].set_x0(0);
  numbers[1].set_x0(19);
  numbers[2].set_x0(46);
  numbers[3].set_x0(65);
  numbers[4].set_x0(92);
  numbers[5].set_x0(111);
}

void loop() {
   if(rtc.getSecond() != lastsec){
    display.clearDisplay();
    display.fillScreen(!color());
    lastsec = rtc.getSecond();
    if(digitalRead(analogPin)){
      for(int i = 0; i < 12; i++){
        int l = 5;
        if(i % 3  == 0){
          l = 7;
        }
        for (int len = 0; len <= l; len ++){
          display.drawPixel(64+(int)((len+25)*sinTable[i*5]),32+(int)((len+25)*cosTable[i*5]),color());
        }
      }
      int h = rtc.getHour(h12,hPM);
      if(h >= 12){
        h -= 12;
      }
      hour.draw(h*5+(int)(rtc.getMinute()*5/(float)60));
      sec.draw(rtc.getSecond());
      minute.draw(rtc.getMinute());
    }else{
      numbers[0].estimate(((int)rtc.getHour(h12,hPM)/10) %10);
      numbers[1].estimate((int)rtc.getHour(h12,hPM) %10);
      numbers[2].estimate(((int)rtc.getMinute()/10) %10);
      numbers[3].estimate(((int)rtc.getMinute()) %10);
      numbers[4].estimate(((int)rtc.getSecond()/10) %10);
      numbers[5].estimate((int)rtc.getSecond()%10);
      dot = !dot;
      if(dot){
        display.drawRoundRect(38,16,7,7,5,color());
        display.drawRoundRect(38,42,7,7,5,color());
        display.drawRoundRect(84,16,7,7,5,color());
        display.drawRoundRect(84,42,7,7,5,color());
      }
    }
    display.display();
  }
}