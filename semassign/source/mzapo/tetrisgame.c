#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BOARD_WIDTH 11
#define BOARD_HEIGHT 11
#define EMPTY 0
#define FILLED 1
#define MOVING 2

// Tetris board
int board[BOARD_HEIGHT][BOARD_WIDTH];

// Current block position
int blockX, blockY;

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
    blockX = BOARD_WIDTH / 2;
    blockY = 0;

    // Game over if the new block can't be placed
    if(board[blockY][blockX] != EMPTY) {
        printf("Game Over\n");
        exit(0);
    }

    board[blockY][blockX] = MOVING;
}

// Function to update the board (move the current block down)
void updateBoard() {
    // If the block can move down, erase it from the current position
    if(blockY < BOARD_HEIGHT - 1 && board[blockY + 1][blockX] == EMPTY) {
        board[blockY][blockX] = EMPTY;
        blockY++;
        board[blockY][blockX] = MOVING;
    } else {
        // If the block can't move down, spawn a new one
        board[blockY][blockX] = FILLED;
        spawnBlock();
    }
}

int main() {
    initializeBoard();
    spawnBlock();

    while(1) {
        system("cls");
        drawBoard();
        updateBoard();
        sleep(1); // Sleep for 1 second
    }

    return 0;
}
