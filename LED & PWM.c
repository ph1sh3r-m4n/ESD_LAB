#include <LPC17xx.h>

// Function declarations
void pwm_init(void);
void PWM1_IRQHandler(void);

// Global variables
unsigned long int i;
unsigned char flag = 0x00, flag1 = 0x00;

// =========================================
// MAIN FUNCTION
// =========================================
int main(void)
{
    SystemInit();             // Initialize system clock
    SystemCoreClockUpdate();  // Update system core clock variable

    pwm_init();               // Initialize and start PWM

    while(1)
    {
        for(i = 0; i <= 1000; i++); // Small delay (main loop is idle)
    }
}

// =========================================
// FUNCTION: Initialize PWM1 Module
// =========================================
void pwm_init(void)
{
    LPC_SC->PCONP |= (1 << 6);        // Power up PWM1 peripheral (bit 6 in PCONP)

    // -------- Configure pin P1.23 as PWM1.4 output --------
    LPC_PINCON->PINSEL3 &= ~(0x0000C000); // Clear bits 15:14 (for P1.23)
    LPC_PINCON->PINSEL3 |=  (0x00008000); // Set bits 15:14 = 10 (Function 2 → PWM1.4)

    // -------- PWM Configuration --------
    LPC_PWM1->PCR = 0x00001000;      // Enable PWM1.4 output (bit 12)
    LPC_PWM1->MCR = 0x00000003;      // Enable interrupt + reset on MR0
                                     // Bit0=1 → interrupt on MR0
                                     // Bit1=1 → reset on MR0 match

    LPC_PWM1->MR0 = 30000;           // Set PWM period (controls frequency)
                                     // The timer resets every 30000 counts

    LPC_PWM1->MR4 = 0x00000100;      // Initial duty cycle value (LED dim at start)
                                     // MR4 defines ON time for PWM1.4

    LPC_PWM1->LER = 0x000000FF;      // Latch enable register → update all MRx

    // -------- Enable and Start PWM --------
    LPC_PWM1->TCR = 0x00000002;      // Reset PWM timer and prescaler
    LPC_PWM1->TCR = 0x00000009;      // Enable counter (bit0) + PWM mode (bit3)

    NVIC_EnableIRQ(PWM1_IRQn);       // Enable PWM1 interrupt in NVIC
    return;
}

// =========================================
// INTERRUPT HANDLER: PWM1_IRQHandler
// =========================================
void PWM1_IRQHandler(void)
{
    LPC_PWM1->IR = 0xFF;             // Clear all PWM interrupt flags

    // ---- Brightness Increasing ----
    if(flag == 0x00)
    {
        LPC_PWM1->MR4 += 100;        // Increment duty cycle
        LPC_PWM1->LER = 0x000000FF;  // Load new MR4 value to shadow register

        if(LPC_PWM1->MR4 >= 27000)   // If near full brightness (almost 90%)
        {
            flag1 = 0xFF;             // Set flag1 (start decreasing soon)
            flag = 0xFF;              // Mark increasing phase as done
            LPC_PWM1->LER = 0x000000FF;
        }
    }

    // ---- Brightness Decreasing ----
    else if(flag1 == 0xFF)
    {
        LPC_PWM1->MR4 -= 100;        // Decrement duty cycle
        LPC_PWM1->LER = 0x000000FF;  // Load new MR4 value

        if(LPC_PWM1->MR4 <= 0x0500)  // If dim enough, reset flags
        {
            flag  = 0x00;             // Start increasing next time
            flag1 = 0x00;
            LPC_PWM1->LER = 0x000000FF;
        }
    }
}
