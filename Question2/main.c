//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_Init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"

#include "picture.h"

#define SYSTICK_DLAY_us 1000000
#define Xmax 127
#define Ymax 63
#define Xmin 0
#define Ymin 8
//------------------------------------------------------------------------------------------------------------------------------------
// Functions declaration
//------------------------------------------------------------------------------------------------------------------------------------
void System_Config(void);
void SPI3_Config(void);
void LCD_start(void);
void LCD_command(unsigned char temp);
void LCD_data(unsigned char temp);
void LCD_clear(void);
void LCD_SetAddress(int PageAddr, int ColumnAddr);
void KeyPadEnable(void);
int KeyPadScanning(void);

//------------------------------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------------------------------
void LCD_start(void){
    LCD_command(0xE2); // Set system reset
    LCD_command(0xA1); // Set Frame rate 100 fps
    LCD_command(0xEB); // Set LCD bias ratio E8~EB for 6~9 (min~max)
    LCD_command(0x81); // Set V BIAS potentiometer
    LCD_command(0xA0); // Set V BIAS potentiometer: A0 ()
    LCD_command(0xC0);
    LCD_command(0xAF); // Set Display Enable
}
void LCD_command(unsigned char temp){
    SPI3->SSR |= 1ul << 0;
    SPI3->TX[0] = temp;
    SPI3->CNTRL |= 1ul << 0;
    while (SPI3->CNTRL & (1ul << 0))
        ;
    SPI3->SSR &= ~(1ul << 0);
}
void LCD_data(unsigned char temp){
    SPI3->SSR |= 1ul << 0;
    SPI3->TX[0] = 0x0100 + temp;
    SPI3->CNTRL |= 1ul << 0;
    while (SPI3->CNTRL & (1ul << 0))
        ;
    SPI3->SSR &= ~(1ul << 0);
}
void LCD_clear(void){
    int16_t i;
    LCD_SetAddress(0x0, 0x0);
    for (i = 0; i < 132 * 8; i++)
    {
        LCD_data(0x00);
    }
}
void LCD_SetAddress(int PageAddr, int ColumnAddr){
    LCD_command(0xB0 | PageAddr);
    LCD_command(0x10 | (ColumnAddr >> 4) & 0xF);
    LCD_command(0x00 | (ColumnAddr & 0xF));
}
void KeyPadEnable(void){
    GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
    GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
}
int KeyPadScanning(void){
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
void System_Config(void){
    SYS_UnlockReg(); // Unlock protected registers
    // Enable clock sources
    //LXT - External Low frequency Crystal 32 kHz
    CLK->PWRCON |= (0x01ul << 1);
    while (!(CLK->CLKSTATUS & (1ul << 1)))
        ;
    CLK->PWRCON |= (0x01ul << 0);
    while (!(CLK->CLKSTATUS & (1ul << 0)))
        ;
    //PLL configuration starts
    CLK->PLLCON &= ~(1ul << 19); //0: PLL input is HXT
    CLK->PLLCON &= ~(1ul << 16); //PLL in normal mode
    CLK->PLLCON &= (~(0x01FFul << 0));
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1ul << 18); //0: enable PLLOUT
    while (!(CLK->CLKSTATUS & (0x01ul << 2)))
        ;
    //PLL configuration ends
    //clock source selection
    CLK->CLKSEL0 &= (~(0x07ul << 0));
    CLK->CLKSEL0 |= (0x02ul << 0);
    //clock frequency division
    CLK->CLKDIV &= (~0x0Ful << 0);
    //enable clock of SPI3
    CLK->APBCLK |= 1ul << 15;
    SYS_LockReg(); // Lock protected registers
}
void SPI3_Config(void){
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

//------------------------------------------------------------------------------------------------------------------------------------
// Game related definition
//------------------------------------------------------------------------------------------------------------------------------------

/**
 * These are the variables used by the game
 */
typedef enum{
    welcome_screen,
    game_rules,
    game_background,
    main_game,
    end_game
} STATES;


STATES game_state;
int i, key_pressed = 0, game_pad = 0;
int snake_head_x_coor = 0, snake_head_y_coor = 0, dx = 0, dy = 0;
int food_x_coor, food_y_coor;
int snake_step = 4; // Determine how many pixels the snake moves in one step 
int x_reset, y_reset;
int hit;
char score_txt[] = "0";
int snake_width = 3; // Snake and food block width is defined by the size of 3 + 1 = 4 pixels


/**
* This function uses rand() to generate food on the map
*/
void generate_food(){
    // Generate random food coordinates 
    // rand() in range: rand() % (max_number + 1 - minimum_number) + minimum_number
    int max_x = (Xmax - 1) - (snake_width - 1);
    int min_x = Xmin + 1;
    int max_y = (Ymax - 1) - (snake_width - 1);
    int min_y = Ymin + 1;

    food_x_coor = rand() % (max_x + 1 - min_x) + min_x;
    food_y_coor = rand() % (max_y + 1 - min_y) + min_y;

    // Generate food with those coordinates
    fill_Rectangle(food_x_coor, food_y_coor, food_x_coor + snake_width, food_y_coor + snake_width, 1, 0);
}


/**
 * This is the final main game control function
 */
void control_game(){
    //draw objects
    fill_Rectangle(snake_head_x_coor, snake_head_y_coor, snake_head_x_coor + snake_width, snake_head_y_coor + snake_width, 1, 0);
    //delay for vision
    while (game_pad == 0)
        game_pad = KeyPadScanning();
    CLK_SysTickDelay(170000);


    //erase the objects
    fill_Rectangle(snake_head_x_coor, snake_head_y_coor, snake_head_x_coor + snake_width, snake_head_y_coor + snake_width, 0, 0);


    //check changes
    switch (game_pad){
        case 2:
            dx = 0;
            dy = -1;
            break;
        case 4:
            dx = -1;
            dy = 0;
            break;
        case 6:
            dx = +1;
            dy = 0;
            break;
        case 5:
            dx = 0;
            dy = +1;
            break;
        default:
            dx = dy = 0;
            break;
    }   
    game_pad = 0;


    //update objects information (position, etc.)
    snake_head_x_coor = snake_head_x_coor + (dx * snake_step);
    snake_head_y_coor = snake_head_y_coor + (dy * snake_step);
    //boundary condition
    //wrap around on X
    if (snake_head_x_coor < Xmin)
        snake_head_x_coor = Xmax - snake_width + 1;
    if (snake_head_x_coor > Xmax - snake_width + 1)
        snake_head_x_coor = Xmin;
    //wrap around on Y
    if (snake_head_y_coor < Ymin)
        snake_head_y_coor = Ymax - snake_width + 1;
    if (snake_head_y_coor > Ymax - 7)
        snake_head_y_coor = Ymin;
    //score conditions
    
    if (hit > 5){
        hit = 0;
        LCD_clear();
        game_state = end_game;
    }
    else
        game_state = game_background;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------------------------------------------------------------
int main(void){
    game_state = welcome_screen; // The initial state of the game
    x_reset = 64;
    y_reset = 32;
    hit = 0;
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
    //LCD static content for testing and debugging 
    //--------------------------------
    draw_Rectangle(Xmin, Ymin, Xmax, Ymax, 1, 0); // Draw the playable field boundary 
    while (1){
        //erase the objects
        fill_Rectangle(food_x_coor, food_y_coor, food_x_coor + snake_width, food_y_coor + snake_width, 0, 0);
        generate_food();
        draw_Rectangle(Xmin, Ymin, Xmax, Ymax, 1, 0); // Draw the playable field boundary
        for (i = 0; i < 2; i++){
            CLK_SysTickDelay(SYSTICK_DLAY_us);
        }
    }

    //--------------------------------
    //LCD dynamic content
    //--------------------------------
    // while (1){
    //     switch (game_state){
    //         case welcome_screen:
    //             //welcome state code here
    //             draw_LCD(snake_game_intro);
    //             for (i = 0; i < 5; i++){
    //                 CLK_SysTickDelay(SYSTICK_DLAY_us);
    //             }
    //             LCD_clear();
    //             game_state = game_rules; // state transition
    //             break;
    //         case game_rules:
    //             // game_rules state code here
    //             printS_5x7(1, 0, "Use Keypad to control");
    //             printS_5x7(1, 8, "2: UP 4: LEFT");
    //             printS_5x7(1, 16, "6: RIGHT 8: DOWN");
    //             printS_5x7(1, 32, "Eat all boxes to win");
    //             printS_5x7(1, 56, "press any key to continue!");
    //             while (key_pressed == 0)
    //                 key_pressed = KeyPadScanning();
    //             key_pressed = 0;
    //             LCD_clear();
    //             game_state = game_background;
    //             break;
    //         case game_background:
    //             // static display information should be here
    //             sprintf(score_txt, "%d", hit);
    //             printS_5x7(5, 0, "Score: ");
    //             printS_5x7(48, 0, score_txt);
    //             draw_Rectangle(Xmin, Ymin, Xmax, Ymax, 1, 0); // Draw the playable field boundary 
    //             game_state = main_game;
    //         case main_game: 
    //             /*
    //                 The main control of the game
    //                 is now defined in the function below
    //                 for easier development, testing and implementation
    //             */  
    //             control_game(); 
    //             break;
    //         case end_game:
    //             //end_game code here
    //             //printS_5x7(1, 32, "press any key to replay!");
    //             draw_LCD(gameover_128x64);
    //             for (i = 0; i < 2; i++)
    //                 CLK_SysTickDelay(SYSTICK_DLAY_us);
    //             while (key_pressed == 0)
    //                 key_pressed = KeyPadScanning();
    //             key_pressed = 0;
    //             LCD_clear();
    //             game_state = welcome_screen;
    //             break;
    //         default:
    //             break;
    //     }
    // }
}
