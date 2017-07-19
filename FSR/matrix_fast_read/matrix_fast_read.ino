#define BAUD_RATE                 115200

#define ROW_COUNT                 6

#define COLUMN_COUNT              6



#define PIN_ADC_INPUT             A0

#define PIN_SHIFT_REGISTER_DATA   2

#define PIN_SHIFT_REGISTER_CLOCK  3

#define PIN_MUX_CHANNEL_0         4  //channel pins 0, 1, 2, etc must be wired to consecutive Arduino pins

#define PIN_MUX_CHANNEL_1         5

#define PIN_MUX_CHANNEL_2         6

#define PIN_MUX_INHIBIT_0         7  //inhibit = active low enable. All mux IC enables must be wired to consecutive Arduino pins

#define PIN_MUX_INHIBIT_1         8



#define SET_SR_DATA_HIGH()        PORTD|=B00000100

#define SET_SR_DATA_LOW()         PORTD&=~B00000100

#define SET_SR_CLK_HIGH()         PORTD|=B00001000

#define SET_SR_CLK_LOW()          PORTD&=~B00001000



#define ROWS_PER_MUX              8

#define MUX_COUNT                 1

#define CHANNEL_PINS_PER_MUX      3



#define PACKET_END_BYTE           0xFF

#define MAX_SEND_VALUE            254  //reserve 255 (0xFF) to mark end of packet

#define COMPRESSED_ZERO_LIMIT     254

#define MIN_SEND_VALUE            1    //values below this threshold will be treated and sent as zeros


#

#ifndef cbi

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))

#endif

#ifndef sbi

#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#endif



/**********************************************************************************************************

* GLOBALS

**********************************************************************************************************/

int current_enabled_mux = MUX_COUNT - 1;  //init to number of last mux so enabled mux increments to first mux on first scan.

int compressed_zero_count = 0;

int offSetMat[ROW_COUNT][COLUMN_COUNT];



/**********************************************************************************************************

* setup()

**********************************************************************************************************/

void setup()

{

  Serial.begin(BAUD_RATE);

  pinMode(PIN_ADC_INPUT, INPUT);

  pinMode(PIN_SHIFT_REGISTER_DATA, OUTPUT);

  pinMode(PIN_SHIFT_REGISTER_CLOCK, OUTPUT);

  pinMode(PIN_MUX_CHANNEL_0, OUTPUT);

  pinMode(PIN_MUX_CHANNEL_1, OUTPUT);

  pinMode(PIN_MUX_CHANNEL_2, OUTPUT);

  pinMode(PIN_MUX_INHIBIT_0, OUTPUT);

  pinMode(PIN_MUX_INHIBIT_1, OUTPUT);

  

  sbi(ADCSRA,ADPS2);  //set ADC prescaler to CLK/16

  cbi(ADCSRA,ADPS1);

  cbi(ADCSRA,ADPS0);

  
   compressed_zero_count = 0;
   for(int i = 0; i < ROW_COUNT; i ++)

  {

    setRow(i);

    shiftColumn(true);

    for(int j = 0; j < COLUMN_COUNT; j ++)

    {
       offSetMat[i][j] = analogRead(PIN_ADC_INPUT);
    }
  }


}





/**********************************************************************************************************

* loop()

**********************************************************************************************************/

void loop()

{

  compressed_zero_count = 0;

  for(int i = 0; i < ROW_COUNT; i ++)

  {

    setRow(i);

    shiftColumn(true);

    for(int j = 0; j < COLUMN_COUNT; j ++)

    {
      int check;

      check = analogRead(PIN_ADC_INPUT) - offSetMat[i][j];
      int raw_reading = check >= 0 ? check : 0;

      byte send_reading = (byte) (lowByte(raw_reading >> 2));

      shiftColumn(false);

      sendCompressed(send_reading);

    }

  }

  if(compressed_zero_count > 0)

  {

    Serial.write((byte) 0);

    Serial.write((byte) compressed_zero_count);

  }

  Serial.write((byte) PACKET_END_BYTE);

}





/**********************************************************************************************************

* setRow() - Enable single mux IC and channel to read specified matrix row. digitalWrite() have not been

* replaced in this function, as mux changes are relatively infrequent.

**********************************************************************************************************/

void setRow(int row_number)

{

  if((row_number % ROWS_PER_MUX) == 0)  //We've reached channel 0 of a mux IC, so disable the previous mux IC, and enable the next mux IC

  {

    digitalWrite(PIN_MUX_INHIBIT_0 + current_enabled_mux, HIGH);  //This offset is why mux inhibits must be wired to consecutive Arduino pins

    current_enabled_mux ++;

    if(current_enabled_mux >= MUX_COUNT)

    {

      current_enabled_mux = 0;

    }

    digitalWrite(PIN_MUX_INHIBIT_0 + current_enabled_mux, LOW);  //enable the next mux, active low

  }

  for(int i = 0; i < CHANNEL_PINS_PER_MUX; i ++)

  {

    if(bitRead(row_number, i))

    {

      digitalWrite(PIN_MUX_CHANNEL_0 + i, HIGH);

    }

    else

    {

      digitalWrite(PIN_MUX_CHANNEL_0 + i, LOW);

    }

  }

}





/**********************************************************************************************************

* shiftColumn() - Shift out a high bit to drive first column, or increment column by shifting out a low

* bit to roll high bit through cascaded shift register outputs. digitalWrite() has been replaced with direct

* port manipulation macros, as this function performs the vast majority of pin writes

**********************************************************************************************************/

void shiftColumn(boolean is_first)

{

  if(is_first)

  {

    SET_SR_DATA_HIGH();

  }

  SET_SR_CLK_HIGH();

  SET_SR_CLK_LOW();

  if(is_first)

  {

    SET_SR_DATA_LOW();

  }

}





/**********************************************************************************************************

* sendCompressed() - If value is nonzero, send it via serial terminal as a single byte. If value is zero,

* increment zero count. The current zero count is sent and cleared before the next nonzero value

**********************************************************************************************************/

void sendCompressed(byte value)

{

  if(value < MIN_SEND_VALUE)

  {

    if(compressed_zero_count < (COMPRESSED_ZERO_LIMIT - 1))

    {

      compressed_zero_count ++;

    }

    else

    {

      Serial.write((byte) 0);

      Serial.write((byte) COMPRESSED_ZERO_LIMIT);

      compressed_zero_count = 0; 

    }

  }

  else

  {

    if(compressed_zero_count > 0)

    {

      Serial.write((byte) 0);

      Serial.write((byte) compressed_zero_count);

      compressed_zero_count = 0;

    }

    if(value > MAX_SEND_VALUE)

    {

       Serial.write((byte) MAX_SEND_VALUE);

    }

    else

    {

       Serial.write((byte) value);

    }

  }

}

