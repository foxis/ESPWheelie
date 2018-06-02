#include <Arduino.h>
#include <SPI.h>


#define PIN_MOUSECAM_RESET     5  // seems to be not as important... instead used this pin for laser / led
#define PIN_MOUSECAM_CS        4

#define ADNS3080_PIXELS_X                 30
#define ADNS3080_PIXELS_Y                 30

#define ADNS3080_PRODUCT_ID            0x00
#define ADNS3080_REVISION_ID           0x01
#define ADNS3080_MOTION                0x02
#define ADNS3080_DELTA_X               0x03
#define ADNS3080_DELTA_Y               0x04
#define ADNS3080_SQUAL                 0x05
#define ADNS3080_PIXEL_SUM             0x06
#define ADNS3080_MAXIMUM_PIXEL         0x07
#define ADNS3080_CONFIGURATION_BITS    0x0a
#define ADNS3080_EXTENDED_CONFIG       0x0b
#define ADNS3080_DATA_OUT_LOWER        0x0c
#define ADNS3080_DATA_OUT_UPPER        0x0d
#define ADNS3080_SHUTTER_LOWER         0x0e
#define ADNS3080_SHUTTER_UPPER         0x0f
#define ADNS3080_FRAME_PERIOD_LOWER    0x10
#define ADNS3080_FRAME_PERIOD_UPPER    0x11
#define ADNS3080_MOTION_CLEAR          0x12
#define ADNS3080_FRAME_CAPTURE         0x13
#define ADNS3080_SROM_ENABLE           0x14
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_LOWER      0x19
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_UPPER      0x1a
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_LOWER      0x1b
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_UPPER      0x1c
#define ADNS3080_SHUTTER_MAX_BOUND_LOWER           0x1e
#define ADNS3080_SHUTTER_MAX_BOUND_UPPER           0x1e
#define ADNS3080_SROM_ID               0x1f
#define ADNS3080_OBSERVATION           0x3d
#define ADNS3080_INVERSE_PRODUCT_ID    0x3f
#define ADNS3080_PIXEL_BURST           0x40
#define ADNS3080_MOTION_BURST          0x50
#define ADNS3080_SROM_LOAD             0x60

#define ADNS3080_PRODUCT_ID_VAL        0x17

class ADNS3080
{
	int _ss_pin;
	int _reset_pin;
	bool _init;

public:
	typedef struct MD
	{
	  byte motion;
	  char dx, dy;
	  byte squal;
	  word shutter;
	  byte max_pix;
	} MD_t;

public:
	ADNS3080(int ss_pin, int reset) {
		_ss_pin = ss_pin;
		_reset_pin = reset;
		_init = false;
	}

	void reset() {
		digitalWrite(_reset_pin, HIGH);
		delay(1); // reset pulse >10us
		digitalWrite(_reset_pin, LOW);
		delay(35); // 35ms from reset to functional
		write_reg(ADNS3080_CONFIGURATION_BITS, 0x19);
	}

	void begin() {
		SPI.begin();
		SPI.setClockDivider(SPI_CLOCK_DIV32);
	  SPI.setDataMode(SPI_MODE3);
	  SPI.setBitOrder(MSBFIRST);
		pinMode(_ss_pin, OUTPUT);
		pinMode(_reset_pin, OUTPUT);
		digitalWrite(_ss_pin, HIGH);
		digitalWrite(_reset_pin, HIGH);

		reset();

		int pid = read_reg(ADNS3080_PRODUCT_ID);
		if (pid != ADNS3080_PRODUCT_ID_VAL) {
			_init = false;
			return ;
		}

		// turn on sensitive mode
		write_reg(ADNS3080_CONFIGURATION_BITS, 0x19);
		_init = true;
	}

	void read_motion(MD_t * p)
	{
		if (!_init) return;

	  digitalWrite(_ss_pin, LOW);
	  SPI.transfer(ADNS3080_MOTION_BURST);
	  delayMicroseconds(75);
	  p->motion =  SPI.transfer(0xff);
	  p->dx =  SPI.transfer(0xff);
	  p->dy =  SPI.transfer(0xff);
	  p->squal =  SPI.transfer(0xff);
	  p->shutter =  SPI.transfer(0xff) << 8;
	  p->shutter |=  SPI.transfer(0xff);
	  p->max_pix =  SPI.transfer(0xff);
	  digitalWrite(_ss_pin, HIGH);
	  delayMicroseconds(5);
	}

	// pdata must point to an array of size ADNS3080_PIXELS_X x ADNS3080_PIXELS_Y
	// you must call mousecam_reset() after this if you want to go back to normal operation
	size_t frame_capture(byte *pdata)
	{
		//if (!_init) return 0;

	  write_reg(ADNS3080_FRAME_CAPTURE, 0x83);

	  digitalWrite(_ss_pin, LOW);

	  SPI.transfer(ADNS3080_PIXEL_BURST);
	  delayMicroseconds(50);

	  byte pix;
	  byte started = 0;
	  size_t count;
	  int timeout = 0;
	  size_t ret = 0;
	  for (count = 0; count < ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y; )
	  {
	    pix = SPI.transfer(0xff);
	    delayMicroseconds(10);
	    if (started == 0)
	    {
	      if (pix & 0x40)
	        started = 1;
	      else
	      {
	        timeout++;
	        if (timeout == 100)
	        {
	          ret = 0;
	          break;
	        }
	      }
	    }
	    if (started == 1)
	    {
	      pdata[count++] = (pix & 0x3f); // scale to normal grayscale byte range
	    }
	  }

	  digitalWrite(_ss_pin, HIGH);
	  delayMicroseconds(14);

	  return ret;
	}

protected:
	void write_reg(byte reg, byte val)
	{
	  digitalWrite(_ss_pin, LOW);
	  SPI.transfer(reg | 0x80);
	  SPI.transfer(val);
	  digitalWrite(_ss_pin, HIGH);
	  delayMicroseconds(50);
	}

	byte read_reg(byte reg)
	{
	  digitalWrite(_ss_pin, LOW);
	  SPI.transfer(reg);
	  delayMicroseconds(75);
	  byte ret = SPI.transfer(0xff);
	  digitalWrite(_ss_pin, HIGH);
	  delayMicroseconds(1);
	  return ret;
	}
};
