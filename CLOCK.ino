#include <time.h>
#include <sys/time.h>  // requried for timeval
#include "ESP32_DEF.h"

/* Clock */
#define JST     (3600 * 9)  // Japan Standerd time
//#define ADJCLK  (600)  // Adjust by 10 min
#define ADJCLK  (3600 * 6)  // Adjust by 6 Hour
#define ENVNAME "TZ"        // 環境変数
#define ENVTZ   "JST-9"     // 環境変数TZに設定する値（GTMからのオフセットを示す値）
/******************************************/
/* システム時間をNTP ServerもしくはRTCに合わせる*/
/* WiFiが接続できればNTP                     */
/* WiFiがつながらずRTCがあればRTC             */
/* 両方だめならそのまま                       */
/******************************************/
void adj_clk()
{
//  if( f_adj == OFF ) return; // Clock Adjust Flag check
  f_sync = 0;
  init_WiFi();// Wifi Connection
  if( f_wifi ) {// WiFiが接続できる場合にはNTPをマスタークロックとして同期する
    f_sync = 2;
    configTzTime(ENVTZ, ntp1, ntp2); // Sync NTP
//    configTime( JST, 0, ntp1, ntp2); // Sync NTP
    delay(1000);
    Serial.print("Adjust System Clock\n");
    if ( f_rtc ) { // RTC Clock有り?
      Serial.print("Adjust Real Time Clock\n");
      set_RTC();
    }
  }
  else if( f_rtc ) { // WiFiがつながらずRTCがある場合はRTCをマスタークロックとしてシステム時刻を設定する
     Serial.print("Adjust System Clock from RTC\n");
     f_sync = 1;
     set_sysclk();
  }
  end_WiFi();
  d_adj = get_systime();
  d_adj += ADJCLK;// 次にNTP同期させるタイミングをセット
  f_adj = OFF; // Clock Adjust Flag OFF
  prn_time2();
}
/******************************************/
/* RTCの時刻をシステムクロックに設定する        */
/******************************************/
void set_sysclk()
{
  struct timeval tv;
  struct tm dt;
  unsigned long sm;
  unsigned char  t[7];
  
  if( f_rtc == OFF ) return;
  while( 1 ) {
    sm = micros();
    get_RTC( t );
    dt.tm_sec = bcdToDec( t[0] );
    if( dt.tm_sec == 0 ) break;
    if( dt.tm_sec < 59 ) delay(1);
  }
/* データはBCDにて格納されている                  */
/* 6バイト目：年(+1900必要                      */
/* 5バイト目：月(+1必要)　                      */
/* 4バイト目：日                                */
/* 3バイト目：曜日(0:日曜)                       */
/* 2バイト目：時　　　　　　                      */
/* 1バイト目：分　　　　　　                      */
/* 0バイト目：秒　　　　　                       */
  dt.tm_min  = bcdToDec( t[1] );
  dt.tm_hour = bcdToDec( t[2] );
  dt.tm_mday = bcdToDec( t[4] );
  dt.tm_mon  = bcdToDec( t[5] );
  dt.tm_year = bcdToDec( t[6] ); 
  tv.tv_sec  = mktime( &dt );
  sm = micros() - sm;
  if( sm > 10000UL ) tv.tv_usec = 1000; // micros()関数がOverflowした可能性がある(10msecを超えたらおかしいと判断その場合実測値が1000μ秒ぐらいだったので1000とする)
  else               tv.tv_usec = sm;
//  if( sm > 100000UL ) tv.tv_usec = 0; // micros()関数がOverflowした可能性がある
//  else              tv.tv_usec = sm;
  Serial.printf("Micros %d\n", sm);
  settimeofday( &tv, NULL );
}
/******************************************/
/* システムCloakの補正が必要かどうか判定する    */
/******************************************/
int chk_clk()
{
  time_t tim;
  struct tm *tm;
  unsigned char t[7];
  int  rtc, dif;

  f_adj = OFF;
  tim = get_systime();
  if ( tim >= d_adj ) { // Sync NTP
    f_adj = ON;
  }
  else {
    if( f_rtc ) {  // RTCが有る場合
      get_RTC( t );      // RTC Clock Get
      rtc = bcdToDec( t[0] );
      tm = localtime( &tim );
      rtc++;          // 0-59 -> 1-60
      tm->tm_sec++;   // 0-59 -> 1-60
      dif = abs( rtc - tm->tm_sec );  // 時間差を計算
      if( dif > 1 && dif < 59 ) { // 2秒以上のズレが発生した場合にClockを合わせる
        f_adj = ON;
        Serial.printf("RTC != SystemClock!! %d %d %d\n", dif, tm->tm_sec, rtc );
      }
    }
  }
  return( f_adj );
}
/***************************/
/* Get System time         */
/**************************/
time_t get_systime()
{
  time_t t;
  
  while( 1 ) {
    t = time(NULL);
    if( t != -1 ) break;
    delay( 1 );
  }
  return( t );
}

/*******/
/* EOF */
/*******/
