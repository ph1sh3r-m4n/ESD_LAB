#include <LPC17xx.h>

// ---------------- LCD Pin Definitions ----------------
#define RS_CTRL  0x08000000   // P0.27
#define EN_CTRL  0x10000000   // P0.28
#define DT_CTRL  0x07800000   // P0.23–P0.26 (Data lines)

// ---------------- Global Variables ----------------
unsigned long int temp1, temp2, i;
unsigned char flag1 = 0, flag2 = 0;
char key;

// ---------------- Function Prototypes ----------------
void delay_lcd(unsigned int);
void lcd_write(void);
void port_write(void);
void lcd_init(void);
void lcd_cmd(unsigned char);
void lcd_data(unsigned char);
char key_scan(void);

// =====================================================
// MAIN FUNCTION
// =====================================================
int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    // -------- LCD Pins Configuration (P0.23–P0.28) --------
    LPC_PINCON->PINSEL1 &= 0xFC003FFF;     // P0.23–P0.28 as GPIO
    LPC_GPIO0->FIODIR |= DT_CTRL | RS_CTRL | EN_CTRL; // Set as output

    // -------- Keypad Pins Configuration --------
    LPC_GPIO0->FIODIR |= (0x0F << 15);   // Rows P0.15–P0.18 output
    LPC_GPIO0->FIODIR &= ~(0x0F << 19);  // Cols P0.19–P0.22 input
    LPC_PINCON->PINMODE1 &= ~(0xFF << 6); // Enable pull-ups on P0.19–P0.22

    // -------- LCD Initialization --------
    lcd_init();

    // Display header
    flag1 = 1;
    temp1 = 'K'; lcd_write();
    temp1 = 'E'; lcd_write();
    temp1 = 'Y'; lcd_write();
    temp1 = ':'; lcd_write();

    while (1)
    {
        key = key_scan();   // Read the pressed key

        if (key != 'N')     // If valid key detected
        {
            lcd_cmd(0xC0);  // Move to 2nd line
            flag1 = 1;
            temp1 = key;
            lcd_write();    // Display key
            delay_lcd(1000000); // Debounce delay
        }
    }
}

// =====================================================
// FUNCTION: LCD INITIALIZATION (4-bit mode)
// =====================================================
void lcd_init(void)
{
    unsigned long int init_cmds[] = {
        0x30,0x30,0x30,0x20, // 4-bit mode set
        0x28,                // 2 lines, 5x7 matrix
        0x0C,                // Display ON, cursor OFF
        0x06,                // Entry mode set
        0x01,                // Clear display
        0x80                 // Cursor to first line
    };

    flag1 = 0; // Command mode
    for (i = 0; i < 9; i++)
    {
        temp1 = init_cmds[i];
        lcd_write();
    }
}

// =====================================================
// FUNCTION: LCD COMMAND (Wrapper)
// =====================================================
void lcd_cmd(unsigned char cmd)
{
    flag1 = 0;
    temp1 = cmd;
    lcd_write();
}

// =====================================================
// FUNCTION: LCD DATA (Wrapper)
// =====================================================
void lcd_data(unsigned char data)
{
    flag1 = 1;
    temp1 = data;
    lcd_write();
}

// =====================================================
// FUNCTION: LCD WRITE (Handles Command & Data)
// =====================================================
void lcd_write(void)
{
    flag2 = (flag1 == 1) ? 0 : ((temp1 == 0x30) || (temp1 == 0x20)) ? 1 : 0;

    // Send upper nibble
    temp2 = temp1 & 0xF0;
    temp2 = temp2 << 19;
    port_write();

    if (flag2 == 0)
    {
        // Send lower nibble
        temp2 = temp1 & 0x0F;
        temp2 = temp2 << 23;
        port_write();
    }
}

// =====================================================
// FUNCTION: PORT WRITE (Transfer data to LCD)
// =====================================================
void port_write(void)
{
    LPC_GPIO0->FIOPIN = temp2;

    if (flag1 == 0)
        LPC_GPIO0->FIOCLR = RS_CTRL; // Command mode
    else
        LPC_GPIO0->FIOSET = RS_CTRL; // Data mode

    LPC_GPIO0->FIOSET = EN_CTRL;     // EN = 1
    delay_lcd(25);
    LPC_GPIO0->FIOCLR = EN_CTRL;     // EN = 0
    delay_lcd(30000);
}

// =====================================================
// FUNCTION: SIMPLE DELAY
// =====================================================
void delay_lcd(unsigned int r)
{
    unsigned int j;
    for (j = 0; j < r; j++);
}

// =====================================================
// FUNCTION: MATRIX KEYPAD SCAN (4x4)
// =====================================================
char key_scan(void)
{
    unsigned long int col, row;
    char keypad[4][4] = {
        {'0','1','2','3'},
        {'4','5','6','7'},
        {'8','9','A','B'},
        {'C','D','E','F'}
    };

    for (row = 0; row < 4; row++)
    {
        LPC_GPIO0->FIOSET = (0x0F << 15);      // All rows HIGH
        LPC_GPIO0->FIOCLR = (1 << (15 + row)); // Current row LOW
        delay_lcd(2000);

        col = (LPC_GPIO0->FIOPIN >> 19) & 0x0F; // Read columns

        if (col != 0x0F)
        {
            delay_lcd(10000); // Debounce

            if (!(col & 0x01)) return keypad[row][0];
            if (!(col & 0x02)) return keypad[row][1];
            if (!(col & 0x04)) return keypad[row][2];
            if (!(col & 0x08)) return keypad[row][3];
        }
    }
    return 'N'; // No key pressed
}
