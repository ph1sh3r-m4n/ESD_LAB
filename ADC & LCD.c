#include <LPC17xx.h>
#include <stdio.h>
#include <math.h>

// LCD connections on CND port
#define LCD_DATA_MASK (0xF << 23) // P0.23 to P0.26 as D4–D7
#define LCD_RS (1 << 27)          // P0.27
#define LCD_EN (1 << 28)          // P0.28

char lcdBuffer[32];
float v4, v5, diff;
unsigned int adc4, adc5;

// ---------- Delay Functions ----------
void delay(unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; i++);
}

void delayUS(unsigned int us)
{
    LPC_TIM0->TCR = 0x02; // reset timer
    LPC_TIM0->PR = 24;    // prescaler for 1µs tick @ 25MHz PCLK
    LPC_TIM0->TCR = 0x01; // start
    while (LPC_TIM0->TC < us);
    LPC_TIM0->TCR = 0x00; // stop timer
}

void initTimer0(void)
{
    LPC_SC->PCONP |= (1 << 1); // power on Timer0
    LPC_TIM0->CTCR = 0x0;      // timer mode
    LPC_TIM0->PR = 0;
    LPC_TIM0->TCR = 0x02;      // reset timer
}

// ---------- LCD Functions ----------
void lcd_pulse_enable(void)
{
    LPC_GPIO0->FIOSET = LCD_EN;
    delayUS(1);
    LPC_GPIO0->FIOCLR = LCD_EN;
    delayUS(100);
}

void lcd_send_nibble(unsigned char nibble)
{
    LPC_GPIO0->FIOCLR = LCD_DATA_MASK;
    LPC_GPIO0->FIOSET = ((nibble & 0x0F) << 23);
    lcd_pulse_enable();
}

void lcd_send_byte(unsigned char byte, int is_data)
{
    if (is_data)
        LPC_GPIO0->FIOSET = LCD_RS;
    else
        LPC_GPIO0->FIOCLR = LCD_RS;

    lcd_send_nibble(byte >> 4);
    lcd_send_nibble(byte & 0x0F);
}

void lcd_command(unsigned char cmd)
{
    lcd_send_byte(cmd, 0);
    delay(2000);
}

void lcd_data(unsigned char data)
{
    lcd_send_byte(data, 1);
    delay(2000);
}

void lcd_init(void)
{
    LPC_GPIO0->FIODIR |= LCD_DATA_MASK | LCD_RS | LCD_EN;
    delay(30000);
    lcd_command(0x33);
    lcd_command(0x32);
    lcd_command(0x28); // 4-bit, 2 line, 5x7 font
    lcd_command(0x0C); // Display ON, cursor off
    lcd_command(0x06); // Entry mode
    lcd_command(0x01); // Clear display
    delay(3000);
}

void lcd_string(char *str)
{
    while (*str)
        lcd_data(*str++);
}

// ---------- ADC Function ----------
unsigned int read_adc(unsigned int channel)
{
    unsigned int result;
    LPC_ADC->ADCR = (1 << channel) | (4 << 8) | (1 << 21); // select ch, clkdiv, enable
    LPC_ADC->ADCR |= (1 << 24); // start conversion
    while (!(LPC_ADC->ADGDR & (1U << 31))); // wait for done
    result = (LPC_ADC->ADGDR >> 4) & 0xFFF; // get 12-bit result
    LPC_ADC->ADCR &= ~(7 << 24); // stop conversion
    return result;
}

// ---------- MAIN ----------
int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();
    initTimer0();

    // LCD init
    lcd_init();

    // ADC setup (AD0.4 on P1.30 and AD0.5 on P1.31)
    LPC_PINCON->PINSEL3 |= (3 << 28) | (3 << 30); // set P1.30, P1.31 as AD0.4, AD0.5
    LPC_SC->PCONP |= (1 << 12); // power up ADC block

    while (1)
    {
        // Read both channels
        adc4 = read_adc(4);
        adc5 = read_adc(5);

        // Convert to voltage
        v4 = (adc4 * 3.3f) / 4095.0f;
        v5 = (adc5 * 3.3f) / 4095.0f;
        diff = fabsf(v4 - v5);

        // Display on LCD
        lcd_command(0x80); // Line 1
        sprintf(lcdBuffer, "ADC4:%4u ADC5:%4u", adc4, adc5);
        lcd_string(lcdBuffer);

        lcd_command(0xC0); // Line 2
        sprintf(lcdBuffer, "Diff: %.2f V   ", diff);
        lcd_string(lcdBuffer);

        delay(1000000);
    }
}
