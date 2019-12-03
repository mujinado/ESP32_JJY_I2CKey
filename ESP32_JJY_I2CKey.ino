#include <time.h>
#include "freertos/task.h"
#include "ESP32_DEF.h"

/******************************************/
/* Define                                 */
/******************************************/
/* hard ware devices */
/* Degital OUT */
#define ONBLED  2
#define D4      4
/*********************/
#define BUFSIZE 33
/* Multi Task */
#define TASKSTACK (4096) // Task Stack Size
/* Timer (minimum time = 1usec) */
#define MSEC100  (100000)  // 100 msec
#define MSEC900  (900000)  // 900 msec
#define SEC1  (1000000)    // 1 sec
#define INTTIM1 (MSEC100)  // Timer1 Interrupt Cycle
#define INTTIM2 (MSEC900)  // Timer2 Interrupt cycle
/* JJY Puls Generation processing */
#define PULSON  5 // JJY ON Signal count(x 100msec)
#define PULSOFF 8 // JJY OFF Signal count(x 100msec)
#define PULSMK  2 // JJY Maker Signal count(x 100msec)
#define JJYDO  D4 // JJY Signal Digital OUT Port
/******************************************/
/* Global variable                        */
/******************************************/
/* WiFi parameter */
//const char* ssid = "aterm-8e6622-g";
//const char* password = "1cda6fa5dfddb";
char ssid[BUFSIZE];//SSID
char password[BUFSIZE];//Password
/* Timer Interrupt */
hw_timer_t *timer1 = NULL;    // For measurement
hw_timer_t *timer2 = NULL;    // For time adjustment
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
/* NTP Servers */
char ntp1[BUFSIZE];
char ntp2[BUFSIZE];
time_t  d_adj; // Adjust Time
/* JJY Puls Generation processing */
volatile int f_jjy; // JJY Signal Start Flag
volatile int f_sig; // JJY Digital out Singnal
volatile int d_sel; // JJY Buffer Selecter
volatile int d_sec; // JJY Timer(sec)
//volatile int c_jjy; // JJy Signal ON/OFF(100msec Counter time=d_jjy x 100msec)
unsigned char  d_jjy[2][60];// JJY Counter data Signal ON/OFF(100msec Counter time=d_jjy x 100msec)
int jjyhz;//周波数
int f_rtc;   // ON:RTCあり OFF:RTC無し
int f_wifi;  // ON:WiFiConnecting OFF:
int f_adj;   // Clock Adjust Flag 0:Adjust不要 1:Adjust必要
int f_sync;  // System Clock Sync Flag 0:NONE Sync 1:RTC Sync 2:NTP Sync

/******************************************/
/* Dual Task MAIN                         */
/******************************************/
void task0(void* param)
{
  int set;
  int sec = 0;

  set = OFF; // データセット済フラグOff
  while (1) {
    while (2) {
      if ((sec != 58) || (set == ON)) vTaskDelay(1); // 1 tic Task Delay
      sec = get_sec(); // Get localtime sec
      if ( sec == 59 ) { // スタート1秒前？
        if ( set == OFF ) break;
      }
      else set = OFF;
    }
    timerRestart(timer2);// 900msec 割り込みスタート
    f_jjy = ON;//JJY Signal Start
    gen_jjytable((d_sel ? 0 : 1));
    set = ON;
  }
}
/******************************************/
/* Get Local time sec                     */
/******************************************/
int  get_sec()
{
  time_t t;
  struct tm *tm;

  t = get_systime();
  tm = localtime(&t);
  return ( tm->tm_sec );
}

/******************************************/
/* DEBUG用のルーチン                        */
/* Print Local Time                       */
/******************************************/
static const char *wd[7] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat"};
void prn_time(time_t t)
{
  struct tm *tm;

  tm = localtime(&t);
  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d %d\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, wd[tm->tm_wday],
                tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_yday);
}

/******************************************/
/* DEBUG用のルーチン                        */
/* Print Local Time 2                     */
/******************************************/
void prn_time2()
{
  time_t t;
  struct tm *tm;

  t = get_systime();
  tm = localtime(&t);
  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d %d\n",
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, wd[tm->tm_wday],
                tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_yday);
}

/******************************************/
/* Generate JJY counter table             */
/******************************************/
int gen_jjytable(int n)
{
  time_t t;
  struct tm *tm;
  int pa1, pa2;// パリティ pa1:hour pa2:min

  t = get_systime() + 1; // 次の60秒のための情報を作るために1秒進める
  tm = localtime(&t);

  clr_jjy( &d_jjy[n][0] );// JJY Buffer Initial

  // JJYバッファに分データーをセット
  pa2 = BCDtoJJY(&d_jjy[n][1], BintoBCD( tm->tm_min ), 8);
  d_jjy[n][1] = d_jjy[n][2];
  d_jjy[n][2] = d_jjy[n][3];
  d_jjy[n][3] = d_jjy[n][4];
  d_jjy[n][4] = PULSOFF;

  // JJYバッファに時データーをセット
  pa1 = BCDtoJJY(&d_jjy[n][11], BintoBCD( tm->tm_hour ), 8);
  d_jjy[n][12] = d_jjy[n][13];
  d_jjy[n][13] = d_jjy[n][14];
  d_jjy[n][14] = PULSOFF;

  // JJYバッファに通算日データーをセット
  BCDtoJJY(&d_jjy[n][22], BintoBCD(tm->tm_yday + 1), 10); //通算日は1/1を含まないので+1する
  d_jjy[n][33] = d_jjy[n][31];
  d_jjy[n][32] = d_jjy[n][30];
  d_jjy[n][31] = d_jjy[n][29];
  d_jjy[n][30] = d_jjy[n][28];
  d_jjy[n][29] = PULSMK;
  d_jjy[n][28] = d_jjy[n][27];
  d_jjy[n][27] = d_jjy[n][26];
  d_jjy[n][26] = d_jjy[n][25];
  d_jjy[n][25] = d_jjy[n][24];
  d_jjy[n][24] = PULSOFF;

  // JJYバッファに年データーをセット
  BCDtoJJY(&d_jjy[n][41], BintoBCD(tm->tm_year - 100), 8);

  // JJYバッファに曜日データーをセット
  BCDtoJJY(&d_jjy[n][50], BintoBCD(tm->tm_wday), 3);

  if (pa1) d_jjy[n][36] = PULSON;//時パリティ
  if (pa2) d_jjy[n][37] = PULSON;//分パリティ

#if DEBUG
  prn_time2();
  prn_buf(n);
#endif
}
/***************************************/
/* DEBUG用のルーチン                     */
/* JJY Bufferの内容を分かりやすく表示する   */
/***************************************/
void prn_buf(int n)
{
  int i;

  for (i = 0; i < 60; i++ ) {
    switch ( d_jjy[n][i] ) {
      case PULSMK://JJY Maker Signal count(x 100msec)
        Serial.print("M");
        break;
      case PULSON://JJY ON Signal count(x 100msec)
        Serial.print("1");
        break;
      case PULSOFF://JJY OFF Signal count(x 100msec)
        Serial.print("0");
        break;
      default:
        Serial.print("E");
        break;
    }
  }
  Serial.print("\n");
}
/*****************************************/
/* Covert Binary to BCD                  */
/*****************************************/
int BintoBCD(int d)
{
  int a, b, c;

  a = d / 100;
  d -= a * 100;
  b = d / 10;
  c = d - (b * 10);

  a <<= 8;
  b <<= 4;
  return (a | b | c);
}
/*****************************************/
/* Covert BCD to JJY timer counter       */
/* JJY Counter : 100msec x counter       */
/* 戻り値としてSUMを返す                    */
/*****************************************/
int BCDtoJJY(unsigned char b[], int d, int n)
{
  int i, j, k;

  j = 0x01;
  j <<= (n - 1);
  for (i = k = 0; i < n; i++) {
    if (d & j) {
      b[i] = PULSON;
      k++;
    }
    else     b[i] = PULSOFF;
    j >>= 1;
  }
  return (k % 2);
}
/******************************************/
/* Timer Interrupt Sub Routine            */
/******************************************/
/******************************************/
/* Timer1 Interrupt Sub Routine           */
/* 100 msec毎に割り込み、10回で1秒            */
/* d_jjy[d_sel][秒]にJJY信号を出している      */
/* 時間が入ってので割り込み毎に減算して0になれば  */
/* 信号OFF                                 */
/* d_selは現在出力中のバッファーを示している     */
/******************************************/
void IRAM_ATTR INT_Timer1()
{
  int n;

  n = d_sec / 10; // d_secは100msec毎にカウントアップされるので10で割ることで「秒」となる
  if ( d_jjy[d_sel][n] > 0 ) { // JJY Counter in cycle ?
    f_sig = HIGH; // JJY Signal ON
    d_jjy[d_sel][n]--;  // JJY Counter Dec
  }
  else {  // 1 Cycle Fin
    f_sig = LOW;  // JJY Signal OFF
  }
  digitalWrite(JJYDO, f_sig); // JJY Signal ON
  digitalWrite(ONBLED, f_sig);
  d_sec++;  // 100msec interrupt Conter
}
/******************************************/
/* Timer2 Interrupt Sub Routine           */
/* 59秒からタイマーをスタートさせ900 msecで　　 */
/* に割り込み発生　　　　　　　　　　　　　　　　*/
/* 100msecタイマーを起動してTimer1割り込み処理 */
/* に処理を引き継ぐ                         */
/******************************************/
void IRAM_ATTR INT_Timer2()
{
  if ( f_jjy ) { // JJY Start Condition ?
    timerRestart(timer1); // Timer1 interrupt restart
    d_sel = (d_sel ? 0 : 1); // Select JJY Buffer
    d_sec = 0;  // 100msec 割り込み回数カウンタークリア
    f_jjy = OFF; // JJY Start flag OFF
    timerStop(timer2);// Timer2 Stop
  }
}
/******************************************/
/* Timer Interrupt routine                */
/******************************************/
/* Timer1 Interrupt */
void IRAM_ATTR onTimer1() {
  portENTER_CRITICAL_ISR(&timerMux);
  INT_Timer1();
  portEXIT_CRITICAL_ISR(&timerMux);
}
/* Timer2 Interrupt */
void IRAM_ATTR onTimer2() {
  portENTER_CRITICAL_ISR(&timerMux);
  INT_Timer2();
  portEXIT_CRITICAL_ISR(&timerMux);
}
/******************************************/
/* Initial routine                        */
/******************************************/
/******************************************/
/* Dual CPU assigned TASKs                */
/* task0:core 0  loop:core 1              */
/******************************************/
void init_tasks()
{
  // コア0で関数task0を優先順位1で起動
  xTaskCreatePinnedToCore(task0, "Task0", TASKSTACK, NULL, 1, NULL, 0);
  /***
    // コア1で関数task1を優先順位1で起動
    xTaskCreatePinnedToCore(task1, "Task1", TASKSTACK, NULL, 1, NULL, 1);
  ***/
}
/*******************************************/
/* Interrupt Timer initial and start timer */
/*******************************************/
void init_inttimer()
{
  // Timer: interrupt time and event setting.
  timer1 = timerBegin(0, 80, true);
  timer2 = timerBegin(1, 80, true);
  // Attach onTimer function.
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAttachInterrupt(timer2, &onTimer2, true);
  // Set alarm to call onTimer function every second (value in microseconds).
  timerAlarmWrite(timer1, INTTIM1, true);
  timerAlarmWrite(timer2, INTTIM2, true);
  // Start an alarm
  timerAlarmEnable(timer1);
  timerAlarmEnable(timer2);
  // Stop Timer
  timerStop(timer1);
  timerStop(timer2);
}

/*****************************************/
/* Conslole Serial BPS Set               */
/*****************************************/
void init_console()
{
  Serial.begin(115200);
  delay(100);
  Serial.print("Start JJY wave Generator sync NTP\n");
  Serial.print("2019/8/8 Ver.1.0\n");
}

/******************************************/
/* Hard ware initial                      */
/******************************************/
void init_hard()
{
  // Set pin mode
  pinMode(JJYDO, OUTPUT);
  pinMode(ONBLED, OUTPUT);
  digitalWrite(JJYDO, LOW); // JJY Signal OFF
  digitalWrite(ONBLED, LOW);
}

/******************************************/
/* JJY Puls Generation processing         */
/* JJYバッファ初期化                         */
/******************************************/
void clr_jjy( unsigned char d[] )
{
  int i;

  // JJYバッファをゼロクリア（送信データのゼロパルスカウントセット）
  for (i = 0; i < 60; i++ ) {
    d[i] = PULSOFF;
  }

  // JJYバッファにマーカーカウントをセット
  d[0] = PULSMK;
  for (i = 9; i < 60; i += 10 ) {
    d[i] = PULSMK;
  }
}
/******************************************/
/* JJY処理変数の初期化                       */
/******************************************/
void init_jjy()
{
  clr_jjy( &d_jjy[0][0] );// JJY Buffer 0 Initialize
  clr_jjy( &d_jjy[1][0] );// JJY Buffer 1 Initialize
  f_jjy = OFF;  // JJY Signal Start Flag
  d_sel = 0;    // JJY Buffer Selecter
  d_sec = 0;    // JJY Buffer Conter
}
/*******************************************/
/* Start Up Procces                        */
/*******************************************/
void setup()
{
  init_hard();    // Hard ware initial
  init_console(); // Console initial for Debug
  init_lcd();     // LCD Keypad initial
  init_SPIFFS();  // File Sytem Start
  if ( cfg_load() || jjyhz == 0 ) {// Condif File Load
    Config_setup( 0 );// Boot First
    Config_loop();    // Edit Config
  }
  prn_status( 0 );
  if ( chk_SEL() ) {
    Config_setup( 1 );
    Config_loop();
  }
  end_SPIFFS();
  Serial.printf("JJY  %dkHz SSID:%s  PW:%s\n", jjyhz, ssid, password);
  Serial.printf("NTP1 %s  NTP2 %s\n", ntp1, ntp2 ); 
  init_dds();     // Digital signal generator(after read config)
  delay(50);
  RTC_setup();
  delay(100);
  prn_status( 1 );
  f_adj = ON;
  adj_clk();       // Clock Sync NTP Server
  init_jjy();      // JJY Puls Generation processing
  init_tasks();    // Dual Core assigned TASK
  init_inttimer(); // Interrupt timer set and start
  prn_status( 2 );
}

/******************************************/
/* MAIL LOOP                              */
/******************************************/
void loop()
{
  delay(50);
  while ( f_jjy == OFF ) vTaskDelay(1); //JJY Signal Start Wait
  while ( 1 ) {
    if ( chk_clk() ) { // Sync NTP
      prn_status( 1 );
      adj_clk();
    }
    prn_sysclock();// LCDに日時表示
    delay(100);
  }
}
/*******/
/* EOF */
/*******/
