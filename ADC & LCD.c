#include <LPC17xx.h>
#include <stdio.h>
#include <math.h>

// ---------------- LCD Pin Definitions (CND connector) ----------------
#define RS_CTRL  (1 << 27)   // P0.27
#define EN_CTRL  (1 << 28)   // P0.28
#define RW_CTRL  (1 << 0)    // P2.0
#define DT_CTRL  (0x0F << 23)// P0.23–P0.26

// ---------------- Function Prototypes ----------------
void delay_lcd(unsigned int);
void delay_ms(unsigned int);
void lcd_init(void);
void lcd_cmd(unsigned char);
void lcd_data(unsigned char);
void lcd_print(char *str);
void lcd_write_nibble(unsigned char);
unsigned int read_adc(uint8_t channel);

// ---------------- Global Variables ----------------
unsigned int adc_val4, adc_val5;
float v4, v5, diff;
char msg[20];

// =====================================================
// MAIN FUNCTION
// =====================================================
int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    // -------- LCD Pin Setup (CND) --------
    LPC_PINCON->PINSEL1 &= 0xFC003FFF;          // P0.23–P0.28 GPIO
    LPC_GPIO0->FIODIR |= DT_CTRL | RS_CTRL | EN_CTRL;  // Data + control as output
    LPC_GPIO2->FIODIR |= RW_CTRL | (1 << 1);    // RW + backlight output
    LPC_GPIO2->FIOCLR = RW_CTRL;                // RW = 0 (Write mode)

    delay_ms(50);                               // LCD power-up delay
    lcd_init();
    lcd_cmd(0x80);
    lcd_print("ADC Diff (V):");

    // -------- ADC Setup (AD0.4 & AD0.5) --------
    LPC_PINCON->PINSEL3 |= (3 << 28) | (3 << 30); // P1.30 AD0.4, P1.31 AD0.5
    LPC_SC->PCONP |= (1 << 12);                  // Power up ADC
    LPC_ADC->ADCR = (1 << 21) | (4 << 8);        // Enable ADC, clk = PCLK/5

    while (1)
    {
        adc_val4 = read_adc(4);
        adc_val5 = read_adc(5);

        v4 = (adc_val4 * 3.3f) / 4095.0f;
        v5 = (adc_val5 * 3.3f) / 4095.0f;
        diff = fabsf(v4 - v5);

        sprintf(msg, "%.2f", diff);
        lcd_cmd(0xC0);
        lcd_print("                "); // Clear line 2
        lcd_cmd(0xC0);
        lcd_print(msg);

        delay_ms(500);
    }
}

// =====================================================
// FUNCTION: READ ADC VALUE
// =====================================================
unsigned int read_adc(uint8_t channel)
{
    unsigned int result;
    LPC_ADC->ADCR &= ~(0x7 << 0);        // Clear previous channel
    LPC_ADC->ADCR |= (1 << channel);     // Select channel
    LPC_ADC->ADCR |= (1 << 24);          // Start conversion
    while ((LPC_ADC->ADGDR & (1 << 31)) == 0);
    result = (LPC_ADC->ADGDR >> 4) & 0xFFF;
    LPC_ADC->ADCR &= ~(7 << 24);
    return result;
}

// =====================================================
// FUNCTION: LCD INITIALIZATION
// =====================================================
void lcd_init(void)
{
    delay_ms(15);      // Wait for LCD power-up

    lcd_cmd(0x33);     // Initialize in 4-bit mode
    lcd_cmd(0x32);     // Set to 4-bit interface
    lcd_cmd(0x28);     // 2 line, 5x8 font
    lcd_cmd(0x0C);     // Display ON, cursor OFF
    lcd_cmd(0x06);     // Auto increment cursor
    lcd_cmd(0x01);     // Clear display
    delay_ms(2);
}

// =====================================================
// FUNCTION: LCD COMMAND
// =====================================================
void lcd_cmd(unsigned char cmd)
{
    LPC_GPIO0->FIOCLR = RS_CTRL;   // RS=0 for command
    lcd_write_nibble(cmd >> 4);
    lcd_write_nibble(cmd & 0x0F);
    delay_ms(2);
}

// =====================================================
// FUNCTION: LCD DATA
// =====================================================
void lcd_data(unsigned char data)
{
    LPC_GPIO0->FIOSET = RS_CTRL;   // RS=1 for data
    lcd_write_nibble(data >> 4);
    lcd_write_nibble(data & 0x0F);
    delay_ms(2);
}

// =====================================================
// FUNCTION: LCD PRINT STRING
// =====================================================
void lcd_print(char *str)
{
    while (*str)
        lcd_data(*str++);
}

// =====================================================
// FUNCTION: WRITE NIBBLE TO LCD (P0.23–P0.26)
// =====================================================
void lcd_write_nibble(unsigned char nib)
{
    LPC_GPIO0->FIOCLR = DT_CTRL;                     // Clear previous data
    LPC_GPIO0->FIOSET = ((nib & 0x0F) << 23);        // Send upper/lower nibble

    LPC_GPIO0->FIOSET = EN_CTRL;                     // EN=1 pulse
    delay_lcd(500);
    LPC_GPIO0->FIOCLR = EN_CTRL;                     // EN=0
    delay_lcd(500);
}

// =====================================================
// FUNCTION: SMALL DELAY FOR LCD
// =====================================================
void delay_lcd(unsigned int r)
{
    for (unsigned int j = 0; j < r; j++);
}

// =====================================================
// FUNCTION: TIMER0 DELAY (ms)
// =====================================================
void delay_ms(unsigned int milliseconds)
{
    LPC_TIM0->TCR = 0x02;
    LPC_TIM0->PR  = 2999;
    LPC_TIM0->TCR = 0x01;
    while (LPC_TIM0->TC < milliseconds);
    LPC_TIM0->TCR = 0x00;
    LPC_TIM0->TC  = 0;
}
