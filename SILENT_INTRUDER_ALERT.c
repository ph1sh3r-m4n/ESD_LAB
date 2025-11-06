#include <LPC17xx.h>
#include <stdio.h>
#include <math.h>

/* ---------- LCD on CNA ----------
   D4–D7 : P0.4–P0.7
   RS    : P0.8
   EN    : P0.9
----------------------------------*/
#define LCD_DATA (0x0F << 4)
#define LCD_RS   (1 << 8)
#define LCD_EN   (1 << 9)

/* ---------- Buzzer pin ---------- */
#define BUZZER_PIN (1 << 22)

/* ---------- Switch pin (P2.12 = SW1) ---------- */
#define RESET_SW (1 << 12)

/* ---------- Globals ---------- */
unsigned int adcVal;
float volts;
char lcdBuffer[32];
unsigned char counter = 0;
unsigned char intruder_state = 0;   // 0 = safe, 1 = intruder

/* ---------- Delay ---------- */
void delay(unsigned long d){ unsigned long x; for(x=0;x<d;x++); }

/* ---------- LCD helpers ---------- */
void lcd_pulse(void){
    LPC_GPIO0->FIOSET = LCD_EN; delay(100);
    LPC_GPIO0->FIOCLR = LCD_EN; delay(1000);
}

void lcd_write_nibble(unsigned char nibble){
    LPC_GPIO0->FIOCLR = LCD_DATA;
    LPC_GPIO0->FIOSET = ((nibble & 0x0F) << 4);
    lcd_pulse();
}

void lcd_write_byte(unsigned char val, int is_data){
    if(is_data) LPC_GPIO0->FIOSET = LCD_RS;
    else        LPC_GPIO0->FIOCLR = LCD_RS;
    lcd_write_nibble(val >> 4);
    lcd_write_nibble(val & 0x0F);
}

void lcd_cmd(unsigned char c){ lcd_write_byte(c,0); delay(2000); }
void lcd_data(unsigned char c){ lcd_write_byte(c,1); delay(2000); }

void lcd_init(void){
    LPC_GPIO0->FIODIR |= LCD_DATA | LCD_RS | LCD_EN;
    delay(30000);
    lcd_cmd(0x33);
    lcd_cmd(0x32);
    lcd_cmd(0x28);   // 4-bit, 2-line
    lcd_cmd(0x0C);   // Display ON, Cursor OFF
    lcd_cmd(0x06);   // Entry mode
    lcd_cmd(0x01);   // Clear
    delay(3000);
}

void lcd_string(const char *s){ while(*s) lcd_data(*s++); }

/* ---------- ADC on AD0.2 (P0.25) ---------- */
void initADC(void){
    LPC_SC->PCONP |= (1 << 12);                // Power ADC
    LPC_PINCON->PINSEL1 &= ~(3 << 18);         // Clear bits for P0.25
    LPC_PINCON->PINSEL1 |=  (1 << 18);         // 01 -> P0.25 as AD0.2
    LPC_ADC->ADCR = (1 << 2) | (4 << 8) | (1 << 21);  // channel 2, clkdiv, PDN
}

unsigned int readADC(void){
    LPC_ADC->ADCR &= ~(7 << 24);
    LPC_ADC->ADCR |=  (1 << 24);               // Start conversion
    while(!(LPC_ADC->ADGDR & (1UL << 31)));    // Wait for DONE
    return (LPC_ADC->ADGDR >> 4) & 0xFFF;      // 12-bit result
}

/* ---------- Buzzer control ---------- */
void buzzer_init(void){
    LPC_PINCON->PINSEL1 &= ~(3 << 12);  // P0.22 as GPIO
    LPC_GPIO0->FIODIR |= BUZZER_PIN;    // Output
    LPC_GPIO0->FIOCLR = BUZZER_PIN;     // Initially OFF
}
void buzzer_on(void){ LPC_GPIO0->FIOSET = BUZZER_PIN; }
void buzzer_off(void){ LPC_GPIO0->FIOCLR = BUZZER_PIN; }

/* ---------- SW1 (Reset) ---------- */
void switch_init(void){
    LPC_PINCON->PINSEL4 &= ~(3 << 24);   // P2.12 as GPIO
    LPC_GPIO2->FIODIR &= ~(RESET_SW);    // Input
}

/* ---------- MAIN ---------- */
int main(void){
    SystemInit();
    SystemCoreClockUpdate();

    lcd_init();
    initADC();
    buzzer_init();
    switch_init();

    lcd_cmd(0x80);
    lcd_string("Silent Intruder");

    while(1){
        adcVal = readADC();
        volts  = (adcVal * 3.3f) / 4095.0f;

        lcd_cmd(0xC0);
        sprintf(lcdBuffer, "Val:%4u  %.2fV", adcVal, volts);
        lcd_string(lcdBuffer);

        delay(1000000);   // Small delay for readability

        /* --- Intruder detection --- */
        if(adcVal < 3650 && intruder_state == 0){  
            intruder_state = 1;
            buzzer_on();

            lcd_cmd(0x01);         // Clear display
            delay(50000);          // Wait for LCD to clear
            lcd_cmd(0x80);        
            lcd_string("INTRUDER ALERT!!");
        }

        /* --- Manual reset using SW1 (P2.12) --- */
        if(intruder_state == 1){
            // Wait here until button pressed
            if((LPC_GPIO2->FIOPIN & RESET_SW) == 0){  // Active LOW
                buzzer_off();
                intruder_state = 0;

                lcd_cmd(0x01);  // Clear LCD after reset
                delay(50000);
                lcd_cmd(0x80);
                lcd_string("SYSTEM RESET OK");
                delay(2000000);
            }
            else{
                continue;  // Remain in alert mode until pressed
            }
        }

        /* --- Normal mode --- */
        if(intruder_state == 0){
            lcd_cmd(0x80);
            lcd_string("COUNTER: ");
            lcd_data(counter + '0');
            counter++;
            if(counter > 9) counter = 0;
        }

        delay(2000000);
    }
}
