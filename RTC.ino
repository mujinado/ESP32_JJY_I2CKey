#include <Wire.h>
#include "ESP32_DEF.h"

/* I2C Address */
#define I2C_SDA 21
#define I2C_SCL 22
#define CLOCK_ADDRESS 0x68 // RTC module DS3231 I2C Address
#define CLKADR   0x00  // Clock Data Start memory address (sec)
#define A1M1ADR  0x07  // A1M1 Flag memory address
#define A1M2ADR  0x08  // A1M2 Flag memory address
#define A1M3ADR  0x09  // A1M3 Flag memory address
#define A1M4ADR  0x0a  // A1M4 Flag memory address
#define A1M1BIT  0b10000000 // A1M1 Flag
#define A1M2BIT  0b10000000 // A1M2 Flag
#define A1M3BIT  0b10000000 // A1M3 Flag
#define A1M4BIT  0b10000000 // A1M4 Flag
#define DS3231CTL 0x0e      // DS3231 Control memory address
#define A1IEBIT  0b00000001 // A1 Interrupt Enable Bit
#define INTCNBIT 0b00000100 // Interrupt Enable Bit
#define DS3231STA 0x0f      // DS3231 Control/Status memory address
#define A1FBIT  0b00000001  // A1 Interrput FIN Bit
#define OSFBIT  0b10000000  // OSF Flag Bit(Second設定の時にこのフラグをOFFすることでRTCが有効となる）

/******************************************/
/* RTC初期設定                             */
/******************************************/
void RTC_setup()
{
  int r;
  
  Wire.begin();
  delay(100);
  Wire.beginTransmission(CLOCK_ADDRESS);
  r = Wire.endTransmission();
  if( r == 0 ) {
    f_rtc = ON;
    setClockMode_24h();
//    set_A1Mx();//1sec timeout
//    Set_SQWFrq(); // SQW Frequeny
    Serial.printf("RTC Device Installed\n");
  }
  else {
    Serial.printf("RTC Device NONE\n");
    f_rtc = OFF;
  }
}

/******************************************/
/* SQW端子に出る信号の周波数を設定            */
/******************************************/
void  Set_SQWFrq() // SQW Frequeny 
{
  byte  d;

  d = readControlByte( 0 );//0x0e Control
  writeControlByte((d & 0b11100111), 0);//RS2 RS1 00:1Hz
}
/******************************************/
/* A1IE信号のON　　　　　　　　　            */
/* A1Mx設定の条件で割り込みが発生する         */
/******************************************/
void  Set_A1IE()
{
  byte  d;

  d = readControlByte( 0 );//0x0e Control
  writeControlByte((d | A1IEBIT), 0);//A1IE S
//  writeControlByte((d | 0b00000001), 0);//A1IE S
}
/***********************************************/
/* A1F信号のOFF　　　　　　　　　                 */
/* A1Mx割り込みが発生したことを表すA1Fフラグをクリア */
/* これをクリアするまで次の割り込みは発生しない      */
/***********************************************/
void  Clear_A1F()
{
  byte  d;

  d = readControlByte( 1 );//0x0f Status
  writeControlByte((d & ~A1FBIT), 1);//A1F Clear
//  writeControlByte((d & 0b11111110), 1);//A1F Clear
}
/***********************************************/
/* RTCからデーターを読み出す　　　                 */
/* データーがunsigned char型のデーターが7バイト    */
/* データはBCDにて格納されている                  */
/* 6バイト目：年(+1900必要                      */
/* 5バイト目：月(+1必要)　                      */
/* 4バイト目：日                                */
/* 3バイト目：曜日(0:日曜)                       */
/* 2バイト目：時　　　　　　                      */
/* 1バイト目：分　　　　　　                      */
/* 0バイト目：秒　　　　　                       */
/***********************************************/
void get_RTC(unsigned char a[])
{
  int i;
  
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(CLKADR));
  Wire.endTransmission();
  Wire.requestFrom(CLOCK_ADDRESS, 7);

  for( i = 0; i < 7; i++ ) {
    a[i] = Wire.read();
  }
}
/***********************************************/
/* System Clockの日時をRTCへ設定                 */
/***********************************************/
void set_RTC()
{
  time_t ta, tb;
  struct tm *tm;

  tb = get_systime();
  while( 1 ) {
    ta = get_systime();
    if( ta != tb ) break;
  }
  tb = ta;
  while( 1 ) {
    ta = get_systime();
    if( ta != tb ) break;
  }
  tm = localtime( &ta );
  setSecond( tm->tm_sec );
  setMinute( tm->tm_min );
  setHour( tm->tm_hour );
  setDoW( tm->tm_wday );
  setDate( tm->tm_mday );
  setMonth( tm->tm_mon );
  setYear( tm->tm_year );
}
/***************************************/
/* RTC Second data Set                 */
/***************************************/
void  setSecond(byte Second)
{
  byte b;
  
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(CLKADR));
  Wire.write(decToBcd(Second)); 
  Wire.endTransmission();
  // Clear OSF flag
  b = readControlByte( 1 );
//  b &=  OSFBIT;
//  b &=  0b01111111;
  writeControlByte(( b & ~OSFBIT), 1 );
}
/***************************************/
/* RTC Minute data Set                 */
/***************************************/
void setMinute(byte Minute)
{
  // Sets the minutes 
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x01));
  Wire.write(decToBcd(Minute)); 
  Wire.endTransmission();
}
/***************************************/
/* RTC Hour data Set                   */
/***************************************/
void setHour(byte Hour)
{
  Hour = decToBcd(Hour) & 0b10111111;
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x02));
  Wire.write(Hour);
  Wire.endTransmission();
}
/***************************************/
/* RTC Day of Week data Set            */
/***************************************/
void setDoW(byte DoW)
{
  // Sets the Day of Week
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x03));
  Wire.write(decToBcd(DoW));  
  Wire.endTransmission();
}
/***************************************/
/* RTC Date data Set                   */
/***************************************/
void setDate(byte Date)
{
  // Sets the Date
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x04));
  Wire.write(decToBcd(Date)); 
  Wire.endTransmission();
}
/***************************************/
/* RTC Month data Set                 */
/***************************************/
void setMonth(byte Month)
{
  // Sets the month
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x05));
  Wire.write(decToBcd(Month));  
  Wire.endTransmission();
}
/***************************************/
/* RTC Year data Set                   */
/***************************************/
void setYear(byte Year)
{
  // Sets the year
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x06));
  Wire.write(decToBcd(Year)); 
  Wire.endTransmission();
}
/***************************************/
/* RTC 24h Clock Mode Set              */
/***************************************/
void setClockMode_24h()
{
  byte b;

  // Start by reading byte 0x02.
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x02));
  Wire.endTransmission();
  Wire.requestFrom(CLOCK_ADDRESS, 1);
  b = Wire.read();
  b &= 0b10111111;
  // Write the byte
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(uint8_t(0x02));
  Wire.write( b );
  Wire.endTransmission();
}
/***************************************/
/* RTC Control data Read               */
/* 0:Control reg 1:Status reg          */
/***************************************/
const uint8_t t_cla[2] = { DS3231CTL, DS3231STA };
byte  readControlByte(bool w)
{
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(t_cla[w]);
  Wire.endTransmission();
  Wire.requestFrom(CLOCK_ADDRESS, 1);
  return( Wire.read() ); 
}
/***************************************/
/* RTC Control data Set                */
/* 0:Control reg 1:Status reg          */
/***************************************/
void  writeControlByte(byte control, bool w)
{
  Wire.beginTransmission(CLOCK_ADDRESS);
  Wire.write(t_cla[w]);
  Wire.write(control);
  Wire.endTransmission();
}
/***************************************/
/* A1Mx Flags Set                      */
/* 1秒でsqw信号が出るように設定            */
/***************************************/
const struct {
  uint8_t adr;
  byte    dat;
} t_axm[4] = {{ A1M1ADR, A1M1BIT },
              { A1M2ADR, A1M2BIT },
              { A1M3ADR, A1M3BIT },
              { A1M4ADR, A1M4BIT }
              }; 
void set_A1Mx()
{
  int i;
  byte  d;

  for( i = 0; i < 4; i++ ) {
    Wire.beginTransmission(CLOCK_ADDRESS);
    Wire.write(t_axm[i].adr);
    Wire.endTransmission();
    Wire.requestFrom(CLOCK_ADDRESS, 1);
    d = Wire.read();
    d |= t_axm[i].dat;
    // Write the byte
    Wire.beginTransmission(CLOCK_ADDRESS);
    Wire.write(t_axm[i].adr);
    Wire.write( d );
    Wire.endTransmission();
  }
}
/***************************************/
/* DEC to BCD                          */
/* Only 1 Byte                         */
/***************************************/
byte decToBcd(byte val) {
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}
/***************************************/
/* BCD to DEC                          */
/* Only 1 Byte                         */
/***************************************/
byte bcdToDec(byte val) {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}
/*******/
/* EOF */
/*******/
