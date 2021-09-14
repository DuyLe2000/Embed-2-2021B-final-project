/*
    This is the seperated file used to control the main game 
    (main game state) 
*/

#include "game_controller.h"
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_Init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"

#define Xmax 128
#define Ymax 64
#define Xmin 1
#define Ymin 1

typedef enum{
    welcome_screen,
    game_rules,
    game_background,
    main_game,
    end_game
} STATES;


void control_game(){
    STATES game_state;
    uint8_t i, key_pressed = 0, game_pad = 0;
    uint8_t x = 64, y = 32, dx = 0, dy = 0;
    uint8_t x_steps, y_steps;
    uint8_t x_reset, y_reset;
    uint8_t hit;
    char score_txt[] = "0";

    game_state = welcome_screen;
    x_steps = 2;
    y_steps = 2;
    x_reset = 64;
    y_reset = 32;
    hit = 0;


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
    switch (game_pad){
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


    //update object�s information (position, etc.)
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
    if ((x - 8 < 6) && (y - 8 < 6)){
        hit++;
        x = x_reset;
        y = y_reset;
    }
    if (hit > 1){
        hit = 0;
        LCD_clear();
        game_state = end_game;
    }
    else
        game_state = game_background;
}