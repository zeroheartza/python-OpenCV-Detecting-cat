/*Suttiwat Read MP3 To Byte On VS1003 With C/C++*/
#include<iostream>
#include<fstream>
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdint.h>//beer for new data type (uint8_t,uint16_t and more)
using namespace std;
//#include "mp3.h"// mp3 header
//#include "test.h"// mp3 header
//#include "test2.h"// mp3 header

const uint8_t vs1003_chunk_size = 32;
#define VS_WRITE_COMMAND 0x02
#define VS_READ_COMMAND 0x03
#define min(a,b) ((a)<(b)?(a):(b))

// SCI Registers
const uint8_t SCI_MODE = 0x0;
const uint8_t SCI_STATUS = 0x1;
const uint8_t SCI_BASS = 0x2;
const uint8_t SCI_CLOCKF = 0x3;
const uint8_t SCI_DECODE_TIME = 0x4;
const uint8_t SCI_AUDATA = 0x5;
const uint8_t SCI_WRAM = 0x6;
const uint8_t SCI_WRAMADDR = 0x7;
const uint8_t SCI_HDAT0 = 0x8;
const uint8_t SCI_HDAT1 = 0x9;
const uint8_t SCI_AIADDR = 0xa;
const uint8_t SCI_VOL = 0xb;
const uint8_t SCI_AICTRL0 = 0xc;
const uint8_t SCI_AICTRL1 = 0xd;
const uint8_t SCI_AICTRL2 = 0xe;
const uint8_t SCI_AICTRL3 = 0xf;
const uint8_t SCI_num_registers = 0xf;

// SCI_MODE bits
const uint8_t SM_DIFF = 0;
const uint8_t SM_LAYER12 = 1;
const uint8_t SM_RESET = 2;
const uint8_t SM_OUTOFWAV = 3;
const uint8_t SM_EARSPEAKER_LO = 4;
const uint8_t SM_TESTS = 5;
const uint8_t SM_STREAM = 6;
const uint8_t SM_EARSPEAKER_HI = 7;
const uint8_t SM_DACT = 8;
const uint8_t SM_SDIORD = 9;
const uint8_t SM_SDISHARE = 10;
const uint8_t SM_SDINEW = 11;
const uint8_t SM_ADPCM = 12;
const uint8_t SM_ADCPM_HP = 13;
const uint8_t SM_LINE_IN = 14;

//Beer Config From AJ.
uint8_t cs_pin =0;
uint8_t dcs_pin=1;
uint8_t dreq_pin=2;
uint8_t reset_pin=3;

void await_data_request(void)
  {
    while (!digitalRead(dreq_pin))
      delay(1);
  }
void control_mode_on(void)
  {
    digitalWrite(dcs_pin, HIGH);
    digitalWrite(cs_pin, LOW);
  }
void control_mode_off(void)
  {
    digitalWrite(cs_pin, HIGH);
  }
void data_mode_on(void)
  {
    digitalWrite(cs_pin, HIGH);
    digitalWrite(dcs_pin, LOW);
  }
void data_mode_off(void)
  {
    digitalWrite(dcs_pin, HIGH);
  }
  
//Fixed DATA transfer by Beer
unsigned char temp_buffer[4];//1000
uint16_t read_register(uint8_t _reg)
{
  uint16_t result;
  control_mode_on();
  delayMicroseconds(1);             // tXCSS
  
  temp_buffer[0]=VS_READ_COMMAND;
  temp_buffer[1]=_reg;
  temp_buffer[2]=0x00;
  temp_buffer[3]=0x00;
  result = wiringPiSPIDataRW(0, temp_buffer, 4);
  delayMicroseconds(1);             // tXCSH
  await_data_request();
  control_mode_off();
  result= temp_buffer[3]|temp_buffer[2]<<8;
  return result;
}

void write_register(uint8_t _reg, uint16_t _value)
{
  uint16_t result;
  control_mode_on();
  delayMicroseconds(1);           // tXCSS
  temp_buffer[0]=VS_WRITE_COMMAND;
  temp_buffer[1]=_reg;
  temp_buffer[2]=_value >> 8;
  temp_buffer[3]=_value & 0xff;
  result = wiringPiSPIDataRW(0, temp_buffer, 4);
  delayMicroseconds(1);           // tXCSH
  await_data_request();
  control_mode_off();
}

void sdi_send_buffer(const uint8_t *data, size_t len)
{
  uint16_t i = 0;
  data_mode_on();
  uint8_t c;
  while (len)
  {
    await_data_request();
    delayMicroseconds(3);

    size_t chunk_length = min(len, vs1003_chunk_size);
    len -= chunk_length;
    while (chunk_length--)
    {
      c = *(data + i);
	  (void)wiringPiSPIDataRW(1, &c , 1);//data transfer by beer
      i++;
    }
  }
  data_mode_off();
}

void sdi_send_zeroes(size_t len)
{
  data_mode_on();
  while (len)
  {
    await_data_request();
    size_t chunk_length = min(len, vs1003_chunk_size);
    len -= chunk_length;
    while (chunk_length--)
		(void)wiringPiSPIDataRW(1, 0 , 1);//data transfer by beer
  }
  data_mode_off();
}


void begin(void)//Setup VS1003 fixed data by beer
{

  // Keep the chip in reset until we are ready
  pinMode(reset_pin, OUTPUT);
  digitalWrite(reset_pin, LOW);
  
  // The SCI and SDI will start deselected
  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);
  pinMode(dcs_pin, OUTPUT);
  digitalWrite(dcs_pin, HIGH);

  // DREQ is an input
  pinMode(dreq_pin, INPUT);
  delay(2);

  

  // init SPI slow mode
  //SPI.setClockDivider(SPI_CLOCK_DIV64); // Slow!
  (void)wiringPiSPISetup(0, 2000000);//Fixed by beer

  // release from reset
  digitalWrite(reset_pin, HIGH);

  // Declick: Immediately switch analog off
  write_register(SCI_VOL, 0xffff); // VOL

  /* Declick: Slow sample rate for slow analog part startup */
  write_register(SCI_AUDATA, 10);

  delay(100);
  
  /* Switch on the analog parts */
  write_register(SCI_VOL, 0xfefe); // VOL

  write_register(SCI_AUDATA, 44101); // 44.1kHz stereo

  write_register(SCI_VOL, 0x2020); // VOL

  // soft reset
  write_register(SCI_MODE, 1<<(SM_SDINEW) | 1<<(SM_RESET));
  delay(1);
  await_data_request();
  
  write_register(SCI_CLOCKF, 0xB800); // Experimenting with higher clock settings
  delay(1);
  await_data_request();

  // Now you can set high speed SPI clock
  //SPI.setClockDivider(SPI_CLOCK_DIV4); // Fastest available
  (void)wiringPiSPISetup(1, 2000000);
}

void setVolume(uint8_t vol)
{
  uint16_t value = vol;
  value <<= 8;
  value |= vol;

  write_register(SCI_VOL, value);
}

void startSong(void)
{
  sdi_send_zeroes(10);
}

void playChunk(const uint8_t *data, size_t len)
{
  sdi_send_buffer(data, len);
}

//read MP3 to byte by beer
void play(void){
	ifstream infile;
	infile.open("ringtone.mp3",ios::binary|ios::in);
	
	int buffer[2];
	startSong();//send 0bit 10time
	printf("Read MP3 file...\n");
	while(infile.read((char *)&buffer,sizeof(buffer)))
	{
		//cast data type to play mp3
		playChunk((const uint8_t*)buffer, sizeof(buffer));
	}
	
	printf("End Read MP3 file\n");
	infile.close();
	//break;
}
int main(void){
	
	printf("Raspberry SPI init\n");
	//inti and check SPI
	if(wiringPiSetup() == -1)
		return 1;
	
	begin();
	pinMode(cs_pin,OUTPUT);
	setVolume(0x25);//set max valume
	
	play();
	
	
	return 0;
}
