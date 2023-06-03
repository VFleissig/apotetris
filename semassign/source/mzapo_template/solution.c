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
    
    parlcd_hx8357_init(parlcd_mem_base);

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
    loop_delay.tv_nsec = 200 * 1000 * 1000;
    /*for (k=0; k<60; k++) {

        parlcd_write_cmd(parlcd_mem_base, 44);
        for (i = 0; i < 320 ; i++) {//320
            for (j = 0; j < 480 ; j++) {//480
                //c = (((i+k) & 0x00) << 11) | ((j+k) & 0x00);//1f
                c = 0x000000;
                parlcd_write_data(parlcd_mem_base, c);
            }
        }


        clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }*/

    
    for (k=0; k<60; k++) {
            parlcd_write_cmd(parlcd_mem_base, 44);
            for (i = 0; i < 320 ; i++) {//320
            for (j = 0; j < 480 ; j++) {//480
                //c = (((i+k) & 0x00) << 11) | ((j+k) & 0x00);//1f
                c = 0x000000;
                parlcd_write_data(parlcd_mem_base, c);
            }
        }

    clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }
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
        if(game_started){
            start_game(mem_base);
        }
        else{
            show_mainMenu(mem_base);
        }
    }
    return 0;
}

int start_game(unsigned char *mem_base){


}


int main(int argc, char *argv[])
{
  unsigned char *mem_base;

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

  start_app(mem_base);

  //lightup(mem_base);

  printf("Hello world\n");

  sleep(4);

  printf("Goodbye world\n");

  /* Release the lock */
  serialize_unlock();

  return 0;
}
