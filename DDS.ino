#include <SPI.h>
#include "ESP32_DEF.h"

// ANALOG DEVICES AD9833
/* SPI Assign */
//VSPI - SCK,MISO,MOSI,SS = 18,19,23,5
#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23
#define SPI_SS   5
#define WSINE 0x2000
#define FRQ40KHZ  0x68DB9
#define FRQ60KHZ  0x9D495
//#define FRQ60KHZ  0x9AAA4  // 1KHzほど周波数が高いような感じなので-1KHzした値
#define FRQLSB( a )   (( a & 0x3FFF) | 0x4000) 
#define FRQMSB( a )  ((( a & 0xFFFC000) >> 14) | 0x4000)

SPIClass spi(VSPI); 
/*****************************************/
/* SPI(DDS)                              */
/*****************************************/
void init_dds()
{
  spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_SS);
  spi.setFrequency(1000000);
  spi.setBitOrder(MSBFIRST);
  spi.setDataMode(SPI_MODE2);
  pinMode(SPI_SS, OUTPUT);    // Device Slelct Signal
  digitalWrite(SPI_SS, HIGH); // Device Select off(HIGH=OFF)
  spi_wrt(0b0000000100000000); //Reset
  delay(50);
  set_AD9833( jjyhz );
  delay(100);
  spi.end();
}
/***************************************/
/* SPI Data send                       */
/***************************************/
void spi_wrt(uint16_t b){
  digitalWrite(SPI_SS, LOW); // Device select
  delayMicroseconds( 1 ); 
  spi.transfer16(b);
  digitalWrite(SPI_SS, HIGH);
  delayMicroseconds( 1 ); 
}
/***************************************/
/* AD9833に周波数を設定する               */
/* hz=60 60KHz  それ以外 40KHz          */ 
/***************************************/
const struct {
  uint16_t lsb;
  uint16_t msb;
} t_ad9833[2] = {
  { FRQLSB( FRQ40KHZ ), FRQMSB( FRQ40KHZ ) },
  { FRQLSB( FRQ60KHZ ), FRQMSB( FRQ60KHZ ) }
};
void set_AD9833( int hz )
{
  spi_wrt(0b0010000000000000); //Control word write
  spi_wrt(t_ad9833[hz == 60 ? 1 : 0].lsb);
  spi_wrt(t_ad9833[hz == 60 ? 1 : 0].msb);
 
  spi_wrt(0b1100000000000000); //位相シフトはゼロ
  spi_wrt( WSINE );//Sine Wave
}
/*******/
/* EOF */
/*******/
