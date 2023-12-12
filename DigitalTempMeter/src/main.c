#include <avr/io.h>
#include <avr/interrupt.h>

void enable_external_interrupts()
{
  sei(); // Enable Global Interrupt

  DDRD &= ~((1 << PIND2) | (1 << PIND3)); // Set Port-D Interrupt Pins 2 & 3 As Input
  PORTD |= (1 << PIND2) | (1 << PIND3);   // Enable Pull-Up Resistors

  EIMSK |= (1 << INT0) | (1 << INT1);   // Enable Both External Interrupts 1 & 2
  EICRA |= (1 << ISC01) | (1 << ISC11); // Trigger Interrupt On Falling-Edge
}

void enable_7_segments()
{
  DDRB = 0xFF;                                        // Set All Port-B As Output
  DDRD |= (1 << PIND5) | (1 << PIND6) | (1 << PIND7); // Set Port-D Pins 5-7 As Output
}

void enable_timer(uint8_t counts)
{
  // In 1 MHz Crystal, Each Clock Pulse = 1/1MHz = 1 usec.
  // If We Use Prescaler Of 64, Each Clock Pulse = 1*64 = 64 usec.
  // So To Get 8 msec, We Need Counts = 8000/64 = 125
  OCR0A = counts;
  TCCR0A |= (1 << WGM01);              // Mode = CTC
  TIMSK0 |= (1 << OCIE0A);             // Enable On Compare A Match Interrupt
  TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler = 64
}

void enable_adc(uint8_t channel)
{
  ADMUX |= (1 << REFS0);     // Set ADC Ref = Vref
  ADMUX |= (channel & 0x1F); // Select ADC Channel

  ADCSRA |= (1 << ADIE);                              // Enable ADC Interrupt
  ADCSRA |= (1 << ADPS1) | (1 << ADPS0);              // Prescaler = 8 --> 1 MHz / 8 = 125 KHz
  ADCSRA |= (1 << ADEN) | (1 << ADSC);                // Start The First Conversion
}

uint16_t tempInC(uint16_t digitalValue)
{
  return digitalValue / 2.046;
}

uint16_t tempInF(uint16_t digitalValue)
{
  return (digitalValue / 2.046 * 1.8) + 32;
}

uint8_t i = 0;
uint16_t digitalTemp = 0;
uint8_t isItCelsius = 1;
uint8_t decimal[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
int main()
{
  enable_external_interrupts();
  enable_7_segments();
  enable_timer(124);
  enable_adc(0);

  while (1)
  {
    // Convert The Digital Temp From ADC, To Celsius/Fahrenheit Temp
    uint16_t temp = isItCelsius ? tempInC(digitalTemp) : tempInF(digitalTemp);

    // Break Up The temp Number To 3 Digits
    uint8_t digits[3];
    for (uint8_t x = 0; x < 3; x++)
    {
      digits[x] = temp % 10;
      temp /= 10;
    }

    PORTB = decimal[digits[i]]; // Put i-th Digit On The Segments Bus
    PORTD |= (7 << 5);          // Turn All Segments Off
    PORTD &= ~(1 << (5 + i));   // Turn Only The i-th Segment On
  }
}

ISR(INT0_vect)
{
  isItCelsius = 1;
}

ISR(INT1_vect)
{
  isItCelsius = 0;
}

ISR(ADC_vect)
{
  digitalTemp = ADC;      // Read The ADC Value
  ADCSRA |= (1 << ADSC);  // Start The Next Conversion
}

ISR(TIMER0_COMPA_vect)
{
  i = (i + 1) % 3;
}