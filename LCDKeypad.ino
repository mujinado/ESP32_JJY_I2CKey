#include <time.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <LiquidCrystal.h>
#include "ESP32_DEF.h"

/******************************************/
/* Define                                 */
/******************************************/
// Key 分類
#define KEYRIGHT  0
#define KEYUP     1
#define KEYDOWN   2
#define KEYLEFT   3
#define KEYSELECT 4
#define KEYNONE   5
#define KEYERR    -1
#define KEY_AD  36  // analog input oprt 
// A/D input Threshold
#define THRIGHT  0
#define THUP     100
#define THDOWN   500
#define THLEFT   900
#define THSELECT 1600
#define THNONE   1900
#define BUFEND( a ) ( a - 2 )
/* LCD Assign */
#define LCD_RS  16
#define LCD_RW  27
#define LCD_EN  17
#define LCD_D4  26
#define LCD_D5  25
#define LCD_D6  33
#define LCD_D7  32
LiquidCrystal lcd(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Adafruit_RGBLCDShield Alcd = Adafruit_RGBLCDShield();
/* LCD CONTRAST PWM */
#define LCD_CONTRAST  12
#define LEDC_CHANNEL  0      // LEDC channel (0-15)
#define LEDC_BASE_FREQ 5000 // use 4900 Hz as a LEDC base frequency  
#define LEDC_TIMER_BIT 8   // use 13 bit precission for LEDC timer

/******************************************/
/* Global variable                        */
/******************************************/
static int c_sta; // 処理カウンター　0:menu 1:周波数選択 2:WiFi入力メニュー 3:SSID 4:Password 5:NTPメニュー 6:NTP1 7:NTP2
static int c_mnu; // 選択メニュー番号(カラム方向カウンター)
static int c_edt; // 文字列編集の編集している文字の場所
static int c_dst; // 文字列表示の初めの位置
const char t_inp[] =  " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.!#$%&()-+:*_/\@[]^";
// t_titテーブルの最長文字数が変わった場合にはprn_stream関数のbuf[](バッファー)サイズ変更必須
//         10        20        30        40        50       60        70
//123456789012345678901234567890123456789012345678901234567890123456789
const char *t_tit[] = { "MENU H=Hz A=AP N=NTP E=END Move:LEFT/RIGHT and Push:SELECT",
                        "JJY Frequency Move:LEFT/RIGHT and Push:SELECT",
                        "MENU SID=SSID PW=Password E=END Move:LEFT/RIGHT and Push:SELECT",
                        "SSID MAX 32Byte UP/DOWN LEFT/RIGHT and Push:SELECT",
                        //         10        20        30        40        50
                        //123456789012345678901234567890123456789012345678901234567890
                        "Password MAX 32Byte UP/DOWN LEFT/RIGHT and Push:SELECT",
                        "MENU Move:LEFT/RIGHT and Push:SELECT",
                        "NTP FQDN MAX 32Byte UP/DOWN LEFT/RIGHT and Push:SELECT",
                        "NTP FQDN MAX 32Byte UP/DOWN LEFT/RIGHT and Push:SELECT"
                      };
const char *t_men[] = { "1:H 2:A 3:N 4:E ", "1:40kHz 2:60kHz ", "1:SID 2:PW 3:E  ", &ssid[0], &password[0], "1:NTP 2:NTP 3:E ", &ntp1[0], &ntp2[0] };
const char t_pos[][16] = {// Cursol Potion Table
  //  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
  { 0, 4, 8, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 6, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  { 0, 6, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
};
//c_sta  0  1  2  3  4  5  6  7
const char t_mnu[] = { 3, 1, 2, 0, 0, 2, 0, 0 };// メニュー選択数　０から始まるのでカウントは-1した数
const char t_sta[][16] = {// Status Control Table
  //  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
  { 1, 2, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
  { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 },
  { 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
  { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 }
};
const int t_hz[] = { 40, 60 };//周波数テーブル
const char t_ntp1[] = "ntp.nict.jp";
const char t_ntp2[] = "ntp.jst.mfeed.ad.jp";
//const char t_ntp2[] = "ntp.ring.gr.jp";
const char *t_dow[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
const char *t_st1[] = { // t_st1[]とt_st2[]は同じ数の文字列を定義必須
  //1234567890123456
  "JJY generator   ",
  "JJY generator   ",
  ""
};
const char *t_st2[] = {
  //1234567890123456
  "SELECT to EDIT  ",
  "Wait NTP Sync   ",
  "Wait JJY Start  "
};
uint8_t t_gij[3][8] = { //シンボルマーク外字テーブル
   // Clock Sync None X
    { B00000,
      B00000,
      B00000,
      B00000,
      B10010,
      B01100,
      B01100,
      B10010 },
    // Clock Sync RTC RT
    { B11100,
      B10100,
      B11100,
      B11000,
      B10100,
      B01110,
      B00100,
      B00100 },
    // Clock Sync NTP NT
    { B10010,
      B11010,
      B10110,
      B00000,
      B01110,
      B00100,
      B00100,
      B00000 }
};
/******************************************/
/* Config編集機能初期化                      */
/******************************************/
void Config_setup(int w)
{
  init_lcd();//LCD イニシャライズ
  if ( (w == 0) || (src_hz( jjyhz ) == -1) ) { // 全くデーターが無い状態からのスタートの処理
    jjyhz = t_hz[0];
    memset( ssid, '\0', sizeof( ssid ));
    memset( password, '\0', sizeof( password ));
    memset( ntp1, '\0', sizeof( ntp1 ));
    strcpy( ntp1, t_ntp1 );
    memset( ntp2, '\0', sizeof( ntp2 ));
    strcpy( ntp2, t_ntp2 );
  }
}
/******************************************/
/* LCD初期化　　　　　                        */
/******************************************/
void init_lcd()
{
  int i;
  lcd.begin(16, 2);              //LCDの初期化
  delay(10);
  Alcd.begin(16, 2);
  for( i = 0; i < 3; i++ ) {
    lcd.createChar(i + 1, t_gij[i]);// 外字登録
  }
  lcd.setCursor(0, 0);           //カーソルの移動
  lcd.cursor();
  lcd.blink();
  init_PWM();
}

void init_PWM()
{
  ledcSetup(LEDC_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  ledcAttachPin(LCD_CONTRAST, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, 0x7f);
}
/******************************************/
/* Keypad 入力処理　                        */
/******************************************/
const struct {
  short dat; // A/D入力閾値
  int   ret; // 戻り値
} t_key[5] = {
  { BUTTON_SELECT,KEYSELECT },
  { BUTTON_UP    ,KEYUP    },
  { BUTTON_DOWN  ,KEYDOWN  },
  { BUTTON_LEFT  ,KEYLEFT  },   
  { BUTTON_RIGHT ,KEYRIGHT }
};
const char *ink[] = {
  "RIGHT", "UP", "DOWN", "LEFT", "SELECT", "NONE"
};
int red_key()
{
  int i;
  int r;
  uint8_t key;

  key = Alcd.readButtons();
//Serial.printf("KEY %d : %s\n", key, ink[r] );
  for( i = 0; i < 5; i++ ) {
    if( key & t_key[i].dat ) {
      r = t_key[i].ret;
      break;      
    }
  }
  if( i == 5 ) r = KEYNONE;
Serial.printf("KEY %d : %s\n", key, ink[r] );
  return( r );                    
}
#if 0
/**************************************/
/* キー入力処理                         */
/*************************************/
int read_buttons( void )
{
  int r;
  int i;

  i = 0;
  do {
    r = red_key();// Key入力
    i++;
    if ( i > 500 ) {
      prn_stream( (char *)NULL );// メニュー再表示
      i = 0;
    }
  } while ( r == KEYNONE ); // THNONE以上はボタンが押されていない
  return ( r );
}
/******************************************/
/* Keypad 入力処理　                        */
/******************************************/
const struct {
  short dat; // A/D入力閾値
  int   ret; // 戻り値
} t_key[6] = {
  { THNONE  ,KEYNONE   },
  { THSELECT,KEYSELECT },
  { THLEFT  , KEYLEFT  },
  { THDOWN  , KEYDOWN  },
  { THUP    , KEYUP    },   
  { THRIGHT , KEYRIGHT }
};
const char *ink[] = {
  "RIGHT", "UP", "DOWN", "LEFT", "SELECT", "NONE"
};
int red_key()
{
  int i;
  int r;
  int key;

  key = get_ad( KEY_AD );
  for( i = 0; i < 5; i++ ) {
    if( key > t_key[i].dat ) {
      r = t_key[i].ret;
      break;      
    }
  }
  if( i == 5 ) r = t_key[i].ret;
//Serial.printf("KEY %d : %s\n", key, ink[r] );
  return( r );                    
}

int get_ad( int p )
{
  int i;
  int d1, d2;

  do {
    d1 = analogRead( KEY_AD );
    delay( 1 );
    d2 = analogRead( KEY_AD );
  } while( abs( d1 - d2 ) > 100 );
  return( (d1 + d2) / 2 );
}
#endif
#if 0
const struct {// Key分類テーブル
  short dat; // A/D入力閾値
  int   ret; // 戻り値
} t_key[5] = {
  { THRIGHT , KEYRIGHT },
  { THUP    , KEYUP    },
  { THDOWN  , KEYDOWN  },
  { THLEFT  , KEYLEFT  },
  { THSELECT, KEYSELECT }
};
int red_key(int key)
{
  int i;
  int r;

  r = KEYERR;
  if ( key > THNONE ) r = KEYNONE;
  else {
    for ( i = 0; i < 5; i++ ) {
      if ( key < t_key[i].dat ) {
        r = t_key[i].ret;
        break;
      }
    }
  }
  return ( r );
}
#endif
/**************************************/
/* キー入力処理                         */
/*************************************/
int read_buttons( void )
{
  int r;
  int i;

  i = 0;
  do {
    r = red_key();// Key入力
    i++;
    if ( i > 500 ) {
      prn_stream( (char *)NULL );// メニュー再表示
      i = 0;
    }
  } while ( r == KEYNONE ); // THNONE以上はボタンが押されていない
  return ( r );
}
/**************************************/
/* SELECTボタンチエック                  */
/* 起動時にSELECEボタンが押されていた場合に */
/* 設定変更モードへ移行するための処理       */
/*************************************/
int chk_SEL( void )
{
  int r;
  int i, j;

  i = j = 0;
  while ( 1 ) {
    do {
      r = red_key();// Key入力
      i++;
      if ( i > 5000 ) { //10秒待って入力が無ければ通常起動へ
        i = -1;
        r = 0;
        break;
      }
    } while ( r == KEYNONE ); // ボタンが押されていない
    if( i < 0 ) break;
    if ( r == KEYSELECT ) {
      j++;
    }
    if ( j > 100 ) { // 1秒以上SELECTキーが押されている事の判定
      r = 1;
      delay( 500 );
      break;
    }
  }
  return ( r );
}
/******************************************/
/* LCD表示処理　　　                        */
/******************************************/
void disp()
{
  lcd.clear();
  prn_stream((char *)t_tit[c_sta] );
  lcd.setCursor(0, 1);    //カーソルの移動
  lcd.print( &t_men[c_sta][c_dst] );     //文字の出力
  lcd.setCursor(t_pos[c_sta][c_mnu], 1); //カーソルの移動
}
/******************************************/
/* 入力説明文の表示が長いので流れる表示にする      */
/******************************************/
void prn_stream( char *s )
{
  static char *a = NULL;
  static int st;
  static char buf[70 + 16];
  static int n;
  int i;
  char  b[17];

  if ( (a != s) && (s != NULL) ) { // Start
    st = 0;
    a = s;
    /* 内部バッファーに文字列を格納後ろはスペースで埋める */
    for ( i = n = 0; i < (sizeof( buf ) - 1); i++, n++ ) {
      buf[i] = s[i];
      if ( s[i] == '\0' ) break;
    }
    for ( ; i < (sizeof( buf ) - 1); i++ ) {
      buf[i] = 0x20;
    }
    buf[(sizeof( buf ) - 1)] = '\0';
  }
  else {// 再表示の表示開始位置の計算
    st++;
    if ( st == (n - 1) ) st = 0;
  }
  strncpy( b, &buf[st], 16 );//表示文字列のコピー
  b[16] = '\0';
  lcd.setCursor(0, 0); //カーソルの移動
  lcd.print( b );     //文字の出力
  lcd.setCursor(t_pos[c_sta][c_mnu], 1); //カーソルの移動
}
/******************************************/
/* Configパラメーター入力／編集メイン処理　      */
/******************************************/
void Config_loop()
{
  int key;

  c_sta = 0;// 入力処理ステージカウンター
  c_mnu = 0;// 入力パラメーター番号カウンター
  c_edt = 0;// 文字列編集位置カウンター
  c_dst = 0;// 表示先頭位置カウンター
  while ( 1 ) {
    disp();// LCDへタイトル＆データーを表示
    delay( 500 );
    key = read_buttons();// Key入力待ち 
    Serial.printf("1 STA %d  MNU %d\n", c_sta, c_mnu );
    switch ( c_sta ) {
      case  0://Main menu
        Serial.print("Main Menu\n");
        if ( sel_menu( key ) ) { // 3.END
          lcd.clear();
          cfg_crt();//ConfigFile Create
          return;
        }
        if ( c_sta == 1 ) c_mnu = src_hz(jjyhz); // 現在の周波数から入力パラメーター番号を作る
        break;
      case 1:// kHz Select
        Serial.print("kHz Select\n");
        sel_hz( key );
        break;
      case 2://WiFi input menu
        Serial.print("Wifi Menu\n");
        if ( sel_menu( key ) ) c_sta = 0;
        break;
      case  3://SSID input
        Serial.print("SSID input\n");
        inp_dat( key, &ssid[0] );
        break;
      case  4://Password input
        Serial.print("Password input\n");
        inp_dat( key, &password[0] );
        break;
      case 5://NTP input menu
        Serial.print("NTP Menu\n");
        if ( sel_menu( key ) ) c_sta = 0;
        break;
      case  6://NTP1 input
        Serial.print("NTP1 input\n");
        inp_dat( key, &ntp1[0] );
        break;
      case  7://NTP2 input
        Serial.print("NTP2 input\n");
        inp_dat( key, &ntp2[0] );
        break;
      default:
        c_sta = c_mnu = 0;
        break;
    }
  }
}
/********************************************/
/* メニュー選択の処理                          */
/* LEFT/RIGHTキーで左右方向の選択をする処理       */
/********************************************/
int  sel_menu( int k )
{
  int r = 0;

  Serial.printf("3 STA %d  MNU %d  KEY %d\n", c_sta, c_mnu, k );
  switch ( k ) {
    case  KEYRIGHT:
      c_mnu++;
      if ( c_mnu > t_mnu[c_sta] ) c_mnu = 0;
      break;
    case  KEYLEFT:
      c_mnu--;
      if ( c_mnu < 0 ) c_mnu = t_mnu[c_sta];
      break;
    case  KEYSELECT:
      Serial.printf("SELECT\n");
      //    if( c_mnu == t_mnu[c_sta] ) r = 1;
      //    c_sta += (c_mnu + 1);
      if ( c_mnu == t_mnu[c_sta] ) r = 1;
      c_sta = t_sta[c_sta][c_mnu];
      c_mnu = 0;
      break;
    default:
      break;
  }
  Serial.printf("5 STA %d  MNU %d  Ret %d\n", c_sta, c_mnu, r );
  return ( r );
}
/********************************************/
/* 周波数の選択処理                           */
/* LEFT/RIGHTキーで40Khz/60KHzを選択をする処理  */
/********************************************/
void  sel_hz( int k )
{
  switch ( k ) {
    case  KEYRIGHT:
      c_mnu++;
      if ( c_mnu > t_mnu[c_sta] ) c_mnu = 0;
      break;
    case  KEYLEFT:
      c_mnu--;
      if ( c_mnu < 0 ) c_mnu = t_mnu[c_sta];
      break;
    case  KEYSELECT:
      jjyhz = t_hz[c_mnu];// JJY Hz Set
      c_sta = t_sta[c_sta][c_mnu];
      c_mnu = 0;
      break;
    default:
      break;
  }
}
/********************************************/
/* SSID/PW/NTPデータの入力処理                 */
/********************************************/
void  inp_dat( int k, char b[] )
{
  static int j = 0;

  switch ( k ) {
    case KEYSELECT:
      c_sta = t_sta[c_sta][c_mnu];
      c_mnu = c_edt = c_dst = 0;
      //    c_sta = 2;
      break;
    case KEYRIGHT:
      if ( c_mnu < 15 ) {
        c_mnu++;
        c_edt++;
      }
      else if ( c_mnu == 15 ) { //右端に来た！
        if ( c_edt < BUFEND(sizeof( ssid )) ) {
          c_edt++;
          c_dst++;
        }
      }
      j = src_num( b[c_edt] );
      //    j = src_num( b[c_mnu] );
      break;
    case KEYLEFT:
      if ( c_mnu > 0 ) {
        c_mnu--;
        c_edt--;
      }
      else if ( c_mnu == 0 ) { // 左端に来た！
        if ( c_edt > 0 ) {
          c_edt--;
          c_dst--;
        }
      }
      j = src_num( b[c_edt] );
      //    j = src_num( b[c_mnu] );
      break;
    case KEYUP:
      if ( j < BUFEND(sizeof( t_inp ))) j++;
      else                             j = 0;
      b[c_edt] = t_inp[j];
      //    b[c_mnu] = t_inp[j];
      break;
    case KEYDOWN:
      if ( j > 0 ) j--;
      else        j = BUFEND(sizeof( t_inp ));
      b[c_edt] = t_inp[j];
      //    b[c_mnu] = t_inp[j];
      break;
    default:
      break;
  }
}
/********************************************/

/*********************************************/
/* 文字がt_inpテーブルの何番目かを調べる関数    　　*/
/********************************************/
int src_num( char d )
{
  int i;

  for ( i = 0; i < BUFEND(sizeof( t_inp )); i++ ) {
    if ( t_inp[i] == d ) return ( i );
  }
  return ( 0 );
}
/*********************************************/
/* 発信周波数が周波数テーブルの何番目かを調べる関数 　　*/
/*********************************************/
int src_hz( int hz )
{
  if ( hz == t_hz[1] ) return ( 1 );
  if ( hz == t_hz[0] ) return ( 0 );
  return ( -1 );
}

/*******************************************/
/* LCDに現在日時を表示する                    */
/******************************************/
static int tm_mday = -1;
static int tm_sec = -1;
//const char t_scsyc[] = { 0xEB, 'R', 'N' };
//const char t_scsyc[] = { 0xEB, 'R', 'N' };
void prn_sysclock()
{
  time_t t;
  struct tm *tm;
  char b[20];

  t = get_systime();
  tm = localtime( &t );
  if ( tm_sec != tm->tm_sec ) {
    tm_sec = tm->tm_sec;
    sprintf(b, "%02d:%02d:%02d(%02dkHz) ", tm->tm_hour, tm->tm_min, tm->tm_sec, jjyhz);
//    sprintf(b, "%02d:%02d:%02d(%02dkHz)%c", tm->tm_hour, tm->tm_min, tm->tm_sec, jjyhz, t_scsyc[f_sync]);
//    Serial.printf("%02d:%02d:%02d (%02dkHz)\n", tm->tm_hour, tm->tm_min, tm->tm_sec, jjyhz);
//    Serial.printf("%s\n", b );
    lcd.setCursor(0, 1); //カーソルの移動
    lcd.print( b );     //文字の出力
    lcd.setCursor(15, 1); //カーソルの移動
    lcd.write(f_sync + 1); // シンボルマーク表示
  }
  if ( tm_mday != tm->tm_mday ) {
    tm_mday = tm->tm_mday;
    sprintf(b, "%04d/%02d/%02d(%s) ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, t_dow[tm->tm_wday]);
    lcd.setCursor(0, 0); //カーソルの移動
    lcd.print( b );     //文字の出力
  }
}
/****************************************/
/* 動作ステータスをLCDに表示                */
/****************************************/
void prn_status(int n)
{
  lcd.setCursor(0, 0); //カーソルの移動
  lcd.print( t_st1[n] );     //文字の出力
  lcd.setCursor(0, 1); //カーソルの移動
  lcd.print( t_st2[n] );     //文字の出力
  tm_mday = -1;  //　時刻合わせの後の再表示のために初期化する
  tm_sec = -1;   //　時刻合わせの後の再表示のために初期化する
}
/*******/
/* EOF */
/*******/
