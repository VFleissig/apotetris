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

#define SPILED_REG_BASE_PHYS 0x43c40000

#define SPILED_REG_LED_LINE_o 0x004 

#define BOARD_WIDTH 11
#define BOARD_HEIGHT 11
#define EMPTY 0
#define FILLED 1
#define MOVING 2
#define MOVED 3
#define MOVINGANDMOVED 5

//nastaveni led baru na fixni hodnotu
/*
int lightup(unsigned char *mem_base){
  for (int i=0; i<30; i++) {
      uint32_t val_line=2863311530;
     *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = val_line;
     val_line<<=1;
     printf("LED val 0x%x\n", val_line);
     sleep(1);
  }
}*/

// Tetris board
int board[BOARD_HEIGHT][BOARD_WIDTH];

// Current block position
int blockX, blockY;



///////TETRIS FUNCTIONS

int LBlock[3][3] = {{2, 0, 0}, {2, 0, 0}, {2, 2, 0}};
int TBlock[3][3] = {{0, 2, 0}, {2, 2, 2}, {0, 0, 0}};
int IBlock[3][3] = {{0, 2, 0}, {0, 2, 0}, {0, 2, 0}};

int currentBlock[3][3];

// Function to draw the board
void drawBoard() {
    int i, j;
    for(i = 0; i < BOARD_HEIGHT; i++) {
        for(j = 0; j < BOARD_WIDTH; j++) {
            if(board[i][j] == EMPTY) {
                printf(".");
            } else if(board[i][j] == MOVING){
                printf("0");
            }
            else if(board[i][j] == MOVED){
                printf("O");
            }
            else if(board[i][j] == MOVINGANDMOVED){
                printf("U");
            }
            else {
                printf("#");
            }
        }
        printf("\n");
    }
}

// Function to initialize the board
void initializeBoard() {
    int i, j;
    for(i = 0; i < BOARD_HEIGHT; i++) {
        for(j = 0; j < BOARD_WIDTH; j++) {
            board[i][j] = EMPTY;
        }
    }
}

// Function to spawn a new block
void spawnBlock() {
    blockX = (BOARD_WIDTH / 2) - 1;
    blockY = 0;
    int shape = rand() % 3;

    // Copy the chosen shape to the currentBlock array
    int i, j;
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            switch(shape) {
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

    // Check if the new block can be placed, if not, game over
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            if(currentBlock[i][j] == MOVING && board[blockY + i][blockX + j] != EMPTY) {
                printf("Game Over\n");
                exit(0);
            }
        }
    }

    // Place the new block on the board
    for(i = 0; i < 3; i++) {
        for(j = 0; j < 3; j++) {
            if(currentBlock[i][j] == MOVING) {
                board[blockY + i][blockX + j] = MOVING;
            }
        }
    }
}

void transformMOVED(){
    printf("\nboard before redraw: \n");
    drawBoard();
    for(int i = 0; i < BOARD_HEIGHT; i++){
        for(int j = 0; j < BOARD_WIDTH; j++){
            if(board[i][j] == MOVED)
                board[i][j] = MOVING;
        }
    }
}

void transformMOVINGtoFILLED(){
    printf("\nboard before redraw: \n");
    drawBoard();
    for(int i = 0; i < BOARD_HEIGHT; i++){
        for(int j = 0; j < BOARD_WIDTH; j++){
            if(board[i][j] == MOVING)
                board[i][j] = FILLED;
        }
    }
}

bool is_movable(){
    int any_moving_blocks_present = 0;
    for(int i = 0; i<BOARD_HEIGHT; i++){
        for(int j = 0; j < BOARD_WIDTH; j++){
            if(board[i][j]==MOVING){
                any_moving_blocks_present = 1;
                if((i >= BOARD_HEIGHT - 1) || (board[i + 1][j] != EMPTY && board[i + 1][j] != MOVING)) {
                    //printf("returned false\n");
                    return false;
                }
                else{
                    //printf("so far true\n");
                }
            }
            
        }
    }
    if(any_moving_blocks_present)
    //printf("returned true\n");
        return true;
    return false;
}

void move_blocks(){
    printf("getsheere");
    for(int i = 0; i<11; i++){
        for(int j = 0; j < 11; j++){
            if(board[i][j]==MOVING || board[i][j] == MOVINGANDMOVED) {

                if(board[i+1][j] == EMPTY){
                    printf("\nemptyfilledwithmoved\n");
                    board[i+1][j] = MOVED;
                }
                else if(board[i+1][j] == MOVING){
                    board[i+1][j] = MOVINGANDMOVED;
                }

                if(board[i][j]==MOVING)
                    board[i][j] = EMPTY;
                else if(board[i][j] == MOVINGANDMOVED)
                    board[i][j] = MOVED;

            }
            //printf("i:%d j:%d\n", i, j);
            //drawBoard();
        }   
    }
    transformMOVED();
}

bool piece_movable_right(){
    for(int i = 0; i<11; i++){
        for(int j = 0; j < 11; j++){
            if(board[i][j]==MOVING) {
                if(j == 10){
                    return false;
                }

            }
            //printf("i:%d j:%d\n", i, j);
            //drawBoard();
        }   
    }
    return true;
}

bool piece_movable_left(){
    for(int i = 0; i<11; i++){
        for(int j = 0; j < 11; j++){
            if(board[i][j]==MOVING) {
                if(j == 0){
                    return false;
                }

            }
            //printf("i:%d j:%d\n", i, j);
            //drawBoard();
        }   
    }
    return true;
}


void move_piece_right(){
    if(piece_movable_right()){
        for(int i = 0; i<11; i++){
            for(int j = 10; j > -1; j--){
                if(board[i][j]==MOVING) {

                    board[i][j+1] = MOVING;
                    board[i][j] = EMPTY;

                }
                //printf("i:%d j:%d\n", i, j);
                //drawBoard();
            }   
        }
    }
}


void move_piece_left(){
    if(piece_movable_left()){
        for(int i = 0; i<11; i++){
            for(int j = 0; j < 11; j++){
                if(board[i][j]==MOVING) {

                    board[i][j-1] = MOVING;
                    board[i][j] = EMPTY;

                }
                //printf("i:%d j:%d\n", i, j);
                //drawBoard();
            }   
        }
    }
}



// Function to update the board (move the current block down)
void updateBoard() {
    printf("getshere");
    // If the block can move down, erase it from the current position
        bool movable = is_movable();
        printf("\nis movable? %d \n", movable);
        if(movable) {
            move_blocks();
                //board[blockY][blockX] = EMPTY;
                //blockY++;
                //board[blockY][blockX] = MOVING;
        } else {
            // If the block can't move down, spawn a new one
            transformMOVINGtoFILLED();
            spawnBlock();
        }

}

///////END TETRIS FUNCTIONS

//posun na led baru
int lightup(unsigned char *mem_base){
  uint32_t val_line = 7;
  for (int i=0; i<30; i++) {
      //uint32_t val_line=0;
     *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = val_line;
     val_line<<=1;
     printf("LED val 0x%x\n", val_line);
     sleep(1);
  }
}

int print_font_at_coordinates(unsigned char *mem_base){
    unsigned char *parlcd_mem_base;

    //unsigned char *mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
    
    int i,j,k;
    unsigned int c;

    sleep(1);
    /* If mapping fails exit with error code */
    if (mem_base == NULL)
        exit(1);

    struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 20 * 1000 * 1000};
    /*for (i=0; i<30; i++) {
        *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = val_line;
        val_line<<=1;
        printf("LED val 0x%x\n", val_line);
        clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }*/

    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);

    if (parlcd_mem_base == NULL)
        exit(1);
    
    //parlcd_hx8357_init(parlcd_mem_base);

    /*parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (i = 0; i < 320 ; i++) {
        for (j = 0; j < 480 ; j++) {
            c = 0;
            parlcd_write_data(parlcd_mem_base, c);
        }
    }*/
/*
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (i = 0; i < 320 ; i++) {
        for (j = 0; j < 480 ; j++) {
            c = ((i & 0x1f) << 11) | (j & 0x1f);
            parlcd_write_data(parlcd_mem_base, c);
        }
    }*/

    loop_delay.tv_sec = 0;
    loop_delay.tv_nsec = 1000 * 1000 * 1000;
    /*for (k=0; k<60; k++) {

        parlcd_write_cmd(parlcd_mem_base, 44);
        for (i = 0; i < 320 ; i++) {//320
            for (j = 0; j < 300 ; j++) {//480
                //c = (((i+k) & 0x00) << 11) | ((j+k) & 0x00);//1f
                if(i%2)
                    c = 0xffffff;
                else
                    c = 0xffffff;// 0xffffff 0x000000
                parlcd_write_data(parlcd_mem_base, c);
                

            }
        }


        clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }*/
        
            parlcd_write_cmd(parlcd_mem_base, 44);
            for (i = 0; i < 480 ; i++) {//320
            for (j = 0; j < 320 ; j++) {//480
                //c = (((i+k) & 0x00) << 11) | ((j+k) & 0x00);//1f
                if(i%10 == 0 || j%10 == 0)
                    c= 0x000000;
                else
                    c = 0xffffff;
                parlcd_write_data(parlcd_mem_base, c);
            }
        }
/*
        parlcd_write_cmd(parlcd_mem_base, 44);
        i = 0;
        while(i < 320*480){
            c = 0x000000;
            parlcd_write_data(parlcd_mem_base, c);
            i++;
        }
*/


    
    /***for(int z = 0; z < PARLCD_REG_SIZE; z++){

        c = ((i & 0x00) << 11) | (j & 0x1f);
        parlcd_write_data(parlcd_mem_base, 0);
        
        clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }*/
}

int show_mainMenu(unsigned char *mem_base){
    print_font_at_coordinates(mem_base);
}



int start_app(unsigned char *mem_base){
    bool game_started = false;
    while(true){
        if(!game_started){
            start_game(mem_base);
        }
        else{
            show_mainMenu(mem_base);
        }
    }
    return 0;
}

///input functions

int checkforinput(unsigned char* knob_mem_base, int prev_knob_val){
     uint32_t rgb_knobs_value;
     int int_val;
     unsigned int uint_val;

     /* Initialize structure to 0 seconds and 200 milliseconds */
     struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 200 * 1000 * 1000};

     /*
      * Access register holding 8 bit relative knobs position
      * The type "(volatile uint32_t*)" casts address obtained
      * as a sum of base address and register offset to the
      * pointer type which target in memory type is 32-bit unsigned
      * integer. The "volatile" keyword ensures that compiler
      * cannot reuse previously read value of the location.
      */
     rgb_knobs_value = *(volatile uint32_t*)(knob_mem_base + SPILED_REG_KNOBS_8BIT_o);

     /* Store the read value to the register controlling individual LEDs */
     *(volatile uint32_t*)(knob_mem_base + SPILED_REG_LED_LINE_o) = rgb_knobs_value;

     /*
      * Store RGB knobs values to the corersponding components controlling
      * a color/brightness of the RGB LEDs
      */
     *(volatile uint32_t*)(knob_mem_base + SPILED_REG_LED_RGB1_o) = rgb_knobs_value;

     *(volatile uint32_t*)(knob_mem_base + SPILED_REG_LED_RGB2_o) = rgb_knobs_value;

     /* Assign value read from knobs to the basic signed and unsigned types */
     int_val = rgb_knobs_value;
     uint_val = rgb_knobs_value;

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
    for(int i = 0; i < (int_val-prev_knob_val)/262144; i++)
        move_piece_left();
     //}
     //else if(prev_knob_val - 174762 > int_val){
    for(int i = 0; i < (prev_knob_val-int_val)/262144; i++)
        move_piece_right();
     //}
     return int_val;

  }
  //git change


int start_game(unsigned char *mem_base){
    unsigned char *parlcd_mem_base;
    unsigned char* knob_mem_base;
    int prev_knob_val = 0;
    
    int i,j,k;
    unsigned int c;

    sleep(1);
    //mapping fail handling
    if (mem_base == NULL)
        exit(1);

    struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 20 * 1000 * 1000};

    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    knob_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);


    if (parlcd_mem_base == NULL)
        exit(1);

    initializeBoard();
    spawnBlock();

    loop_delay.tv_sec = 0;
    loop_delay.tv_nsec = 1000 * 1000 * 1000;

    int filled = 0;
    while(true){
        
            parlcd_write_cmd(parlcd_mem_base, 44);
            for (i = 0; i < 320 ; i++) {//legacy 320
            for (j = 0; j < 320 ; j++) {//legacy 480
                
                if(i%29 == 0 || j%29 == 0)
                    c= 0x000000;
                else if(board[i/29][j/29] == EMPTY){
                        c = 0xffffff;
                    }
                else if(board[i/29][j/29] == MOVING){
                    c = 0x0000f0;
                }
                else
                    c = 0x00ff00;
                parlcd_write_data(parlcd_mem_base, c);
            }
        }
        system("cls");

        prev_knob_val = checkforinput(knob_mem_base, prev_knob_val);
        drawBoard();
        updateBoard();
        sleep(1);
    }

}


int main(int argc, char *argv[])
{
  unsigned char *mem_base;
  unsigned char *parlcd_mem_base;

  int i;

  mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
  /* Serialize execution of applications */

  /* Try to acquire lock the first */
  if (serialize_lock(1) <= 0) {
    printf("System is occupied\n");

    if (1) {
      printf("Waitting\n");
      /* Wait till application holding lock releases it or exits */
      serialize_lock(0);
    }
  }


    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
  parlcd_hx8357_init(parlcd_mem_base);

  start_app(mem_base);

  //lightup(mem_base);

  printf("Hello world\n");

  sleep(4);

  printf("Goodbye world\n");

  /* Release the lock */
  serialize_unlock();

  return 0;
}
