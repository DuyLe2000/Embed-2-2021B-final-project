//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_Init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"

#define SYSTICK_DLAY_us 1000000
#define Xmax 128
#define Ymax 64
#define Xmin 1
#define Ymin 1
#define NO_INPUT -1;
#define STAR -2;

//------------------------------------------------------------------------------------------------------------------------------------
// Functions declaration
//------------------------------------------------------------------------------------------------------------------------------------
void System_Config(void);
void timer_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void KeyPadEnable(void);
uint8_t KeyPadScanning(void);
int str_compare(char *str1, char *str2);
void str_copy(char *str1, char *str2);
int str_len(char *str);


//------------------------------------------------------------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------------------------------------------------------------
typedef enum{
    option,
    unlock,
    change_key,
    welcome,
    wrong_key
} STATES;


int main(void) {
    STATES lock_state;
    char password[6] = "757333";
    char user_input[6] = "";
    uint8_t key_press = 0;
    //--------------------------------
    //System initialization
    //--------------------------------
    System_Config();
    KeyPadEnable();
    // Config timer for debounce
    TIMER0->TCSR |= (1 << 26); // Reset timer
    TIMER0->TCSR |= ~(3ul << 27); // One shot mode
    TIMER0->TCSR &= ~(0xFF << 0); // Set prescale to 0
    TIMER0->TCSR |= (1 << 16);

    TIMER0->TCMPR &= ~(0xFFFFFFF << 0);
    TIMER0->TCMPR |= 0x5B8D80; // 500 ms

    //--------------------------------
    //SPI3 initialization
    //--------------------------------
    SPI3_Config();
    //--------------------------------
    //LCD initialization
    //--------------------------------
    LCD_start();
    LCD_clear();
    //--------------------------------
    //LCD static content
    //--------------------------------
    //--------------------------------
    //LCD dynamic content
    //--------------------------------
    lock_state = option;

    int8_t current_char = 0;
    char key_display[6] = "______";
    int8_t key_delay[6] = {-1, -1, -1 , -1, -1, -1};

    while (1) {
        switch (lock_state) {
            case option: {
                // Option screen

                // Reset all user input value
                current_char = 0;
                str_copy(key_display, "______");
                for (int i = 0; i < 6; i++) {
                    key_delay[i] = NO_INPUT;
                }

                printS_5x7(1, 1, "EEET2481 - Door Lock System");
                printS_5x7(1, 10, "Please select");
                printS_5x7(1, 19, "1: Unlock | 2: Change key");
                while (key_press != 1 && key_press != 2 && !(TIMER0->TCSR & (1 << 25))) {
                    TIMER0->TCSR |= (1 << 30);
                    key_press = KeyPadScanning();
                }

                if (key_press == 1) {
                    // Clear the screen
                    LCD_clear();
                    lock_state = unlock;
                } else if (key_press == 2) {
                    // Clear the screen
                    LCD_clear();
                    lock_state = change_key;
                }
                key_press = 0;
                break;
            } case unlock: {
                // Unlock enter key screen
                printS_5x7(1, 1, "EEET2481 - Door Lock System");
                printS_5x7(1, 9, "Please input your key");
                printC_5x7(1, 17, key_display[0]);
                printC_5x7(8, 17, key_display[1]);
                printC_5x7(15, 17, key_display[2]);
                printC_5x7(22, 17, key_display[3]);
                printC_5x7(29, 17, key_display[4]);
                printC_5x7(36, 17, key_display[5]);

                if (!(PB->PIN & (1 << 15))) {
                    lock_state = option;
                    break;
                }

                if (!(TIMER0->TCSR & (1 << 25))) {
                    TIMER0->TCSR |= (1 << 30);
                    key_press = KeyPadScanning();
                }

                if (key_press != 0) {
                    key_display[current_char] = key_press + '0';
                    user_input[current_char] = key_press + '0';
                    current_char++;
                }
                key_press = 0;
                
                if (current_char == 6) {
                    if (str_compare(user_input, password)) {
                        printC_5x7(36, 17, key_display[5]);
                        LCD_clear();
                        lock_state = welcome;
                    } else {
                        printC_5x7(36, 17, key_display[5]);
                        LCD_clear();
                        lock_state = wrong_key;
                    }
                }
                break;
            } case change_key: {
                // Change key enter screen
                printS_5x7(1, 1, "EEET2481 - Door Lock System");
                printS_5x7(1, 9, "Please input new 6-digit key");
                printC_5x7(1, 17, key_display[0]);
                printC_5x7(8, 17, key_display[1]);
                printC_5x7(15, 17, key_display[2]);
                printC_5x7(22, 17, key_display[3]);
                printC_5x7(29, 17, key_display[4]);
                printC_5x7(36, 17, key_display[5]);

                if (!(TIMER0->TCSR & (1 << 25))) {
                    TIMER0->TCSR |= (1 << 30);
                    key_press = KeyPadScanning();
                }

                if (key_press != 0) {
                    key_display[current_char] = key_press + '0';
                    user_input[current_char] = key_press + '0';
                    current_char++;
                }
                key_press = 0;

                if (current_char == 6) {
                    str_copy(password, user_input);
                    LCD_clear();
                    printS_5x7(1, 1, "EEET2481 - Door Lock System");
                    printS_5x7(1, 9, "Your key has been changed!");
                    printS_5x7(1, 17, "THANK YOU!");
                    CLK_SysTickDelay(800000);
                    LCD_clear();
                    lock_state = option;
                }

                if (!(PB->PIN & (1 << 15))) {
                    lock_state = option;
                }
				break;
            } case welcome:
                // Unlock successful screen
                printS_5x7(1, 1, "EEET2481 - Door Lock System");
                printS_5x7(1, 9, "Welcome");
                if (!(PB->PIN & (1 << 15))) {
                    LCD_clear();
                    lock_state = option;
                }
                break;
            case wrong_key:
                // Unlock failed screen
                printS_5x7(1, 1, "EEET2481 - Door Lock System");
                printS_5x7(1, 9, "The key is wrong");
                printS_5x7(1, 17, "System restart in 1 second.");
                printS_5x7(1, 15, "Thank you!");

                if (!(PB->PIN & (1 << 15))) {
                    LCD_clear();
                    lock_state = option;
                }
                break;
            default:
                break;
        }
    }
}
//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------
void LCD_start(void) {
    LCD_command(0xE2); // Set system reset
    LCD_command(0xA1); // Set Frame rate 100 fps
    LCD_command(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)
    LCD_command(0x81); // Set V BIAS potentiometer
    LCD_command(0xA0); // Set V BIAS potentiometer: A0 ()
    LCD_command(0xC0);
    LCD_command(0xAF); // Set Display Enable
}


void LCD_command(unsigned char temp) {
    SPI3->SSR |= 1ul << 0;
    SPI3->TX[0] = temp;
    SPI3->CNTRL |= 1ul << 0;
    while (SPI3->CNTRL & (1ul << 0));
    SPI3->SSR &= ~(1ul << 0);
}


void LCD_data(unsigned char temp) {
    SPI3->SSR |= 1ul << 0;
    SPI3->TX[0] = 0x0100 + temp;
    SPI3->CNTRL |= 1ul << 0;
    while (SPI3->CNTRL & (1ul << 0));
    SPI3->SSR &= ~(1ul << 0);
}


void LCD_clear(void) {
    int16_t i;
    LCD_SetAddress(0x0, 0x0);
    for (i = 0; i < 132 * 8; i++) {
        LCD_data(0x00);
    }
}


void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr) {
    LCD_command(0xB0 | PageAddr);
    LCD_command(0x10 | (ColumnAddr >> 4) & 0xF);
    LCD_command(0x00 | (ColumnAddr & 0xF));
}


void KeyPadEnable(void) {
    GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_INPUT);
}


uint8_t KeyPadScanning(void) {
    PA0 = 1;
    PA1 = 1;
    PA2 = 0;
    PA3 = 1;
    PA4 = 1;
    PA5 = 1;
    if (PA3 == 0)
        return 1;
    if (PA4 == 0)
        return 4;
    if (PA5 == 0)
        return 7;
    PA0 = 1;
    PA1 = 0;
    PA2 = 1;
    PA3 = 1;
    PA4 = 1;
    PA5 = 1;
    if (PA3 == 0)
        return 2;
    if (PA4 == 0)
        return 5;
    if (PA5 == 0)
        return 8;
    PA0 = 0;
    PA1 = 1;
    PA2 = 1;
    PA3 = 1;
    PA4 = 1;
    PA5 = 1;
    if (PA3 == 0)
        return 3;
    if (PA4 == 0)
        return 6;
    if (PA5 == 0)
        return 9;
    
    return 0;
}


void System_Config(void) {
    SYS_UnlockReg(); // Unlock protected registers
    // Enable clock sources
    //LXT - External Low frequency Crystal 32 kHz
    CLK->PWRCON |= (0x01ul << 1);
    while (!(CLK->CLKSTATUS & (1ul << 1)));
    CLK->PWRCON |= (0x01ul << 0);
    while (!(CLK->CLKSTATUS & (1ul << 0)));
    //PLL configuration starts
    CLK->PLLCON &= ~(1ul << 19); //0: PLL input is HXT
    CLK->PLLCON &= ~(1ul << 16); //PLL in normal mode
    CLK->PLLCON &= (~(0x01FFul << 0));
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1ul << 18); //0: enable PLLOUT
    while (!(CLK->CLKSTATUS & (0x01ul << 2)));
    //PLL configuration ends
    //clock source selection
    CLK->CLKSEL0 &= (~(0x07ul << 0));
    CLK->CLKSEL0 |= (0x02ul << 0); // 50 Mhz
    //clock frequency division
    CLK->CLKDIV &= (~0x0Ful << 0);
    //enable clock of SPI3
    CLK->APBCLK |= 1ul << 15;
    // Set 12Mhz clock for timer 0
    CLK->CLKSEL1 &= ~(0b111 << 8);
    SYS_LockReg(); // Lock protected registers
}


void SPI3_Config(void) {
    SYS->GPD_MFP |= 1ul << 11;   //1: PD11 is configured for alternative function
    SYS->GPD_MFP |= 1ul << 9;    //1: PD9 is configured for alternative function
    SYS->GPD_MFP |= 1ul << 8;    //1: PD8 is configured for alternative function
    SPI3->CNTRL &= ~(1ul << 23); //0: disable variable clock feature
    SPI3->CNTRL &= ~(1ul << 22); //0: disable two bits transfer mode
    SPI3->CNTRL &= ~(1ul << 18); //0: select Master mode
    SPI3->CNTRL &= ~(1ul << 17); //0: disable SPI interrupt
    SPI3->CNTRL |= 1ul << 11;    //1: SPI clock idle high
    SPI3->CNTRL &= ~(1ul << 10); //0: MSB is sent first
    SPI3->CNTRL &= ~(3ul << 8);  //00: one transmit/receive word will be executed in one data transfer
    SPI3->CNTRL &= ~(31ul << 3); //Transmit/Receive bit length
    SPI3->CNTRL |= 9ul << 3;     //9: 9 bits transmitted/received per data transfer
    SPI3->CNTRL |= (1ul << 2);   //1: Transmit at negative edge of SPI CLK
    SPI3->DIVIDER = 0;           // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}


int str_compare(char *str1, char *str2) {
    /*
        Compare 2 string. Return 1 if equal, 0 if not equal
    */
    int str1_length = str_len(str1);
    int str2_length = str_len(str2);
    if (str1_length != str2_length) {
        return 0;
    }
    for (int i = 0; i < str1_length; i++) {
        if (*(str1 + i) != *(str2 + i)) {
            return 0;
        }
    }
    return 1;
}


int str_len(char *str) {
    /*
        Find length of a given string. Return length of string
    */
    int str_lenght = 0;
    for (int i = 0; *(str + i) != '\0'; i++) {
        str_lenght++;
    }
    return str_lenght;
}


void str_copy(char *str1, char *str2) {
    /*
        Copy str2 into str1
    */
    int str2_lenght = str_len(str2);
    for (int i = 0; i <= str2_lenght; i++) {
        *(str1 + i) = *(str2 + i);
    }
}
