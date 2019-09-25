 #include <Wire.h>
#include "DS1307.h"
#include "DHT.h"
#include <EEPROM.h>


#define ERR_GLYPH_NOT_FOUND 1
#define DHTTYPE  DHT11   // DHT 11

#define BUZZER_PIN         9
#define DHT11_PIN         11
#define HC595_DATA_PIN    12   //数据引脚
#define HC595_SHCP_PIN    13   //时钟引脚
#define HC595_STCP_PIN    10   //锁存引脚

#define KEY               A2
#define LUX               A3

#define DS1307_INT        3

#define ROW1             A0
#define ROW2             A1
#define ROW3              2
#define ROW4              4
#define ROW5              5
#define ROW6              6
#define ROW7              7
#define ROW8              8

#define MAX_BRIGHTNESS    0xff      //最大亮度
#define MAX_ROW            8
#define FB_PIXEL_W  32
#define FB_PIXEL_H  8

#define USERFLAG     0xAA     //用户数据标志位
#define USERDATAADDR 0x00     //用户数据地址
#define BRIGHTNESS   0x32     //出厂默认亮度
#define DISPLAY_MODE_MAX 0x03

#define KEY_ADC_KEY_NONE_VALUE 0
#define KEY_ADC_KEY_1_VALUE 512
#define KEY_ADC_KEY_2_VALUE 340
#define KEY_ADC_KEY_3_VALUE 255
#define KEY_ADC_KEY_THRESHOLD 32

#define KEY_NONE 0
#define KEY_2    2
#define KEY_3    3
#define KEY_4    4
enum _key_state_{
    KS_NONE, //No key pressed
    K2_PRESSED,  // pressed stably
    K3_PRESSED,  // pressed stably
    K4_PRESSED,  // pressed stably
    K4_PRESSED_LONG // long pressed
};
// typedef struct _key_ {
//   uint8_t adc_pin;
//   uint8_t key_state;
//   uint8_t pressed_key;
//   uint8_t pressing_tick;
//   uint8_t pressed_tick;
//   uint8_t releasing_tick;
//   key_event_fn_t event_fn;
// } key_t;
// a single char encoding
typedef struct _glyph_
{
  char ch;    //encoding char
  uint8_t w;  // width
  uint8_t h;  // height
  uint8_t bitmap[8]; //7x8bit
} glyph_t;
typedef struct 
{
  uint8_t userflag;
  uint8_t brightness;    //encoding char
  uint8_t displaymode;
} userdata_t;
// font type
enum _font_type {
        FT_LED,
        FT_ARIA
};
enum _set_type{
        SET_HOUR,
        SET_MINUTE,
};
const uint8_t font_3x5_led[] PROGMEM = {
        '.',  1, 5, 0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x00,
        '$',  3, 5, 0x40,0xE0,0xE0,0xE0,0x40,0x00,0x00,0x00,//place holder
        ' ',  1, 5, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        '0',  3, 5, 0xE0,0xA0,0xA0,0xA0,0xE0,0x00,0x00,0x00,
        '1',  3, 5, 0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00,
        '2',  3, 5, 0xE0,0x20,0xE0,0x80,0xE0,0x00,0x00,0x00,
        '3',  3, 5, 0xE0,0x20,0xE0,0x20,0xE0,0x00,0x00,0x00,
        '4',  3, 5, 0xA0,0xA0,0xE0,0x20,0x20,0x00,0x00,0x00,
        '5',  3, 5, 0xE0,0x80,0xE0,0x20,0xE0,0x00,0x00,0x00,
        '6',  3, 5, 0xE0,0x80,0xE0,0xA0,0xE0,0x00,0x00,0x00,
        '7',  3, 5, 0xE0,0x20,0x20,0x20,0x20,0x00,0x00,0x00,
        '8',  3, 5, 0xE0,0xA0,0xE0,0xA0,0xE0,0x00,0x00,0x00,
        '9',  3, 5, 0xE0,0xA0,0xE0,0x20,0xE0,0x00,0x00,0x00,
        ':',  1, 5, 0x00,0x80,0x00,0x80,0x00,0x00,0x00,0x00,
        '%',  3, 5, 0xA0,0x40,0xA0,0x00,0x00,0x00,0x00,0x00,
        '/',  2, 5, 0x40,0x40,0x40,0x80,0x80,0x00,0x00,0x00,
        '-',  2, 5, 0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x00,  //-
        0xE0, 3, 5, 0x00,0x80,0x60,0x40,0x60,0x00,0x00,0x00, // ℃
        0 // here is the end flag.
};
const uint8_t font_4x7_led[] PROGMEM = {
        '.', 1, 7, 0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,
        '$', 4, 7, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //place holder
        ' ', 1, 7, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        '0',  4, 7, 0xF0,0x90,0x90,0x90,0x90,0x90,0xF0,0x00,
        '1',  4, 7, 0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x00,
        '2',  4, 7, 0xF0,0x10,0x10,0xF0,0x80,0x80,0xF0,0x00,
        '3',  4, 7, 0xF0,0x10,0x10,0xF0,0x10,0x10,0xF0,0x00,
        '4',  4, 7, 0x90,0x90,0x90,0xF0,0x10,0x10,0x10,0x00,
        '5',  4, 7, 0xF0,0x80,0x80,0xF0,0x10,0x10,0xF0,0x00,
        '6',  4, 7, 0xF0,0x80,0x80,0xF0,0x90,0x90,0xF0,0x00,
        '7',  4, 7, 0xF0,0x10,0x10,0x10,0x10,0x10,0x10,0x00,
        '8',  4, 7, 0xF0,0x90,0x90,0xF0,0x90,0x90,0xF0,0x00,
        '9',  4, 7, 0xF0,0x90,0x90,0xF0,0x10,0x10,0xF0,0x00,
        ':',  1, 7, 0x00,0x00,0x80,0x00,0x80,0x00,0x00,0x00,
        '%',  4, 7, 0xC0,0xC0,0x10,0x20,0x40,0xB0,0x30,0x00,
        '/',  2, 7, 0x00,0x00,0x40,0xC0,0x80,0x00,0x00,0x00,
        '-',  2, 7 ,0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,  //-
        0xE0, 4, 7, 0x80,0x60,0x90,0x80,0x80,0x90,0x60,0x00, // ℃
        0 // here is the end flag.
};
const uint8_t font_5x8_led[] PROGMEM = {
        '.',  1, 8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
        '$',  5, 8, 0x20,0x78,0xA0,0xF8,0xF8,0x28,0xF0,0x20,  //place holder
        ' ',  2, 8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        '0',  5, 8, 0xF8,0xF8,0xD8,0xD8,0xD8,0xD8,0xF8,0xF8,
        '1',  5, 8, 0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        '2',  5, 8, 0xF8,0x18,0x18,0xF8,0xF8,0xC0,0xC0,0xF8,
        '3',  5, 8, 0xF8,0x18,0x18,0xF8,0xF8,0x18,0x18,0xF8,
        '4',  5, 8, 0x98,0x98,0x98,0x98,0xF8,0x18,0x18,0x18,
        '5',  5, 8, 0xF8,0xC0,0xC0,0xF8,0xF8,0x18,0x18,0xF8,
        '6',  5, 8, 0xF8,0x80,0x80,0xF8,0x98,0x98,0x98,0xF8,
        '7',  5, 8, 0xF8,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
        '8',  5, 8, 0xF8,0x98,0x98,0xF8,0xF8,0x98,0x98,0xF8,
        '9',  5, 8, 0xF8,0x98,0x98,0x98,0xF8,0x18,0x18,0xF8,
        ':',  2, 8, 0x00,0xC0,0xC0,0x00,0x00,0xC0,0xC0,0x00,
        '%',  5, 8, 0xC8,0xC8,0x10,0x20,0x40,0x98,0x98,0x00,
        '/',  2, 8, 0x00,0x40,0x40,0x40,0x80,0x80,0x80,0x00,
        '-',  2, 8, 0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,  //-
        0xE0, 5, 8, 0xB8,0x20,0x20,0x20,0x38,0x00,0x00,0x00,  // ℃
        0 // here is the end flag.
};
const uint8_t font_6x8_led[] PROGMEM = {
        '.',  1, 8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
        '$',  6, 8, 0x30,0xFC,0xB0,0xFC,0xFC,0x34,0xFC,0x30,  //place holder
        ' ',  2, 8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        '0',  6, 8, 0xFC,0xFC,0xCC,0xCC,0xCC,0xCC,0xFC,0xFC,
        '1',  6, 8, 0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,
        '2',  6, 8, 0xFC,0x0C,0x0C,0xFC,0xFC,0xC0,0xC0,0xFC,
        '3',  6, 8, 0xFC,0x0C,0x0C,0xFC,0xFC,0x0C,0x0C,0xFC,
        '4',  6, 8, 0x8C,0x8C,0x8C,0xFC,0xFC,0x0C,0x0C,0x0C,
        '5',  6, 8, 0xFC,0xC0,0xC0,0xFC,0xFC,0x0C,0x0C,0xFC,
        '6',  6, 8, 0xFC,0xC0,0xC0,0xFC,0xCC,0xCC,0xCC,0xFC,
        '7',  6, 8, 0xFC,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
        '8',  6, 8, 0xFC,0xCC,0xCC,0xFC,0xFC,0xCC,0xCC,0xFC,
        '9',  6, 8, 0xFC,0x8C,0x8C,0xFC,0xFC,0x0C,0xFC,0xFC,
        ':',  2, 8, 0x00,0xC0,0xC0,0x00,0x00,0xC0,0xC0,0x00,
        '%',  6, 8, 0xC4,0xC8,0x10,0x20,0x4C,0x8C,0x00,0x00,
        '/',  2, 8, 0x00,0x40,0x40,0x40,0x80,0x80,0x80,0x00,
        '-',  2, 8, 0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,  //-
        0xE0, 6, 8, 0xBC,0x3C,0x20,0x20,0x20,0x3C,0x3C,0x00,  // ℃
        0 // here is the end flag.
};
const uint8_t font_6x8_led1[] PROGMEM = {
        '.',  1, 8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
        '$',  6, 8, 0x30,0xFC,0xB0,0xFC,0xFC,0x34,0xFC,0x30,  //place holder
        ' ',  2, 8, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        '0',  6, 8, 0x30,0x48,0x84,0x84,0x84,0x84,0x48,0x30,
        '1',  6, 8, 0x30,0x10,0x10,0x10,0x10,0x10,0x10,0x38,
        '2',  6, 8, 0x78,0x84,0x84,0x08,0x10,0x20,0x40,0xFC,
        '3',  6, 8, 0x78,0x84,0x04,0x78,0x04,0x04,0x84,0x78,
        '4',  6, 8, 0x18,0x28,0x48,0x88,0xFC,0x08,0x08,0x08,
        '5',  6, 8, 0xFC,0x80,0x80,0xF8,0x04,0x04,0x84,0x78,
        '6',  6, 8, 0x78,0x80,0x80,0xF8,0x84,0x84,0x84,0x78,
        '7',  6, 8, 0xFC,0x04,0x04,0x08,0x10,0x20,0x40,0x80,
        '8',  6, 8, 0x78,0x84,0x84,0x78,0x84,0x84,0x84,0x78,
        '9',  6, 8, 0x78,0x84,0x84,0x7C,0x04,0x04,0x84,0x78,
        ':',  2, 8, 0x00,0xC0,0xC0,0x00,0x00,0xC0,0xC0,0x00,
        '%',  6, 8, 0xC4,0xC8,0x10,0x20,0x4C,0x8C,0x00,0x00,
        '/',  2, 8, 0x00,0x40,0x40,0x40,0x80,0x80,0x80,0x00,
        '-',  2, 8, 0x00,0x00,0x00,0xC0,0x00,0x00,0x00,0x00,  //-
        0xE0, 6, 8, 0xBC,0x3C,0x20,0x20,0x20,0x3C,0x3C,0x00,  // ℃
        0 // here is the end flag.
};

                           // 低位先入 从左到右对应  0-31位   共阴极   0xFFFFFFFF 全灭     0x00000000全亮
                           //第1行     //第2行    //第3行     //第4行     //第5行     //第6行    //第7行    //第8行
uint32_t DisPlayBuff[8] = {0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0xffffffff};   //显示缓冲区
uint8_t  RowList[8]={ROW1,ROW2,ROW3,ROW4,ROW5,ROW6,ROW7,ROW8};
//uint8_t  CurrLine = 0;
uint8_t  DutyCycle = 30;   //亮度等级  0-256   256-最亮  0-熄灭                                           
float Hum,Temp;
char string_buff[10];
uint8_t DS1307ISR_FLAG = 0;
uint16_t UartTimeout = 0;
uint8_t  RX_num = 0;
uint8_t  RX_buff[64];
userdata_t UserData;
uint8_t key_state = KS_NONE;
uint16_t key_counter = 0;
float h = 0;
float t = 0;

uint8_t *pCurrentFont = font_6x8_led;

//char *pStr = string_buff;
DHT dht(DHT11_PIN, DHTTYPE);
DS1307 clock;//define a object of DS1307 class


void ClearUserData(void)
{
       // EEPROM.clear();
}
void SetUserData(void)
{
        EEPROM.write(0,UserData.userflag);     
        EEPROM.write(1,UserData.brightness);   
        EEPROM.write(2,UserData.displaymode);   
}
void ReadUserData(void)
{
        UserData.userflag = EEPROM.read(0);
        UserData.brightness = EEPROM.read(1);
        UserData.displaymode = EEPROM.read(2);
        //   Serial.println(UserData.userflag,DEC);
        //   Serial.println(UserData.brightness,DEC);
        if (UserData.userflag != USERFLAG)
        {
                UserData.userflag = USERFLAG;
                UserData.brightness = BRIGHTNESS;
                UserData.displaymode = 0;
                SetUserData();
                clock.fillByYMD(2019,1,1);
                clock.fillByHMS(0,0,0);
                clock.setTime();
        }
        else
        {
                /* code */
        }            
}
// the setup function runs once when you press reset or power the board
void setup() 
{
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);
  pinMode(ROW5, OUTPUT);
  pinMode(ROW6, OUTPUT);
  pinMode(ROW7, OUTPUT);
  pinMode(ROW8, OUTPUT);

  pinMode(HC595_DATA_PIN, OUTPUT);
  pinMode(HC595_SHCP_PIN, OUTPUT);
  pinMode(HC595_STCP_PIN, OUTPUT);

  digitalWrite(ROW1, HIGH);
  digitalWrite(ROW2, HIGH);
  digitalWrite(ROW3, HIGH);
  digitalWrite(ROW4, HIGH);
  digitalWrite(ROW5, HIGH);
  digitalWrite(ROW6, HIGH);
  digitalWrite(ROW7, HIGH);
  digitalWrite(ROW8, HIGH);

  //pinMode(DS1307_INT, INPUT_PULLUP);

  //下降沿触发，触发中断0，调用blink函数
  //attachInterrupt(1, ReadTime_ISR, FALLING);

  Serial.begin(115200);
  dht.begin();
  clock.begin();
  clock.SetISR();
  //下降沿触发，触发中断0，调用blink函数
  //attachInterrupt(1, ReadTime_ISR, FALLING);
  ReadUserData();
}
void ReadDHT11(void)
{
  float Hum = dht.readHumidity();
  float Temp = dht.readTemperature();

  // Serial.print(Hum);
  // Serial.print("---");
  // Serial.println(Temp);
}
void shiftOut_32(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint32_t val)
{
  
  for (size_t i = 0; i < 4; i++)
  {
    shiftOut(HC595_DATA_PIN, HC595_SHCP_PIN, LSBFIRST, val>>(i*8));
  }

}
void DisPlay(uint8_t brightness)
{
  for (uint8_t i= 0; i < MAX_ROW; i++)
  {
    if (brightness == 0)
    {
      digitalWrite(HC595_STCP_PIN, LOW); //将ST_CP口上加低电平让芯片准备好接收数据
      shiftOut_32(HC595_DATA_PIN, HC595_SHCP_PIN, LSBFIRST, 0xffffffff);
      digitalWrite(HC595_STCP_PIN, HIGH); //将ST_CP这个针脚恢复到高电平
    }
    else
    {
      digitalWrite(HC595_STCP_PIN, LOW); //将ST_CP口上加低电平让芯片准备好接收数据
      shiftOut_32(HC595_DATA_PIN, HC595_SHCP_PIN, LSBFIRST, DisPlayBuff[i]);
      digitalWrite(HC595_STCP_PIN, HIGH); //将ST_CP这个针脚恢复到高电平
      digitalWrite(RowList[i], LOW);
      delayMicroseconds(brightness);
      digitalWrite(RowList[i], HIGH);
      delayMicroseconds(10);
    }    
  }
}
//uint8_t* font_get_font(uint8_t font_type){
//    return font_type == FT_LED ? font_4x7_led : font_4x7_aria;
//}


uint8_t font_get_glyph(const uint8_t* font, char encoding, glyph_t* glyph)
{

        glyph_t* pos =(glyph_t *) font;
        char ch = 0;
        do{
                ch = pgm_read_byte(pos);
                if(ch == encoding) {
                        memcpy_P(glyph, pos, sizeof(glyph_t));
                        return 0;
                } else {
                        pos++;
                }
        }while(ch != 0);

        return ERR_GLYPH_NOT_FOUND;
}
void _fb_set_pixel(uint32_t* fb, uint8_t x, uint8_t y, bool pixel){
        if(x >FB_PIXEL_W || y > FB_PIXEL_H) 
        {
                return;
        }
        uint32_t xx = 0x00000001;
        uint32_t v = fb[y];
        if(pixel)
        {
          v &= ~(xx << x); //打点
        }
        else
        {
          v |= (xx << x);    
        }
        fb[y] = v;
}
bool _fb_get_pixel(uint32_t* fb, uint8_t x, uint8_t y)
{
        if(x >FB_PIXEL_W || y > FB_PIXEL_H) 
        {
                return 0;
        }
        uint32_t v = fb[y];
        return ( v >> x ) & 1;
}
void _fb_flush_bank(uint32_t* fb, uint8_t bank)
{

}
uint8_t fb_draw_char(uint32_t* fb, char ch, uint8_t x, uint8_t y,const uint8_t* font)
{
        if(x > FB_PIXEL_W || y > FB_PIXEL_H) 
        {
                return x;
        }

        glyph_t glyph;
        if(font_get_glyph(font, ch, &glyph) != ERR_GLYPH_NOT_FOUND) {
                for(uint8_t yy = 0; yy < min(glyph.h, FB_PIXEL_H - y); yy++) {
                        uint8_t b = glyph.bitmap[yy];
                        for(uint8_t xx = 0; xx < min(glyph.w, FB_PIXEL_W - x); xx++)
                        {
                                _fb_set_pixel(fb, x + xx, y + yy, (b >> (7 - xx )) & 1);
                        }
                }
                return x + glyph.w + 1;
        }
        return x;
}
uint8_t fb_draw_string(uint32_t* fb, char* str,uint8_t len, uint8_t x, uint8_t y, const uint8_t* font)
{
        for(uint8_t i=0;i<len; i++) 
        {
                x = fb_draw_char(fb, *(str+i), x, y, font);
        }
        return x;
}
void ReadTime_ISR()
{
  DS1307ISR_FLAG = 1;
}

uint8_t _key_decide_key(uint16_t val){
        if(val == KEY_ADC_KEY_NONE_VALUE) {
                return KEY_NONE;
        }

        if( val >= KEY_ADC_KEY_1_VALUE - KEY_ADC_KEY_THRESHOLD
            && val <= KEY_ADC_KEY_1_VALUE + KEY_ADC_KEY_THRESHOLD) {
                return KEY_2;
        }

        if(val >= KEY_ADC_KEY_2_VALUE - KEY_ADC_KEY_THRESHOLD
           && val <= KEY_ADC_KEY_2_VALUE + KEY_ADC_KEY_THRESHOLD) {
                return KEY_3;
        }

        if(val >= KEY_ADC_KEY_3_VALUE - KEY_ADC_KEY_THRESHOLD
           && val <= KEY_ADC_KEY_3_VALUE + KEY_ADC_KEY_THRESHOLD) {
                return KEY_4;
        }

        return KEY_NONE;
}
void key_scan(void)
{
        uint16_t val = analogRead(KEY);
        uint8_t k = _key_decide_key(val);
        //Serial.println(k,DEC);
        if (key_state == KS_NONE)
        {
        
                if(k==KEY_NONE)
                {
                        key_state = KS_NONE;
                        key_counter = 0;
                }
                else if (k == KEY_2)
                {
                        if (key_counter < 15)
                        {
                                key_counter++;
                        }
                        else //if(key_counter)
                        {
                                key_state = K2_PRESSED;
                        }  
                }
                else if (k == KEY_3)
                {
                        if (key_counter < 15)
                        {
                                key_counter++;
                        }
                        else //if(key_counter)
                        {
                                key_state = K3_PRESSED;
                        }  
                }
                else if (k == KEY_4)
                {
                        if (key_counter < 15)
                        {
                                key_counter++;
                        }
                        else 
                        {
                                key_state = K4_PRESSED;
                        }  
                        
                }                
        }     
}
// the loop function runs over and over again forever
void loop() 
{
// _fb_set_pixel(DisPlayBuff,1,1,1);
// _fb_set_pixel(DisPlayBuff,2,6,1);
// _fb_set_pixel(DisPlayBuff,14,3,1);
// _fb_set_pixel(DisPlayBuff,16,5,1);
//glyph_t glyph;
//font_get_glyph(font_4x7_led,'2',&glyph);
//   fb_draw_char(DisPlayBuff,'2',4, 0,font_5x8_led);
//  fb_draw_char(DisPlayBuff,'6',6, 0,font_4x7_led);
//  fb_draw_char(DisPlayBuff,'9',18, 0,font_4x7_led);
//fb_draw_string(DisPlayBuff,"16:24",sizeof("01:23"),0,0,font_6x8_led);
  key_scan();
  if (key_state != KS_NONE)
  {
          switch (key_state)
          {
          case K2_PRESSED:
                  //Serial.println("K2");
                          UserData.brightness++;
                          SetUserData();
                          key_state = KS_NONE;
                          key_counter = 0;
                  break;
          case K3_PRESSED:
                  //Serial.println("K3");
                          UserData.brightness--;
                          SetUserData();
                          key_state = KS_NONE;
                          key_counter = 0;
                  break;
          case K4_PRESSED:
                //   //Serial.println("K4");
                  uint16_t val = analogRead(KEY);
                  uint8_t k = _key_decide_key(val);
                  key_counter++;
                  if ((key_counter < 50)&&(k == KS_NONE))
                  {
                          //Serial.println("K4");
                          uint16_t count = 0;
                          h = dht.readHumidity();
                          t = dht.readTemperature();
                          memset(DisPlayBuff,0xff,32);
                          sprintf(string_buff,"%02d%c %02d%%",(int)t,0xE0,(int)h); 
                          fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),0,1,font_4x7_led); 
                          while (count < 1000)
                          {
                                count++;
                                DisPlay(UserData.brightness);
                          }
                          memset(DisPlayBuff,0xff,32);
                          key_state = KS_NONE;
                          key_counter = 0;                  
                  }
                  else if ((key_counter > 200)&&(k == KS_NONE))
                  {
                          //Serial.println("K4_long");
                          key_state = KS_NONE;
                          key_counter = 0;  
                          uint8_t flag = 1;

                          clock.getTime();
                          uint8_t hour = clock.hour;
                          uint8_t minute = clock.minute;
                          uint8_t set_state = SET_HOUR;
                          memset(DisPlayBuff,0xff,32);
                          while (flag == 1)
                          {

                                  key_scan();
                                        if (key_state != KS_NONE)
                                        {
                                                switch (key_state)
                                                {
                                                case K2_PRESSED:
                                                        if (set_state==SET_HOUR)
                                                        {
                                                                if ((0<=hour)&&(hour<23))
                                                                {
                                                                        hour++;
                                                                }
                                                                else
                                                                {
                                                                        hour=0;
                                                                } 
                                                        }
                                                        else
                                                        {
                                                                if ((0<=minute)&&(minute<59))
                                                                {
                                                                        minute++;
                                                                }
                                                                else
                                                                {
                                                                        minute=0;
                                                                } 
                                                        }
                                                        key_state = KS_NONE;
                                                        key_counter = 0;
                                                        break;
                                                case K3_PRESSED:
                                                        if (set_state==SET_HOUR)
                                                        {
                                                                if ((0<hour)&&(hour<=23))
                                                                {
                                                                        hour--;
                                                                }
                                                                else
                                                                {
                                                                        hour=23;
                                                                } 
                                                        }
                                                        else
                                                        {
                                                                if ((0<minute)&&(minute<=59))
                                                                {
                                                                        minute--;
                                                                }
                                                                else
                                                                {
                                                                        minute=59;
                                                                } 
                                                        }
                                                        key_state = KS_NONE;
                                                        key_counter = 0;
                                                        break;
                                                case K4_PRESSED:
                                                        //   //Serial.println("K4");
                                                        uint16_t val = analogRead(KEY);
                                                        uint8_t k = _key_decide_key(val);
                                                        key_counter++;
                                                        if ((key_counter < 50)&&(k == KS_NONE))
                                                        {
                                                                if (set_state==SET_HOUR)
                                                                {
                                                                        set_state=SET_MINUTE;
                                                                }
                                                                else
                                                                {
                                                                        set_state=SET_HOUR;
                                                                }
                                                                key_state = KS_NONE;
                                                                key_counter = 0;                  
                                                        }
                                                        else if ((key_counter > 500)&&(k == KS_NONE))
                                                        {
                                                                clock.fillByYMD(clock.year,clock.month,clock.dayOfMonth);
                                                                clock.fillByHMS(hour,minute,0);
                                                                clock.setTime();
                                                                flag = 0;
                                                                key_state = KS_NONE;
                                                                key_counter = 0;  
                                                        }
                                                        
                                                        break;
                                                default:
                                                        break;
                                                }
                                        }
                                        if (set_state==SET_HOUR)
                                        {
                                                clock.getTime();
                                                if (clock.second%2 == 1)
                                                {  
                                                        sprintf(string_buff,"$$:%02d",minute); 
                                                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,pCurrentFont); 
                                                }
                                                else
                                                {
                                                        sprintf(string_buff,"%02d:%02d",hour,minute); 
                                                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,pCurrentFont); 
                                                }  
                                        }
                                        else
                                        {
                                                clock.getTime();
                                                if (clock.second%2 == 1)
                                                {  
                                                        sprintf(string_buff,"%02d:$$",hour); 
                                                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,pCurrentFont); 
                                                }
                                                else
                                                {
                                                        sprintf(string_buff,"%02d:%02d",hour,minute); 
                                                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,pCurrentFont); 
                                                }  
                                        }
                                        DisPlay(UserData.brightness);
                          }
                  }
                  
                  break;
          default:
                  break;
          }
  }
  
  if (Serial.available()>0)
  {
          if (RX_num != Serial.available())
          {
                  RX_num = Serial.available();
          }
          else if (RX_num == Serial.available())
          {
                  UartTimeout++;
          }
          if (UartTimeout > 2)
          {
                  //Serial.println(Serial.available(),DEC);
                  Serial.readBytes(RX_buff, RX_num);
                  if ((RX_buff[0]==0xaa)&&(RX_buff[1]==0xbb))
                  {
                          switch (RX_buff[2])
                          {
                          case 0x01:
                                  if (((0<=RX_buff[3])&&(RX_buff[3]<=99))
                                     &&((0<RX_buff[4])&&(RX_buff[4]<=12))
                                     &&((0<RX_buff[5])&&(RX_buff[5]<=31)))
                                  {
                                          clock.fillByYMD(RX_buff[3]+2000,RX_buff[4],RX_buff[5]);
                                          clock.fillByHMS(clock.hour,clock.minute,clock.second);
                                          clock.setTime();
                                          Serial.write(RX_buff, RX_num);
                                  }
                                  break;
                          case 0x02:
                                  if (((0<=RX_buff[3])&&(RX_buff[3]<=23))
                                     &&((0<=RX_buff[4])&&(RX_buff[4]<=59))
                                     &&((0<=RX_buff[5])&&(RX_buff[5]<=59)))
                                  {
                                          clock.fillByYMD(clock.year,clock.month,clock.dayOfMonth);
                                          clock.fillByHMS(RX_buff[3],RX_buff[4],RX_buff[5]);
                                          clock.setTime();
                                          Serial.write(RX_buff, RX_num);
                                  }
                                  break;
                          case 0x03:
                                  if ((0<=RX_buff[3])&&(RX_buff[3]<=256))
                                  {
                                          UserData.brightness = RX_buff[3];
                                          SetUserData();
                                          Serial.write(RX_buff, RX_num);
                                  }                       
                                  break;     
                           case 0x04:
                                  if ((0<=RX_buff[3])&&(RX_buff[3]< DISPLAY_MODE_MAX))
                                  {
                                          UserData.displaymode = RX_buff[3];
                                          SetUserData();
                                          Serial.write(RX_buff, RX_num);
                                          memset(DisPlayBuff,0xff,32);
                                  }                       
                                  break;                                      
                          default:
                                  break;
                          }
                  }
                  UartTimeout = 0;   
                  RX_num = 0;        
          }  
  }

  clock.getTime();
        if (UserData.displaymode == 0x00)
        {
                 if (clock.second%2 == 1)
                {  
                        sprintf(string_buff,"%02d:%02d",clock.hour,clock.minute); 
                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,font_6x8_led); 
                }
                else
                {
                        sprintf(string_buff,"%02d %02d",clock.hour,clock.minute); 
                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,font_6x8_led); 
                }    
        }
        else if (UserData.displaymode == 0x01)
        {
                 if (clock.second%2 == 1)
                {  
                        sprintf(string_buff,"%02d:%02d",clock.hour,clock.minute); 
                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,font_6x8_led1); 
                }
                else
                {
                        sprintf(string_buff,"%02d %02d",clock.hour,clock.minute); 
                        fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),1,0,font_6x8_led1); 
                }   
        }        
        else if (UserData.displaymode == 0x02)
        {
                sprintf(string_buff,"%02d:%02d:%02d",clock.hour,clock.minute,clock.second); 
                fb_draw_string(DisPlayBuff,string_buff,strlen(string_buff),2,1,font_3x5_led); 
        }
  DisPlay(UserData.brightness);
}
