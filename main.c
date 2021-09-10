//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>

#include "Draw2D.h"
#include "LCD.h"
#include "MCU_Init.h"
#include "NUC100Series.h"
#include "SYS_init.h"
#include "picture.h"

#define SYSTICK_DLAY_us 1000000
#define Xmax 128
#define Ymax 64
#define Xmin 1
#define Ymin 1

//------------------------------------------------------------------------------------------------------------------------------------
// Functions declaration
//------------------------------------------------------------------------------------------------------------------------------------
void System_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(uint8_t PageAddr, uint8_t ColumnAddr);
void KeyPadEnable(void);
uint8_t KeyPadScanning(void);

//------------------------------------------------------------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------------------------------------------------------------
typedef enum {
    welcome_screen,
    game_rules,
    game_background,
    main_game,
    end_game
} STATES;

int main(void) {
    STATES game_state;
    uint8_t i, key_pressed = 0, game_pad = 0;
    uint8_t x = 64, y = 32, dx = 0, dy = 0;
    uint8_t x_steps, y_steps;
    uint8_t x_reset, y_reset;
    uint8_t hit;
    char score_txt[] = "0";
    //--------------------------------
    //System initialization
    //--------------------------------
    System_Config();
    KeyPadEnable();
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
    game_state = welcome_screen;
    x_steps = 2;
    y_steps = 2;
    x_reset = 64;
    y_reset = 32;
    hit = 0;
    while (1) {
        switch (game_state) {
            case welcome_screen:
                //welcome state code here
                draw_LCD(monster_128x64);
                for (i = 0; i < 3; i++)
                    CLK_SysTickDelay(SYSTICK_DLAY_us);
                LCD_clear();
                game_state = game_rules;  // state transition
                break;
            case game_rules:
                // game_rules state code here
                printS_5x7(1, 0, "Use Keypad to control");
                printS_5x7(1, 8, "2: UP 4: LEFT");
                printS_5x7(1, 16, "6: RIGHT 8: DOWN");
                printS_5x7(1, 32, "Eat all boxes to win");
                printS_5x7(1, 56, "press any key to continue!");
                while (key_pressed == 0)
                    key_pressed = KeyPadScanning();
                key_pressed = 0;
                LCD_clear();
                game_state = game_background;
                break;
            case game_background:
                // static display information should be here
                sprintf(score_txt, "%d", hit);
                printS_5x7(48, 0, score_txt);
                fill_Rectangle(8, 8, 8 + 6, 8 + 6, 1, 0);
                fill_Rectangle(96, 8, 96 + 6, 8 + 6, 1, 0);
                fill_Rectangle(8, 48, 8 + 6, 48 + 6, 1, 0);
                fill_Rectangle(96, 48, 96 + 6, 48 + 6, 1, 0);
                game_state = main_game;
            case main_game:
                //main_game state code here
                //draw objects
                fill_Rectangle(x, y, x + 6, y + 6, 1, 0);
                //delay for vision
                while (game_pad == 0)
                    game_pad = KeyPadScanning();
                CLK_SysTickDelay(170000);
                //erase the objects
                fill_Rectangle(x, y, x + 6, y + 6, 0, 0);
                //check changes
                switch (game_pad) {
                    //case 1: dx =+1; dy =-1; break;
                    case 2:
                        dx = 0;
                        dy = -1;
                        break;
                    //case 3: dx = +1; dy = -1; break;
                    case 4:
                        dx = -1;
                        dy = 0;
                        break;
                    //case 5: break;
                    case 6:
                        dx = +1;
                        dy = 0;
                        break;
                    //case 7: dx= -1; dy= +1; break;
                    case 8:
                        dx = 0;
                        dy = +1;
                        break;
                    //case 9: dx=+1; dy=+1; break;
                    default:
                        dx = dy = 0;
                        break;
                }
                game_pad = 0;
                //update object’s information (position, etc.)
                x = x + (dx * x_steps);
                y = y + (dy * y_steps);
                //boundary condition
                //wrap around on X
                if (x < Xmin)
                    x = Xmax - 7;
                if (x > Xmax - 7)
                    x = Xmin;
                //wrap around on Y
                if (y < Ymin)
                    y = Ymax - 7;
                if (y > Ymax - 7)
                    y = Ymin;
                //score conditions
                if ((x - 8 < 6) && (y - 8 < 6)) {
                    hit++;
                    x = x_reset;
                    y = y_reset;
                }
                if (hit > 1) {
                    hit = 0;
                    LCD_clear();
                    game_state = end_game;
                } else
                    game_state = game_background;
                break;
            case end_game:
                //end_game code here
                printS_5x7(1, 32, "press any key to replay!");
                for (i = 0; i < 2; i++)
                    CLK_SysTickDelay(SYSTICK_DLAY_us);
                while (key_pressed == 0)
                    key_pressed = KeyPadScanning();
                key_pressed = 0;
                LCD_clear();
                game_state = welcome_screen;
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
    LCD_command(0xE2);  // Set system reset
    LCD_command(0xA1);  // Set Frame rate 100 fps
    LCD_command(0xEB);  // Set LCD bias ratio E8~EB for 6~9 (min~max)
    LCD_command(0x81);  // Set V BIAS potentiometer
    LCD_command(0xA0);  // Set V BIAS potentiometer: A0 ()
    LCD_command(0xC0);
    LCD_command(0xAF);  // Set Display Enable
}

void LCD_command(unsigned char temp) {
    SPI3->SSR |= 1ul << 0;
    SPI3->TX[0] = temp;
    SPI3->CNTRL |= 1ul << 0;
    while (SPI3->CNTRL & (1ul << 0))
        ;
    SPI3->SSR &= ~(1ul << 0);
}

void LCD_data(unsigned char temp) {
    SPI3->SSR |= 1ul << 0;
    SPI3->TX[0] = 0x0100 + temp;
    SPI3->CNTRL |= 1ul << 0;
    while (SPI3->CNTRL & (1ul << 0))
        ;
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
    SYS_UnlockReg();  // Unlock protected registers
    // Enable clock sources
    //LXT - External Low frequency Crystal 32 kHz
    CLK->PWRCON |= (0x01ul << 1);
    while (!(CLK->CLKSTATUS & (1ul << 1)));
    CLK->PWRCON |= (0x01ul << 0);
    while (!(CLK->CLKSTATUS & (1ul << 0)));
    //PLL configuration starts
    CLK->PLLCON &= ~(1ul << 19);  //0: PLL input is HXT
    CLK->PLLCON &= ~(1ul << 16);  //PLL in normal mode
    CLK->PLLCON &= (~(0x01FFul << 0));
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1ul << 18);  //0: enable PLLOUT
    while (!(CLK->CLKSTATUS & (0x01ul << 2)));
    //PLL configuration ends
    //clock source selection
    CLK->CLKSEL0 &= (~(0x07ul << 0));
    CLK->CLKSEL0 |= (0x02ul << 0);
    //clock frequency division
    CLK->CLKDIV &= (~0x0Ful << 0);
    //enable clock of SPI3
    CLK->APBCLK |= 1ul << 15;
    SYS_LockReg();  // Lock protected registers
}

void SPI3_Config(void) {
    SYS->GPD_MFP |= 1ul << 11;    //1: PD11 is configured for alternative function
    SYS->GPD_MFP |= 1ul << 9;     //1: PD9 is configured for alternative function
    SYS->GPD_MFP |= 1ul << 8;     //1: PD8 is configured for alternative function
    SPI3->CNTRL &= ~(1ul << 23);  //0: disable variable clock feature
    SPI3->CNTRL &= ~(1ul << 22);  //0: disable two bits transfer mode
    SPI3->CNTRL &= ~(1ul << 18);  //0: select Master mode
    SPI3->CNTRL &= ~(1ul << 17);  //0: disable SPI interrupt
    SPI3->CNTRL |= 1ul << 11;     //1: SPI clock idle high
    SPI3->CNTRL &= ~(1ul << 10);  //0: MSB is sent first
    SPI3->CNTRL &= ~(3ul << 8);   //00: one transmit/receive word will be executed in one data transfer
    SPI3->CNTRL &= ~(31ul << 3);  //Transmit/Receive bit length
    SPI3->CNTRL |= 9ul << 3;      //9: 9 bits transmitted/received per data transfer
    SPI3->CNTRL |= (1ul << 2);    //1: Transmit at negative edge of SPI CLK
    SPI3->DIVIDER = 0;            // SPI clock divider. SPI clock = HCLK / ((DIVIDER+1)*2). HCLK = 50 MHz
}