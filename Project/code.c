```c
#include "LPC17xx.h"

// LCD Control Pins
#define RS_CTRL 0x08000000 // P0.27, 1<<27
#define EN_CTRL 0x10000000 // P0.28, 1<<28
#define DT_CTRL 0x07800000 // P0.23 to P0.26 data lines, F<<23

// Buzzer Pin
#define BUZZER_PIN (1 << 17) // P0.17

// PIR Sensor Pin
#define PIR_PIN (1 << 10) // P0.10

// Motion detection settings
#define MOTION_PERSISTENCE 50 // Number of cycles to maintain "motion detected" state

// Global variables
unsigned long int temp1 = 0, temp2 = 0, i, j;
unsigned char flag1 = 0, flag2 = 0;
unsigned char motion_msg[] = {"Motion Detected!"};
unsigned char no_motion_msg[] = {"Monitoring..."};
int motion_state;
int no_motion_counter = 0;

// LCD initialization commands
unsigned long int init_command[] = {0x30, 0x30, 0x30, 0x20, 0x28, 0x0c, 0x06, 0x01, 0x80};

// Function prototypes
void lcd_write(void);
void port_write(void);
void delay_lcd(unsigned int);
void lcd_puts(unsigned char *);
void lcd_clear(void);
void lcd_command(unsigned long int);
void buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);

int main(void) {
    // Initialize system
    SystemInit();
    SystemCoreClockUpdate();
   
    // Configure GPIO pins for LCD
    LPC_GPIO0->FIODIR = DT_CTRL | RS_CTRL | EN_CTRL;
   
    // Initialize buzzer
    buzzer_init();
   
    // Configure P0.10 as input for PIR sensor
    LPC_PINCON->PINSEL0 &= ~(3 << 20); // Clear bits 20-21 to use P0.10 as GPIO
    LPC_GPIO0->FIODIR &= ~PIR_PIN; // Set P0.10 as input
   
    // Initialize LCD
    flag1 = 0;
    for (i = 0; i < 9; i++) {
        temp1 = init_command[i];
        lcd_write();
    }
   
    // Display initial message
    lcd_clear();
    lcd_puts(no_motion_msg);
   
    motion_state = 0; // Track motion state to avoid repeated messages
    no_motion_counter = 0;
   
    while(1) {
        // Check PIR sensor state
        if(LPC_GPIO0->FIOPIN & PIR_PIN) {
            // Motion detected
            no_motion_counter = 0; // Reset the no-motion counter
           
            if(motion_state == 0) {
                lcd_clear();
                lcd_puts(motion_msg);
               
                // Turn ON buzzer
                buzzer_on();
               
                motion_state = 1;
            }
        } else {
            // No motion detected in this read
            if(motion_state == 1) {
                // Increment the counter of consecutive no-motion readings
                no_motion_counter++;
               
                // Only change state if we've had enough consecutive no-motion readings
                if(no_motion_counter >= MOTION_PERSISTENCE) {
                    lcd_clear();
                    lcd_puts(no_motion_msg);
                   
                    // Turn OFF buzzer
                    buzzer_off();
                   
                    motion_state = 0;
                    no_motion_counter = 0;
                }
            }
        }
       
        // Add a small delay to debounce and reduce CPU usage
        for(j = 0; j < 500000; j++); // Reduced delay for more responsive sensing
    }
}

// Function to initialize buzzer on P0.17
void buzzer_init(void) {
    // Configure P0.17 as GPIO output
    LPC_PINCON->PINSEL1 &= ~(3 << 2); // Clear bits 2-3 to use P0.17 as GPIO
    LPC_GPIO0->FIODIR |= BUZZER_PIN;  // Set P0.17 as output
   
    // Turn off buzzer initially
    buzzer_off();
}

// Function to turn on buzzer
void buzzer_on(void) {
    LPC_GPIO0->FIOSET = BUZZER_PIN;
}

// Function to turn off buzzer
void buzzer_off(void) {
    LPC_GPIO0->FIOCLR = BUZZER_PIN;
}

// Function to send command or data to LCD
void lcd_write(void) {
    flag2 = (flag1 == 1) ? 0 : ((temp1 == 0x30) || (temp1 == 0x20)) ? 1 : 0;
    temp2 = temp1 & 0xf0;
    temp2 = temp2 << 19;
    port_write();
   
    if (flag2 == 0) {
        temp2 = temp1 & 0x0f;
        temp2 = temp2 << 23;
        port_write();
    }
}

// Function to write to LCD port
void port_write(void) {
    LPC_GPIO0->FIOPIN = temp2;
   
    if (flag1 == 0)
        LPC_GPIO0->FIOCLR = RS_CTRL;
    else
        LPC_GPIO0->FIOSET = RS_CTRL;
   
    LPC_GPIO0->FIOSET = EN_CTRL;
    delay_lcd(25);
    LPC_GPIO0->FIOCLR = EN_CTRL;
    delay_lcd(30000);
}

// Function to display a string on LCD
void lcd_puts(unsigned char *string) {
    flag1 = 1; // Set to data mode
    i = 0;
    while (string[i] != '\0') {
        temp1 = string[i];
        lcd_write();
        i++;
    }
}

// Function to clear LCD screen
void lcd_clear(void) {
    flag1 = 0; // Set to command mode
    temp1 = 0x01; // Clear display command
    lcd_write();
    delay_lcd(50000); // Longer delay needed for clear command
}

// Function to send command to LCD
void lcd_command(unsigned long int command) {
    flag1 = 0; // Set to command mode
    temp1 = command;
    lcd_write();
}

// Delay function
void delay_lcd(unsigned int r1) {
    unsigned int r;
    for(r = 0; r < r1; r++);
    return;
}
```
