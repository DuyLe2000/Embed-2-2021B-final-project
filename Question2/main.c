//------------------------------------------- main.c CODE STARTS ---------------------------------------------------------------------------
#include <stdio.h>
#include "NUC100Series.h"
#include "MCU_Init.h"
#include "SYS_init.h"
#include "LCD.h"
#include "Draw2D.h"

#include "picture.h"

#define SYSTICK_DLAY_us 1000000
#define FALLING_DLAY_us 800000
#define CLK_CHOICE 12 // 12 or 50 MHz

#define SCREEN_X_MAX 127
#define SCREEN_Y_MAX 63
#define SCREEN_X_MIN 0
#define SCREEN_Y_MIN 8

// Timer CMPR value
#define DEBOUNCE_DELAY 2399999 // 200 ms
#define FALL_DELAY 7199999 // 600ms

// Game related definition
#define MAX_BOX_AMOUNT 3
#define BOX_START_Y 9
#define BOX_START_X 20
#define BOX_WIDTH 2 // Actual width is 3
#define BOX_MOVE_RATE 2
#define MIN_BOX_START_X 1
#define MAX_BOX_START_X 124
#define MIN_BOX_START_Y 9
#define MAX_BOX_START_Y 9

#define PLAYER_HEIGHT 1 // Actual height is 2
#define PLAYER_WIDTH 5 // Actual width is 6
#define PLAYER_INITIAL_X 1
#define PLAYER_INITIAL_Y 60 // 59
#define PLAYER_STEP 6

typedef enum {
    welcome_screen,
    game_rules,
    game_initialize,
    main_game,
    pause,
    end_game
} STATES;

typedef struct coordinate_s {
    int x;
    int y;
} coordinate;

typedef struct player_info_s {
    int x;
    int y;
    int dx;
    int dy;
    int step;
} player_info;

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
void Timer_Config(void);

void draw_player(void);
void erase_player(void);
void draw_box(coordinate current_box);
void erase_box(coordinate current_box);
void update_score(void);
void collision_detection(void);
void draw_playing_field(void);
void box_controller(void);
void progress_controller(void);
void control_game(void);
void pause_game(void);

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
    /*
        Add debounce. Each key can only be pressed after 600 ms
    */
    PA0 = 1; PA1 = 1; PA2 = 0; PA3 = 1; PA4 = 1; PA5 = 1;
    if (!(TIMER0->TCSR & (1 << 30))) {
        if (PA3 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 1;
        }
        if (PA4 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 4;
        }
        if (PA5 == 0){
            TIMER0->TCSR |= (1 << 30);
            return 7;
        }
    }
    PA0 = 1; PA1 = 0; PA2 = 1; PA3 = 1; PA4 = 1; PA5 = 1;
    if (!(TIMER0->TCSR & (1 << 30))) {
        if (PA3 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 2;
        }
        if (PA4 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 5;
        }
        if (PA5 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 8;
        }

    }
    PA0 = 0; PA1 = 1; PA2 = 1; PA3 = 1; PA4 = 1; PA5 = 1;
    if (!(TIMER0->TCSR & (1 << 30))) {
        if (PA3 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 3;
        }
        if (PA4 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 6;
        }
        if (PA5 == 0) {
            TIMER0->TCSR |= (1 << 30);
            return 9;
        }
    }
    return 0;
}
void System_Config(void){
    SYS_UnlockReg(); // Unlock protected registers
    // Enable clock sources
    //LXT - External Low frequency Crystal 32 kHz
    CLK->PWRCON |= (0x01ul << 1);
    while (!(CLK->CLKSTATUS & (1ul << 1)));
    CLK->PWRCON |= (0x01ul << 0);
    while (!(CLK->CLKSTATUS & (1ul << 0)));
    CLK->CLKSEL0 &= (~(0x07ul << 0));

#if CLK_CHOICE == 50
    //PLL configuration
    CLK->PLLCON &= ~(1ul << 19); //0: PLL input is HXT
    CLK->PLLCON &= ~(1ul << 16); //PLL in normal mode
    CLK->PLLCON &= (~(0x01FFul << 0));
    CLK->PLLCON |= 48;
    CLK->PLLCON &= ~(1ul << 18); //0: enable PLLOUT
    while (!(CLK->CLKSTATUS & (0x01ul << 2)));
    CLK->CLKSEL0 |= (0x02ul << 0);
#else
    CLK->CLKSEL0 &= ~(0x03ul << 0);
#endif

    //clock frequency division
    CLK->CLKDIV &= (~0x0Ful << 0);
    //enable clock of SPI3
    CLK->APBCLK |= 1ul << 15;

    // Set 12Mhz clock for timer 0
    CLK->APBCLK |= (1 << 2);
    CLK->CLKSEL1 &= ~(0b111 << 8);

    // Set 12Mhz clock for timer 1
    CLK->APBCLK |= (1 << 3);
    CLK->CLKSEL1 &= ~(0b111 << 12);
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
void Timer_Config(void) {
    // Config timer for debounce - TIMER0 in one shot mode
    TIMER0->TCSR |= (1 << 26); // Reset timer
    TIMER0->TCSR &= ~(3ul << 27); // One shot mode
    TIMER0->TCSR &= ~(0xFFu << 0); // Set prescale to 0
    TIMER0->TCSR &= ~(1u << 24);   
    TIMER0->TCSR |= (1 << 16);
    TIMER0->TCMPR = DEBOUNCE_DELAY;

    // Config timer for falling box - TIMER1 in periodic
    TIMER1->TCSR |= (1 << 26); // Reset timer
    TIMER1->TCSR &= ~(3ul << 27); // Oneshot mode
    TIMER1->TCSR &= ~(0xFFu << 0); // Set prescale to 0
    TIMER1->TCSR &= ~(1u << 24);
    TIMER1->TCSR |= (1 << 16);
    TIMER1->TCMPR = FALL_DELAY;
}

//------------------------------------------------------------------------------------------------------------------------------------
// Game related definition
//------------------------------------------------------------------------------------------------------------------------------------

/**
 * These are the variables used by the game
 */
static coordinate box[MAX_BOX_AMOUNT];
static int box_count = 1;

static coordinate dead_box;
static int dead_box_spawn = 0;

static player_info player;

static STATES game_state;
static int key_pressed = 0, game_pad = 0;

static int score;
static char score_txt[] = "0";
static int high_score;
static char high_score_txt[] = "0";

void draw_player() {
    /**
     * Draw player character at their current position
    */
    fill_Rectangle(player.x, player.y, player.x + PLAYER_WIDTH, player.y + PLAYER_HEIGHT, 1, 0);
}

void erase_player() {
    /**
     * Erase player character at their current position
    */
    fill_Rectangle(player.x, player.y, player.x + PLAYER_WIDTH, player.y + PLAYER_HEIGHT, 0, 0);
}

void draw_box(coordinate current_box) {
    /**
     * Draw box inside the playing field
    */
    if (current_box.y > SCREEN_Y_MIN) {
        fill_Rectangle(current_box.x, current_box.y, current_box.x + BOX_WIDTH, current_box.y + BOX_WIDTH, 1, 0);
    }
}

void erase_box(coordinate current_box) {
    /**
     * Erase box inside the playing field
    */
    if (current_box.y > SCREEN_Y_MIN) {
        fill_Rectangle(current_box.x, current_box.y, current_box.x + BOX_WIDTH, current_box.y + BOX_WIDTH, 0, 0);
    }
}

void draw_dead_box(coordinate current_box) {
    /**
     * Draw dead box inside the playing field
    */
    if (current_box.y > SCREEN_Y_MIN) {
        draw_Rectangle(current_box.x, current_box.y, current_box.x + BOX_WIDTH, current_box.y + BOX_WIDTH, 1, 0);
    }
}

void update_score() {
    /**
     * Update player score when box collide with player
    */
    score++;
    sprintf(score_txt, "%d", score);
    printS_5x7(5, 0, "Score: ");
    printS_5x7(48, 0, score_txt);
}

void collision_detection() {
    /**
     * Detect collision between box and player
    */
    for (int i = 0; i < box_count; i++) {
        if (box[i].x + BOX_WIDTH >= player.x && box[i].x <= player.x + PLAYER_WIDTH 
                && box[i].y < player.y + PLAYER_HEIGHT && box[i].y + BOX_WIDTH >= player.y) {
            erase_box(box[i]);
            box[i].x = rand() % (MAX_BOX_START_X + 1 - MIN_BOX_START_X) + MIN_BOX_START_X;
            box[i].y = rand() % (MIN_BOX_START_Y + 1 - MAX_BOX_START_Y) + MIN_BOX_START_Y;
            draw_box(box[i]);
            update_score();
        }
    }

    if (dead_box_spawn) {
        if (dead_box.x + BOX_WIDTH >= player.x && dead_box.x <= player.x + PLAYER_WIDTH 
                && dead_box.y < player.y + PLAYER_HEIGHT && dead_box.y + BOX_WIDTH >= player.y) {
            game_state = end_game;
            return;
        }
    }
}

void draw_playing_field() {
    /**
     * Draw the border of the playing field
    */
    draw_Rectangle(SCREEN_X_MIN, SCREEN_Y_MIN, SCREEN_X_MAX, SCREEN_Y_MAX, 1, 0); // Draw the playable field boundary 
}

void box_controller() {
    /**
     * Control box movement and generation
    */
    for (int i = 0; i < box_count; i++) {
        // Remove the old falling food at old location and render a new one at new location
        erase_box(box[i]);
    
        box[i].y += BOX_MOVE_RATE; // Update food coordinates

        // Condition to stop falling when reached the bottom
        if ((box[i].y + BOX_WIDTH) >= (SCREEN_Y_MAX - 1)) {
            game_state = end_game;
            return;
        }
        draw_box(box[i]);
    }

    if (dead_box_spawn == 1) {
        erase_box(dead_box);
    
        dead_box.y += BOX_MOVE_RATE; // Update food coordinates

        // Condition to stop falling when reached the bottom
        if ((dead_box.y + BOX_WIDTH) >= (SCREEN_Y_MAX - 1)) {
            dead_box.x = rand() % (MAX_BOX_START_X + 1 - MIN_BOX_START_X) + MIN_BOX_START_X;
            dead_box.y = rand() % (MIN_BOX_START_Y + 1 - MAX_BOX_START_Y) + MIN_BOX_START_Y;
        }
        draw_dead_box(dead_box);
    }

    collision_detection();
}

void progress_controller() {
    if (score == 2 && box_count == 1) {
        box_count++;
        box[1].x = rand() % (MAX_BOX_START_X + 1 - MIN_BOX_START_X) + MIN_BOX_START_X;
        box[1].y = rand() % (MIN_BOX_START_Y + 1 - MAX_BOX_START_Y) + MIN_BOX_START_Y - 28;
    } else if (score == 5 && box_count == 2) {
        box_count++;
        box[2].x = rand() % (MAX_BOX_START_X + 1 - MIN_BOX_START_X) + MIN_BOX_START_X;
        box[2].y = rand() % (MIN_BOX_START_Y + 1 - MAX_BOX_START_Y) + MIN_BOX_START_Y - 14;
    }

    if (score == 2 && dead_box_spawn == 0) {
        dead_box_spawn = 1;
        dead_box.x = rand() % (MAX_BOX_START_X + 1 - MIN_BOX_START_X) + MIN_BOX_START_X;
        dead_box.y = rand() % (MIN_BOX_START_Y + 1 - MAX_BOX_START_Y) + MIN_BOX_START_Y - 14;
    }
}

void control_game() {
    /**
     * Game controller
    */
    progress_controller();
    draw_playing_field();
    //fill_Rectangle(SCREEN_X_MIN + 1, SCREEN_Y_MIN + 1, SCREEN_X_MAX - 1, SCREEN_Y_MAX - 1, 0, 0);
    draw_player();
    game_pad = KeyPadScanning();
    if (game_pad == 4 || game_pad == 6) {
        // Erase player character upon movement
        erase_player();

        // Check direction changes
        switch (game_pad) {
            case 4:
                player.dx = -1;
                player.dy = 0;
                break;
            case 6:
                player.dx = +1;
                player.dy = 0;
                break;
            default:
                player.dx = player.dy = 0;
                break;
        }

        // Update objects information (position, etc.)
        player.x += player.dx * player.step;

        // Boundary condition for edge prevention
        // Stops at X edge
        if (player.x < SCREEN_X_MIN + 1) {
            player.x = SCREEN_X_MIN + 1;
        } else if (player.x >= SCREEN_X_MAX - 1 - PLAYER_WIDTH) { 
            player.x = SCREEN_X_MAX - 1 - PLAYER_WIDTH;
        }
        draw_player();
    } else if (game_pad == 1) {
        TIMER1->TCSR &= ~(1 << 30);
        game_state = pause;
        return;
    }

    if (!(TIMER1->TCSR & (1 << 30))) {
        box_controller();
        TIMER1->TCSR |= (1 << 30);
    }
}

void pause_game() {
    printS_5x7(90, 0, "Paused!");
    game_pad = KeyPadScanning();
    while (game_pad != 1) {
        game_pad = KeyPadScanning();
    }
    printS_5x7(90, 0, "       ");
    game_state = main_game;
    TIMER1->TCSR |= (1 << 30);
}

//------------------------------------------------------------------------------------------------------------------------------------
// Main program
//------------------------------------------------------------------------------------------------------------------------------------
int main(void) {
    game_state = welcome_screen; // The initial state of the game
    score = 0;
    high_score = 0;

    //--------------------------------
    //System initialization
    //--------------------------------
    System_Config();
    KeyPadEnable();
    Timer_Config();
    //--------------------------------
    //SPI3 initialization
    //--------------------------------
    SPI3_Config();
    //--------------------------------
    //LCD initialization
    //--------------------------------
    LCD_start();
    LCD_clear();

    while (1){
        switch (game_state){
            case welcome_screen:
                //welcome state code here
                draw_LCD(sky_fall_logo);
                for (int i = 0; i < 3; i++){
                    CLK_SysTickDelay(SYSTICK_DLAY_us);
                }
                LCD_clear();
                game_state = game_rules; // state transition
                break;
            case game_rules:
                // game_rules state code here
                printS_5x7(1, 0, "Use Keypad to control");
                printS_5x7(1, 8, "4: LEFT 6: RIGHT");
                printS_5x7(1, 16, "1: PAUSE/UNPAUSE");
                printS_5x7(1, 32, "Catch as many box as");
                printS_5x7(1, 40, "possible");
                printS_5x7(1, 56, "press any key to continue!");
                while (key_pressed == 0)
                    key_pressed = KeyPadScanning();
                key_pressed = 0;
                LCD_clear();
                game_state = game_initialize;
                break;
            case game_initialize:
                // Game initialization
                // Game state initialization
                // Drawing static content out side of playing area
                score = 0;
                clear_LCD();
                sprintf(score_txt, "%d", score);
                printS_5x7(5, 0, "Score: ");
                printS_5x7(48, 0, score_txt);

                player.x = PLAYER_INITIAL_X;
                player.y = PLAYER_INITIAL_Y;
                player.dx = 0;
                player.dy = 0;
                player.step = PLAYER_STEP;
                box_count = 1;
                dead_box_spawn = 0;
                for (int i = 0; i < MAX_BOX_AMOUNT; i++) {
                    box[i].y = BOX_START_Y;
                    box[i].x = BOX_START_X;
                }
                draw_playing_field();

                TIMER1->TCSR |= (1 << 26);
                TIMER1->TCSR |= (1 << 30);
                game_state = main_game;
                break;
            case main_game: 
                control_game();
                break;
            case pause:
                pause_game();                
                break;
            case end_game:
                //end_game code here
                //printS_5x7(1, 32, "press any key to replay!");
                draw_LCD(gameover_128x64);
                for (int i = 0; i < 2; i++) {
                    CLK_SysTickDelay(SYSTICK_DLAY_us);
                }
                
                clear_LCD();
                if (score > high_score) {
                    high_score = score;
                    printS_5x7(12, 40, "New high score!");
                }
                sprintf(high_score_txt, "%d", high_score);
                sprintf(score_txt, "%d", score);
                printS_5x7(10, 16, "High Score: ");
                printS_5x7(80, 16, high_score_txt);
                printS_5x7(10, 24, "Score: ");
                printS_5x7(53, 24, score_txt);
                printS_5x7(1, 56, "press any key to continue!");

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
