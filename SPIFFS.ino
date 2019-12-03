#include "FS.h"
#include "SPIFFS.h"
#include "ESP32_DEF.h"

#define CFGFNAME "/jjy.cfg"
const char *t_fhz[] = { "1", "2" };

/******************************************/
/* SPIFFS初期化　 　                        */
/******************************************/
void init_SPIFFS()
{
  SPIFFS.begin(true);
}
/******************************************/
/* SPIFFS終了 　 　                        */
/******************************************/
void end_SPIFFS()
{
  SPIFFS.end();
}
/******************************************/
/* Configファイル読み込み                     */
/******************************************/
int  cfg_load()
{
  File fp;
  char b[2];
  
  fp = SPIFFS.open(CFGFNAME, FILE_READ); // 読み取り
  if( !(fp) ) {
    Serial.print(CFGFNAME);
    Serial.print(" File Exist\n");
    return( -1 );
  }
  else {
    fp.read((uint8_t *)b, 2); // JJY 40K('1') or 60K('2') 
    jjyhz = ( b[0] == '1' ? 40 : 60 );
    fp.read((uint8_t *)ssid, sizeof(ssid)); // WiFi SSID
    fp.read((uint8_t *)password, sizeof(password)); // WiFi Password          
    fp.read((uint8_t *)ntp1, sizeof(ntp1)); // NTP Server 1 
    fp.read((uint8_t *)ntp2, sizeof(ntp1)); // NTP Server 2
    fp.close();
//    Serial.printf("SSID %s  PW   %s\n", ssid, password );  
//    Serial.printf("NTP1 %s  NTP2 %s\n", ntp1, ntp2 ); 
  }
  return( 0 );
}
/******************************************/
/* Configファイル作成                       */
/******************************************/
void cfg_crt()
{
  File fp;

  fp = SPIFFS.open(CFGFNAME, FILE_WRITE); // 書き込み、存在すれば上書き
  if( !(fp) ) {
    Serial.print(CFGFNAME);
    Serial.print(" Open Faild Abort!!\n");
    while( 1 ) delay(100);
  }
  else {
    fp.write((uint8_t *)t_fhz[(jjyhz == 40 ? 0 : 1)],2 );
    clr_isgraph( ssid, sizeof(ssid));
    fp.write((uint8_t *)ssid, sizeof(ssid));
    clr_isgraph( password, sizeof(password));
    fp.write((uint8_t *)password, sizeof(password));
    clr_isgraph( ntp1, sizeof(ntp1));
    fp.write((uint8_t *)ntp1, sizeof(ntp1));
    clr_isgraph( ntp2, sizeof(ntp2));
    fp.write((uint8_t *)ntp2, sizeof(ntp2));
    fp.close();
  }
}
/******************************************/
/* 表示キャラクタ以外を削除する処理            */
/* スペースを含み表示できない文字コードが       */
/* 出てきたらそこから後ろはNULL文字で埋める     */
/******************************************/
void  clr_isgraph( char d[], size_t s )
{
  int i;
  
  for( i = 0; i < s; i++ ) {
    if( isgraph(d[i]) == 0 ) break;
  }
  for(      ; i < s; i++ ) {
    d[i] = '\0';
  }
}
/*******/
/* EOF */
/*******/
