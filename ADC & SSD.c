#include <LPC17xx.h>
#include <math.h>
#include <stdint.h>

#define SEGMENT_MASK (0xFF << 4)      // P0.4–P0.11 → segments (CNA)
#define DIGIT_MASK   (0x0F << 23)     // P1.23–P1.26 → digit select (CNB)

// ----------- Function Prototypes ------------------
void delay_ms(unsigned int);
unsigned int read_adc(uint8_t channel);
void display_number(unsigned int num);
void display_digit(uint8_t digit, uint8_t pos);

// ----------- Global Variables ---------------------
float v4, v5, diff;
unsigned int adc4, adc5;
unsigned int disp_val;

// 7-segment lookup (common cathode → segments active HIGH)
uint8_t seg_code[10] = {
    0x3F, //0
    0x06, //1
    0x5B, //2
    0x4F, //3
    0x66, //4
    0x6D, //5
    0x7D, //6
    0x07, //7
    0x7F, //8
    0x6F  //9
};

// =====================================================
// MAIN FUNCTION
// =====================================================
int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    // -------- 7-Segment Setup (CNA + CNB) --------
    LPC_GPIO0->FIODIR |= SEGMENT_MASK; // P0.4–P0.11 → output
    LPC_GPIO1->FIODIR |= DIGIT_MASK;   // P1.23–P1.26 → output

    // -------- ADC Setup (AD0.4 + AD0.5) ----------
    LPC_PINCON->PINSEL3 |= (3 << 28) | (3 << 30); // P1.30→AD0.4, P1.31→AD0.5
    LPC_SC->PCONP |= (1 << 12);                   // Power up ADC
    LPC_ADC->ADCR = (1 << 21) | (4 << 8);         // Enable ADC, clk = PCLK/5

    while (1)
    {
        adc4 = read_adc(4);
        adc5 = read_adc(5);

        v4 = (adc4 * 3.3f) / 4095.0f;
        v5 = (adc5 * 3.3f) / 4095.0f;
        diff = fabsf(v4 - v5);                    // |V4 - V5|

        disp_val = (unsigned int)(diff * 100);    // Convert to hundredths

        // Multiplex display for ~0.5s
        for (int i = 0; i < 100; i++)
        {
            display_number(disp_val);
        }
    }
}

// =====================================================
// FUNCTION: READ ADC VALUE (Channel AD0.x)
// =====================================================
unsigned int read_adc(uint8_t channel)
{
    unsigned int result;
    LPC_ADC->ADCR &= ~(0x7);           // Clear previous channel
    LPC_ADC->ADCR |= (1 << channel);   // Select channel
    LPC_ADC->ADCR |= (1 << 24);        // Start conversion
    while ((LPC_ADC->ADGDR & (1 << 31)) == 0); // Wait for DONE
    result = (LPC_ADC->ADGDR >> 4) & 0xFFF;
    LPC_ADC->ADCR &= ~(7 << 24);       // Stop conversion
    return result;
}

// =====================================================
// FUNCTION: DISPLAY A 4-DIGIT NUMBER
// =====================================================
void display_number(unsigned int num)
{
    uint8_t d[4];
    d[0] = num % 10;
    d[1] = (num / 10) % 10;
    d[2] = (num / 100) % 10;
    d[3] = (num / 1000) % 10;

    for (int i = 0; i < 4; i++)
    {
        display_digit(d[i], i);
        delay_ms(3);
    }
}

// =====================================================
// FUNCTION: DISPLAY SINGLE DIGIT (COMMON-CATHODE)
// =====================================================
void display_digit(uint8_t digit, uint8_t pos)
{
    LPC_GPIO0->FIOCLR = SEGMENT_MASK; // clear old segments
    LPC_GPIO1->FIOCLR = DIGIT_MASK;   // turn off all digits

    LPC_GPIO0->FIOSET = (seg_code[digit] << 4); // send segment pattern
    LPC_GPIO1->FIOSET = (1 << (23 + pos));      // enable one digit (active high)
}

// =====================================================
// FUNCTION: DELAY USING TIMER0
// =====================================================
void delay_ms(unsigned int milliseconds)
{
    LPC_TIM0->TCR = 0x02;    // reset timer
    LPC_TIM0->PR  = 2999;    // 1 ms prescale (assuming 100 MHz CCLK)
    LPC_TIM0->TCR = 0x01;    // enable timer
    while (LPC_TIM0->TC < milliseconds);
    LPC_TIM0->TCR = 0x00;    // stop timer
    LPC_TIM0->TC  = 0;       // reset counter
}
