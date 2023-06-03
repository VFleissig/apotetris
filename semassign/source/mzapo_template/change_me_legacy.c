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

  lightup(mem_base);

  printf("Hello world\n");

  sleep(4);

  printf("Goodbye world\n");

  /* Release the lock */
  serialize_unlock();

  return 0;
}
