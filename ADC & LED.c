#include <LPC17xx.h>
#include <stdint.h>

#define LED_MASK   (0xFF << 4)    // LEDs on P0.4 – P0.11 (CNA)

// ---------- Function Prototypes ----------
void delay_ms(unsigned int);
unsigned int read_adc(void);
void ring_counter(void);
void johnson_counter(void);

// ---------- Global Variables ----------
unsigned int adc_val;
float voltage;

// =====================================================
// MAIN FUNCTION
// =====================================================
int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    // ---------- LED setup (CNA connector) ----------
    LPC_GPIO0->FIODIR |= LED_MASK;      // P0.4–P0.11 as output

    // ---------- ADC setup on P1.30 (AD0.4) ----------
    LPC_PINCON->PINSEL3 |= (3 << 28);   // P1.30 → AD0.4 (function 11)
    LPC_SC->PCONP |= (1 << 12);         // Power up ADC
    LPC_ADC->ADCR = (1 << 4) |          // Select AD0.4 channel
                    (4 << 8)  |         // ADC clock = PCLK/5
                    (1 << 21);          // Enable ADC

    while (1)
    {
        adc_val = read_adc();                 // Get 12-bit ADC value
        voltage = (adc_val * 3.3f) / 4095.0f; // Convert to voltage

        if (voltage > 2.0f)
            ring_counter();                   // >2 V → Ring counter
        else
            johnson_counter();                // ≤2 V → Johnson counter
    }
}

// =====================================================
// FUNCTION: READ ADC VALUE (Channel AD0.4)
// =====================================================
unsigned int read_adc(void)
{
    unsigned int result;
    LPC_ADC->ADCR |= (1 << 24);                // Start conversion
    while ((LPC_ADC->ADGDR & (1 << 31)) == 0); // Wait for DONE
    result = (LPC_ADC->ADGDR >> 4) & 0xFFF;    // Extract 12-bit result
    LPC_ADC->ADCR &= ~(7 << 24);               // Stop conversion
    return result;
}

// =====================================================
// FUNCTION: RING COUNTER on P0.4–P0.11
// =====================================================
void ring_counter(void)
{
    uint32_t val = 0x01;
    for (int i = 0; i < 8; i++)
    {
        LPC_GPIO0->FIOCLR = LED_MASK;        // Clear all LEDs
        LPC_GPIO0->FIOSET = (val << 4);      // Set one LED
        delay_ms(200);
        val <<= 1;
        if (val == 0x100) val = 0x01;        // Restart after 8 bits
    }
}

// =====================================================
// FUNCTION: JOHNSON COUNTER on P0.4–P0.11
// =====================================================
void johnson_counter(void)
{
    uint8_t val = 0x00;

    // Fill phase
    for (int i = 0; i < 8; i++)
    {
        val = (val >> 1) | 0x80;             // Shift right, insert 1
        LPC_GPIO0->FIOCLR = LED_MASK;
        LPC_GPIO0->FIOSET = (val << 4);
        delay_ms(200);
    }

    // Empty phase
    for (int i = 0; i < 8; i++)
    {
        val >>= 1;                            // Shift right, insert 0
        LPC_GPIO0->FIOCLR = LED_MASK;
        LPC_GPIO0->FIOSET = (val << 4);
        delay_ms(200);
    }
}

// =====================================================
// FUNCTION: TIMER0 DELAY (Accurate millisecond delay)
// =====================================================
void delay_ms(unsigned int milliseconds)
{
    LPC_TIM0->TCR = 0x02;     // Reset Timer
    LPC_TIM0->PR  = 2999;     // Prescale for 1 ms (100 MHz CCLK → 25 MHz PCLK)
    LPC_TIM0->TCR = 0x01;     // Enable Timer
    while (LPC_TIM0->TC < milliseconds);
    LPC_TIM0->TCR = 0x00;     // Disable Timer
    LPC_TIM0->TC  = 0;        // Reset counter
}
