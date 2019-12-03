#include <WiFi.h>
#include "ESP32_DEF.h"

#define TDELAY  500 //1回のディレータイム500msec
#define TIMEOUT 30  //WiFiがつながるまでのタイムアウト時間30秒
#define TCONT( a )  ((30 * 1000)/TDELAY) // 30秒になるまでのカウント数を計算するマクロ(秒X1000msec/1回のディレ時間)

//int f_wifi;
/******************************************/
/* WiFi Connect                           */
/******************************************/
void init_WiFi()
{
  int i;
  
  f_wifi = OFF;
  Serial.printf("Start WiFi Connect ");
  WiFi.begin(ssid, password);
  delay(TDELAY);
  for( i = 0; i < TCONT( TDELAY ); i++ ) {
    if( WiFi.status() == WL_CONNECTED ) {
      f_wifi = ON;
      break;
    }
    Serial.print('.');
    delay(TDELAY);
  }
  if( f_wifi ) {
    Serial.printf("Connected, IP address: ");
    Serial.println(WiFi.localIP());
  }
  Serial.print('\n');
}
/******************************************/
/* WiFi Disconnect                        */
/******************************************/
void  end_WiFi()
{
  WiFi.disconnect();
}
/*******/
/* EOF */
/*******/
