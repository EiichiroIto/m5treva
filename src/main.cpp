#include <M5Stack.h>

const int clockpin = 2;
const int datapin = 5;
const int waitcount = 0;
const int verbose = 0;

const uint8_t quality = 75;
const uint16_t width = 96;
const uint16_t height = 72;
const int bitmap_size = width * height;

uint8_t *rawdata = NULL;
uint16_t *bitmap = NULL;

void shot();
int convertRGB565(uint16_t *dst);

void setup()
{
  pinMode(clockpin, OUTPUT);
  pinMode(datapin, INPUT);
  delay(500);
  M5.begin();
  Serial.updateBaudRate(115200);
  Serial.println("treva setup start");

  rawdata = (uint8_t *) malloc(width * height * 2);
  if (rawdata == NULL) {
    Serial.println("cannot malloc rawdata");
    return;
  }

  Serial.printf("bitmap size = %d bytes\r\n", bitmap_size);
  bitmap = (uint16_t *) malloc(bitmap_size * sizeof (uint16_t));
  if (bitmap == NULL) {
    Serial.println("cannot malloc bitmap");
    return;    
  }
  Serial.println("treva loop start");
}

void loop()
{
  if (verbose) {
    Serial.println("shot");
  }
  shot();
  if (verbose) {
    Serial.println("convert");
  }
  int ret = convertRGB565(bitmap);
  if (ret != bitmap_size) {
    Serial.printf("invalid converted size %d (expected %d)\n", ret, bitmap_size);
    return;
  }
  if (verbose) {
    Serial.println("draw");
  }
  M5.Lcd.drawBitmap(0, 0, width, height, bitmap);
  delay(1);
}

int read_bit()
{
  int out;

  digitalWrite(clockpin, 0);
  usleep(waitcount);
  out = digitalRead(datapin);
  digitalWrite(clockpin, 1);
  usleep(waitcount);
  return out;
}

void wait_successive1(int num)
{
  int count = 0;
  while (count < num) {
    if (read_bit() > 0) {
      count += 1;
    } else {
      count = 0;
    }
  }
}

void wait_successive0(int num)
{
  int count = 0;
  while (count < num) {
    if (read_bit() < 1) {
      count += 1;
    } else {
      count = 0;
    }
  }
}

void shot()
{
  int i, k, d;

  if ( verbose ) {
    Serial.println(" syncing...");
  }
  wait_successive1(100);
  wait_successive0(65);
  for (i = 0; i < 16; i ++) {
    read_bit();
  }
  if ( verbose ) {
    Serial.println(" getting...");
  }
  for (k = 0; k < 96*72*2; k ++) {
    d = 0;
    for (i = 0; i < 8; i ++) {
      d <<= 1;
      if (read_bit() > 0) {
        d |= 1;
      }
    }
    rawdata[k] = d;
  }
  if ( verbose ) {
    Serial.println(" done.");
  }
}

#define ForceRange(v,min,max) ((v)<(min)?(min):((v)>(max)?(max):(v)))
#define RGB565(r,g,b) (((r>>3)<<11)|((g>>2)<<5)|(b>>3))

int convertRGB565(uint16_t *dst)
{
  int i, j, count = 0;

  for ( i = 0; i < height; i ++ ) {
    for ( j = 0; j < width; j ++ ) {
      int p = (i*width+j)*2;
      float Y = rawdata[p+1], V, U;
      int red, green, blue;
      if ((j % 2) == 0) {
        V = rawdata[p];
        U = rawdata[p+2];
      } else {
        V = rawdata[p-2];
        U = rawdata[p];
      }
      U = U - 128.0;
      V = V - 128.0;
#if 1
      red = U + Y;
      green = 0.98*Y - 0.53*U - 0.19*V;
      blue = V + Y;
#else
      red = 1.40 * U + Y;
      green = 1.02 * Y - 0.75 * U - 0.336 * V;
      blue = 1.77 * V + Y;
#endif
      red = ForceRange(red, 0, 255);
      green = ForceRange(green, 0, 255);
      blue = ForceRange(blue, 0, 255);
      unsigned short col16 = RGB565(red, green, blue);
      *dst++ = col16;
      count ++;
    }
  }
  return count;
}
