/*******************************************************************
  Project main function template for MicroZed based MZ_APO board
  designed by Vaclav Fleissig

  change_me.c      - main file

  include your name there and license for distribution.

 *******************************************************************/

#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>

#include <stdio.h>

#include <stdint.h>

#include <time.h>

#include <unistd.h>

#include <stdbool.h>

#include "mzapo_parlcd.h"

#include "mzapo_phys.h"

#include "mzapo_regs.h"

#include "serialize_lock.h"

#include "font_types.h"

#include "font_prop14x16.c"

#include "font_rom8x16.c"

#define SPILED_REG_BASE_PHYS 0x43c40000

#define SPILED_REG_LED_LINE_o 0x004

#define BOARD_WIDTH 11
#define BOARD_HEIGHT 11
#define EMPTY 0
#define FILLED 1
#define MOVING 2
#define MOVED 3
#define MOVINGANDMOVED 5

int char_comp;

unsigned char * mem_base;
unsigned char * parlcd_mem_base;
unsigned char * knob_mem_base;

font_descriptor_t * font_instance = & font_winFreeSystem14x16;

unsigned int menu[320 * 320];
char selected = 0;

void parse_pixel(int x, int y, int color) {
    if (x >= 0 && x < 320 && y >= 0 && y < 320) {
        menu[320 * y + x] = color;
    }
}

// - function -----------------------------------------------------------------
int parse_char(int x, int y, int color, char c, int scale_factor) {
    if (c < font_instance -> firstchar)
        return;
    c -= font_instance -> firstchar;

    int char_def;

    int position = c * font_instance -> height;
    int char_width = font_instance -> width[c];

    for (int i = 0; i < (font_instance -> height * scale_factor); i += scale_factor) {
        char_comp = 32768;
        char_def = font_instance -> bits[position++];

        for (int j = 0; j < (char_width * scale_factor); j += scale_factor) {
            if (char_def & char_comp) {
                for (int k = 0; k < scale_factor; k++) {
                    for (int l = 0; l < scale_factor; l++) {
                        parse_pixel(x + j + l, y + i + k, color);
                    }
                }

            } else {
                for (int k = 0; k < scale_factor; k++) {
                    for (int l = 0; l < scale_factor; l++) {
                        parse_pixel(x + j + l, y + i + k, 0xffffff);
                    }
                }
            }

            char_comp = (char_comp >> 1);
        }
    }
    return char_width * scale_factor;
}

void parse_string(int x, int y, unsigned int color, char * c, int size) {
    int width = 0;
    int n = 0;
    char currc = c[n];
    int posx = x;
    while (currc != '\0') {
        width = parse_char(x, y, color, currc, size);
        x += width;
        n += 1;
        currc = c[n];
    }
}

//game started variable
bool game_started = false;


int board[BOARD_HEIGHT][BOARD_WIDTH];

//position of the tetris block
int blockX, blockY;

unsigned int score = 0;

//posun na led baru, legacy funkce
int lightup(unsigned char * mem_base) {
    uint32_t val_line = 7;
    for (int i = 0; i < 30; i++) {
        //uint32_t val_line=0;
        *(volatile uint32_t * )(mem_base + SPILED_REG_LED_LINE_o) = val_line;
        val_line <<= 1;
        printf("LED val 0x%x\n", val_line);
        sleep(1);
    }
}

void increment_score(unsigned char * mem_base) {
    score = (score << 1) + 1;
    *(volatile uint32_t * )(mem_base + SPILED_REG_LED_LINE_o) = score;
    printf("led score:", score);
    if (score >= 2 /*4294967295*/ ) {
        printf("You won!\n");
        exit(0);
    }
}

void init_score() {
    score = 0;
    *(volatile uint32_t * )(mem_base + SPILED_REG_LED_LINE_o) = score;
    printf("led score:", score);

}

///////TETRIS FUNCTIONS

int LBlock[3][3] = {
    {
        2,
        0,
        0
    },
    {
        2,
        0,
        0
    },
    {
        2,
        2,
        0
    }
};
int TBlock[3][3] = {
    {
        0,
        2,
        0
    },
    {
        2,
        2,
        2
    },
    {
        0,
        0,
        0
    }
};
int IBlock[3][3] = {
    {
        0,
        2,
        0
    },
    {
        0,
        2,
        0
    },
    {
        0,
        2,
        0
    }
};

int currentBlock[3][3];

//function to draw the board in terminal
void drawBoard() {
    int i, j;
    for (i = 0; i < BOARD_HEIGHT; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == EMPTY) {
                printf(".");
            } else if (board[i][j] == MOVING) {
                printf("0");
            } else if (board[i][j] == MOVED) {
                printf("O");
            } else if (board[i][j] == MOVINGANDMOVED) {
                printf("U");
            } else {
                printf("#");
            }
        }
        printf("\n");
    }
    printf("gamestarted: %d \n", game_started);
}

//function to initialize the board
void initializeBoard() {
    int i, j;
    for (i = 0; i < BOARD_HEIGHT; i++) {
        for (j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = EMPTY;
        }
    }
}

//function to spawn a new block
void spawnBlock() {
    blockX = (BOARD_WIDTH / 2) - 1;
    blockY = 0;
    int shape = rand() % 3;

    //copy the chosen shape to the currentBlock array
    int i, j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            switch (shape) {
            case 0:
                currentBlock[i][j] = LBlock[i][j];
                break;
            case 1:
                currentBlock[i][j] = TBlock[i][j];
                break;
            case 2:
                currentBlock[i][j] = IBlock[i][j];
                break;
            }
        }
    }

    //check if the new block can be placed, if not, game over
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (currentBlock[i][j] == MOVING && board[blockY + i][blockX + j] != EMPTY) {
                printf("\nblock[%d][%d] caused gameover\n", blockY + i, blockX + j);
                printf("Game Over\n");
                //free(menu);
                exit(0);
            }
        }
    }

    //place the new block on the board
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (currentBlock[i][j] == MOVING) {
                board[blockY + i][blockX + j] = MOVING;
            }
        }
    }
}

void transform_moved() {
    printf("\nboard before redraw: \n");
    drawBoard();
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == MOVED)
                board[i][j] = MOVING;
        }
    }
}

void transform_moving_to_filled() {
    printf("\nboard before redraw: \n");
    drawBoard();
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == MOVING)
                board[i][j] = FILLED;
        }
    }
}

bool is_movable() {
    int any_moving_blocks_present = 0;
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == MOVING) {
                any_moving_blocks_present = 1;
                if ((i >= BOARD_HEIGHT - 1) || (board[i + 1][j] != EMPTY && board[i + 1][j] != MOVING)) {
                    return false;
                } else {
                }
            }

        }
    }
    if (any_moving_blocks_present)
        return true;
    return false;
}

void move_blocks() {
    printf("getsheere");
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
            if (board[i][j] == MOVING || board[i][j] == MOVINGANDMOVED) {

                if (board[i + 1][j] == EMPTY) {
                    printf("\nemptyfilledwithmoved\n");
                    board[i + 1][j] = MOVED;
                } else if (board[i + 1][j] == MOVING) {
                    board[i + 1][j] = MOVINGANDMOVED;
                }

                if (board[i][j] == MOVING)
                    board[i][j] = EMPTY;
                else if (board[i][j] == MOVINGANDMOVED)
                    board[i][j] = MOVED;

            }
        }
    }
    transform_moved();
}

bool piece_movable_right() {
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
            if (board[i][j] == MOVING) {
                if (j == 10 || board[i + 1][j + 1] == FILLED || board[i][j + 1] == FILLED) {
                    return false;
                }

            }
            //printf("i:%d j:%d\n", i, j);
            //drawBoard();
        }
    }
    return true;
}

bool piece_movable_left() {
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
            if (board[i][j] == MOVING) {
                if (j == 0 || board[i + 1][j + 1] == FILLED) {
                    return false;
                }

            }
            //printf("i:%d j:%d\n", i, j);
            //drawBoard();
        }
    }
    return true;
}

void move_piece_right() {
    if (piece_movable_right()) {
        for (int i = 0; i < 11; i++) {
            for (int j = 10; j > -1; j--) {
                if (board[i][j] == MOVING) {

                    board[i][j + 1] = MOVING;
                    board[i][j] = EMPTY;

                }
                //printf("i:%d j:%d\n", i, j);
                //drawBoard();
            }
        }
    }
}

void move_piece_left() {
    if (piece_movable_left()) {
        for (int i = 0; i < 11; i++) {
            for (int j = 0; j < 11; j++) {
                if (board[i][j] == MOVING) {

                    board[i][j - 1] = MOVING;
                    board[i][j] = EMPTY;

                }
                //printf("i:%d j:%d\n", i, j);
                //drawBoard();
            }
        }
    }
}

int check_for_filled_line() {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board[i][j] == EMPTY /*board[i][j]!=FILLED || board[i][j]!=MOVING*/ ) {
                printf("broken at board[%d][%d]", i, j);
                break;
            }
            if (j == 10) {
                return i;
            }
        }
    }
    return -1;
}

// Function to update the board (move the current block down)
void update_board(unsigned char * mem_base) {
    int linecheck = -1;
    printf("getshere");
    // If the block can move down, erase it from the current position
    bool movable = is_movable();
    printf("\nis movable? %d \n", movable);
    if (movable) {
        move_blocks();
    } else {
        // If the block can't move down, spawn a new one
        transform_moving_to_filled();

        while (true) {
            linecheck = check_for_filled_line();
            printf("\nlinecheck: %d\n", linecheck);
            if (linecheck == -1)
                break;
            for (int i = 0; i < BOARD_HEIGHT; i++) {
                for (int j = 0; j < BOARD_WIDTH; j++) {
                    if (i < linecheck && board[i][j] == FILLED) {
                        board[i][j] = MOVING;
                    } else if (i == linecheck) {
                        board[i][j] = EMPTY;
                    }
                }
            }
            increment_score(mem_base);
            move_blocks();
            transform_moving_to_filled();
        }
        sleep(1);

        spawnBlock();
    }

}

///////END TETRIS FUNCTIONS

/*
void drawPixel(int x, int y, unsigned int color) {
    parlcd_set_position(parlcd_mem_base, x, y);
    parlcd_write_data(parlcd_mem_base, color);
}*/

void clear_menu() {
    int c = 0xffffff;
    for (int i = 0; i < 320; i++) { //320
        for (int j = 0; j < 320; j++) { //480
            menu[(i * 320 + j)] = c;

        }
    }
}

void parse_controls() {

    int c = 0xffffff;
    clear_menu();
    parlcd_write_cmd(parlcd_mem_base, 44);
    for (int i = 0; i < 320; i++) { //320
        for (int j = 0; j < 320; j++) { //480
            c = 0xffffff;
            parlcd_write_data(parlcd_mem_base, menu[(i * 320 + j)]);

        }
    }

    parse_string(50, 30, 0x000000, "Controls\0", 3);
    parse_string(5, 130, 0x000000, "Use red knob to navigate main menu.\0", 1);
    parse_string(5, 160, 0x000000, "Use red knob press to select an option.\0", 1);
    parse_string(5, 190, 0x000000, "In game, use red knob to move the pieces.\0", 1);
    parse_string(5, 220, 0x000000, "In game, use red knob press to quit the game.\0", 1);

    parlcd_write_cmd(parlcd_mem_base, 44);
    for (int i = 0; i < 320; i++) { //320
        for (int j = 0; j < 320; j++) { //480
            c = 0xffffff;
            parlcd_write_data(parlcd_mem_base, menu[(i * 320 + j)]);

        }
    }

    int p = 0;

    sleep(10);
    clear_menu();
}

void parse_menu() {
    //unsigned char *mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);

    unsigned int c;

    int menu_colors[3] = {
        0x0000ff,
        0x000000,
        0x000000
    };
    int menu_size[3] = {
        2,
        2,
        2
    };

    c = 0xffffff;

    parlcd_write_cmd(parlcd_mem_base, 44);
    for (int i = 0; i < 320; i++) { //320
        for (int j = 0; j < 320; j++) { //480
            c = 0xffffff;
            parlcd_write_data(parlcd_mem_base, menu[(i * 320 + j)]);

        }
    }

    for (int i = 0; i < 3; i++) {
        if (selected == i) {
            menu_colors[i] = 0x0000ff;
        } else {
            menu_colors[i] = 0x000000;
        }
    }

    parse_string(50, 30, 0x000000, "Main Menu\0", 3);
    parse_string(140, 100, menu_colors[0], "Play\0", menu_size[0]);
    parse_string(110, 150, menu_colors[1], "Controls\0", menu_size[1]);
    parse_string(140, 200, menu_colors[2], "Quit\0", menu_size[2]);

    int p = 0;

    /* If mapping fails exit with error code */
    if (mem_base == NULL) {
        //free(menu);
        exit(1);
    }

}

int show_mainMenu() {
    knob_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
    int prev_knob_val = 0;
    int led_progress = 1;

    while (1) {
        parse_menu();
        led_progress += led_progress << 1;

        *(volatile uint32_t * )(knob_mem_base + SPILED_REG_LED_LINE_o) = led_progress;

        prev_knob_val = checkforinput_mainmenu(knob_mem_base, prev_knob_val);
        if (prev_knob_val > 0xffffff) {
            break;
            show_mainMenu();
            sleep(1);
        }

        game_started = 0;
        //sleep(1);
    }
}

int start_app() {
    while (true) {
        printf("startapp\n");
        if (game_started) {
            start_game();
        } else {
            show_mainMenu();
        }
    }
    return 0;
}

///input functions

void execute_menu() {
    if (selected == 0) {
        //game_started = 1;
        game_started = 1;
        //start_game();
    } else if (selected == 1) {
        parse_controls();
    } else {
        exit(0);
    }
}

int checkforinput_mainmenu(unsigned char * knob_mem_base, int prev_knob_val) {
    uint32_t rgb_knobs_value;
    int int_val;
    unsigned int uint_val;

    /* Initialize structure to 0 seconds and 200 milliseconds */
    struct timespec loop_delay = {
        .tv_sec = 0,
        .tv_nsec = 200 * 1000 * 1000
    };

    /*
     * Access register holding 8 bit relative knobs position
     * The type "(volatile uint32_t*)" casts address obtained
     * as a sum of base address and register offset to the
     * pointer type which target in memory type is 32-bit unsigned
     * integer. The "volatile" keyword ensures that compiler
     * cannot reuse previously read value of the location.
     */
    rgb_knobs_value = * (volatile uint32_t * )(knob_mem_base + SPILED_REG_KNOBS_8BIT_o);

    /*
     * Store RGB knobs values to the corersponding components controlling
     * a color/brightness of the RGB LEDs
     */
    *(volatile uint32_t * )(knob_mem_base + SPILED_REG_LED_RGB1_o) += 15;

    *(volatile uint32_t * )(knob_mem_base + SPILED_REG_LED_RGB2_o) += 15;

    /* Assign value read from knobs to the basic signed and unsigned types */
    int_val = rgb_knobs_value;
    uint_val = rgb_knobs_value;

    if ((int_val > 0xffffff)) {
        execute_menu();
        return int_val;
    }

    /* Print values */
    printf("int %10d uint 0x%08x\n", int_val, uint_val);

    /*
     * Wait for time specified by "loop_delay" variable.
     * Use monotonic clocks as time reference to ensure
     * that wait interval is not prolonged or shortened
     * due to real time adjustment.
     */
    //clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);

    //if(prev_knob_val + 174762 < int_val){
    for (int i = 0; i < (int_val - prev_knob_val + 302145 / 2) / 262144; i++) {
        selected -= 1;
        selected %= 3;
        printf("selected %d\n", selected);

    }
    //}
    //else if(prev_knob_val - 174762 > int_val){
    for (int i = 0; i < (prev_knob_val - int_val + 302145 / 2) / 262144; i++) {
        selected += 1;
        selected %= 3;
        printf("selected %d\n", selected);
    }
    prev_knob_val = int_val;
    //}
    return int_val;

}

int checkforinput(unsigned char * knob_mem_base, int prev_knob_val) {
    uint32_t rgb_knobs_value;
    int int_val;
    unsigned int uint_val;

    /* Initialize structure to 0 seconds and 200 milliseconds */
    struct timespec loop_delay = {
        .tv_sec = 0,
        .tv_nsec = 200 * 1000 * 1000
    };

    /*
     * Access register holding 8 bit relative knobs position
     * The type "(volatile uint32_t*)" casts address obtained
     * as a sum of base address and register offset to the
     * pointer type which target in memory type is 32-bit unsigned
     * integer. The "volatile" keyword ensures that compiler
     * cannot reuse previously read value of the location.
     */
    rgb_knobs_value = * (volatile uint32_t * )(knob_mem_base + SPILED_REG_KNOBS_8BIT_o);

    /* Store the read value to the register controlling individual LEDs */
    //*(volatile uint32_t*)(knob_mem_base + SPILED_REG_LED_LINE_o) = rgb_knobs_value;

    /*
     * Store RGB knobs values to the corersponding components controlling
     * a color/brightness of the RGB LEDs
     */
    //*(volatile uint32_t*)(knob_mem_base + SPILED_REG_LED_RGB1_o) = rgb_knobs_value;

    //*(volatile uint32_t*)(knob_mem_base + SPILED_REG_LED_RGB2_o) = rgb_knobs_value;

    /* Assign value read from knobs to the basic signed and unsigned types */
    int_val = rgb_knobs_value;
    uint_val = rgb_knobs_value;

    if ((int_val > 0xffffff)) {
        printf("Thanks for playing!\n");
        exit(0);
        //return prev_knob_val;
    }

    /* Print values */
    printf("int %10d uint 0x%08x\n", int_val, uint_val);

    /*
     * Wait for time specified by "loop_delay" variable.
     * Use monotonic clocks as time reference to ensure
     * that wait interval is not prolonged or shortened
     * due to real time adjustment.
     */
    //clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);

    printf("\nmovable left: %d \n movable right: %d \n", piece_movable_left, piece_movable_right);
    //if(prev_knob_val + 174762 < int_val){
    for (int i = 0; i < (int_val - prev_knob_val + 302145 / 2) / 262144; i++) {
        if (!piece_movable_left)
            break;
        move_piece_left();
    }
    //}
    //else if(prev_knob_val - 174762 > int_val){
    for (int i = 0; i < (prev_knob_val - int_val + 302145 / 2) / 262144; i++) {
        if (!piece_movable_right)

            break;
        move_piece_right();
    }
    prev_knob_val = int_val;
    //}
    return int_val;

}
//git change

int start_game() {
    int prev_knob_val = 0;

    int i, j, k;
    unsigned int c;

    struct timespec loop_delay = {
        .tv_sec = 0,
        .tv_nsec = 20 * 1000 * 1000
    };

    init_score();
    game_started = 1;
    printf("gamestarted insstart_game\n");
    sleep(1);
    //mapping fail handling
    if (mem_base == NULL) {
        //free(menu);
        exit(1);
    }

    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    knob_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);

    if (parlcd_mem_base == NULL) {
        //free(menu);
        exit(1);
    }

    initializeBoard();
    spawnBlock();

    loop_delay.tv_sec = 0;
    loop_delay.tv_nsec = 1000 * 1000 * 1000;

    int filled = 0;
    while (true) {

        parlcd_write_cmd(parlcd_mem_base, 44);
        for (i = 0; i < 320; i++) { //legacy 320
            for (j = 0; j < 320; j++) { //legacy 480

                if (i % 29 == 0 || j % 29 == 0)
                    c = 0x000000;
                else if (board[i / 29][j / 29] == EMPTY) {
                    c = 0xffffff;
                } else if (board[i / 29][j / 29] == MOVING) {
                    c = 0x0000f0;
                } else
                    c = 0x00ff00;
                parlcd_write_data(parlcd_mem_base, c);
            }
        }
        system("cls");

        prev_knob_val = checkforinput(knob_mem_base, prev_knob_val);
        drawBoard();
        update_board(mem_base);
        if (game_started == false)
            return 0;
        sleep(1);
    }

}

int main(int argc, char * argv[]) {
    int i;

    mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
    /* Serialize execution of applications */

    /* Try to acquire lock the first */
    if (serialize_lock(1) <= 0) {
        printf("System is occupied\n");

        //menu = malloc(sizeof(char)*320*320);

        if (1) {
            printf("Waiting\n");
            /* Wait till application holding lock releases it or exits */
            serialize_lock(0);
        }
    }

    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    parlcd_hx8357_init(parlcd_mem_base);

    unsigned int c;

    c = 0x000000;

    for (int i = 0; i < 320 * 320; i++) {
        menu[i] = 0xffffff;
    }

    start_app();

    //lightup(mem_base);

    printf("Hello world\n");

    sleep(4);

    printf("Goodbye world\n");

    /* Release the lock */
    serialize_unlock();

    return 0;
}