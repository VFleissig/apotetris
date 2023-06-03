/*******************************************************************
  Project main function template for MicroZed based MZ_APO board
  designed by Petr Porazil at PiKRON

  Jan Kohout and Martin Bílík 2020

 *******************************************************************/

#define _POSIX_C_SOURCE 200112L

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <string.h>

#include <pthread.h>

//for mem aloc and assert check with better debug
#include "utils.h"
#include "snake_impact.h"
#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "snake.h"
#include "space_impact.h"

#include "font_types.h"
//trying multi thread
pthread_mutex_t mtx;
pthread_cond_t cond;

void* input_thread(void*);// input processing thread
//this shoud refresh screen at given fps(30/60)
void* window_thread(void*);
//used to control LEDs
void* tinkyblinky_thread(void*);

//snake's stuff
int diff = 2;

//end snake's stuff


int arrow_pos = 2;
int font_size = 2;
font_descriptor_t* fdes = &font_winFreeSystem14x16;
bool leds = true;
bool invert = false;
//unsigned char *led_mem_base;
//unsigned char *parlcd_mem_base;
data_t data = {.quit = false};
high_t snake_high = {.saved = 0};
high_t space_high = {.saved = 0};
high_scores_t main_high = {.snake = &snake_high, .space = &space_high, .placaes = {"1st", "2nd", "3rd", "4th", "5th"}};


struct menu_t main_menu = { .main_str = {"SNAKE_IMPACT", " ", "SNAKE", "SPACE IMPACT", "HIGH SCORES", "SETTINGS", "QUIT"},
	.num_of_entries = 6, .def_pos = 2
};
struct menu_t snake_menu = { .main_str = {"PLAY", "SNAKE SPEED", "LEVEL", "HIGH SCORES", "PC PLAYER", "MAIN MENU"},
	.num_of_entries = 5, .def_pos = 0
};
struct menu_t space_menu = { .main_str = {"PLAY", "DIFFICULTY", "HIGH SCORES", "PC PLAYER", "MAIN MENU"},
	.num_of_entries = 4, .def_pos = 0
};
struct menu_t high_score = { .main_str = { "SNAKE", "SPACE IMPACT", "DELETE ALL", "MAIN MENU"},
	.num_of_entries = 3, .def_pos = 0
};
struct menu_t settings = { .main_str = {"FONT SIZE", "FONT TYPE", "TURN ON/OFF LEDS", "INVERT COLORS", "MAIN MENU"},
	.num_of_entries = 4, .def_pos = 0
};

// - main function ------------------------------------------------------------
int main(int argc, char *argv[]) {

	data.fb  = (unsigned short *)malloc(320 * 480 * 3);
	sleep(1);
	led_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
	parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
	if (read_high_scores() != true)
	{
		fprintf(stderr, "ERROR:while reading high scores\n");
	}


	//naming the threads
	enum { INPUT, WINDOW, LED_PER, NUM_THREADS };
	const char *threads_names[] = { "Input", "Display refresh", "Lights" };

	void* (*thr_functions[])(void*) = { input_thread, window_thread, tinkyblinky_thread};

	pthread_t threads[NUM_THREADS];
	pthread_mutex_init(&mtx, NULL); // initialize mutex with default attributes
	pthread_cond_init(&cond, NULL); // initialize mutex with default attributes

	for (int i = 0; i < NUM_THREADS; ++i) {
		int r = pthread_create(&threads[i], NULL, thr_functions[i], &data);
		fprintf(stderr, "INFO: Create thread '%s' %s\n", threads_names[i], ( r == 0 ? "OK" : "FAIL") );
	}

	/* If mapping fails exit with error code */
	my_assert(led_mem_base != NULL, __func__, __LINE__, __FILE__);
	my_assert(parlcd_mem_base != NULL, __func__, __LINE__, __FILE__);



	//LCD initialize
	parlcd_hx8357_init(parlcd_mem_base);
	parlcd_write_cmd(parlcd_mem_base, 0x2c);
	display_clean(parlcd_mem_base);
	fprintf(stderr, "Screen clear\n");

	//display_update(data.parlcd_mem_base);
	call_termios(0);
	fprintf(stderr, "Terminal set\n");

	menu(&main_menu, 'M', font_size);




	void * ret;
	pthread_join(threads[0], &ret);
	if (ret == NULL)
	{
		fprintf(stdout, "Input_thread succesfully ended.\n");
	}
	call_termios(1);
	fprintf(stderr, "Terminal restored\n");
	//wating till end of terminal_t
	pthread_join(threads[1], &ret);
	if (ret == NULL)
	{
		fprintf(stdout, "Window_thread succesfully ended.\n");
	}
	//wating till end of read_t
	pthread_join(threads[2], &ret);
	if (ret == NULL)
	{
		fprintf(stdout, "Tinkyblinky_thread succesfully ended.\n");
	}
	if (write_high_scores() == true)
	{
		fprintf(stderr, "High scores saved\n");
	} else
	{
		fprintf(stderr, "ERROR unable to save high scores\n");

	}

	draw_string(50, 50, "Have a nice day", 5, 'M', get_color(20, 20, 255));

	display_clean(parlcd_mem_base);
	return 0;
}

void* input_thread(void* d)
{
	char in;
	struct menu_t arr_menu[5] = {main_menu, snake_menu, space_menu, high_score, settings};
	int pos = 0;
	bool end = check_end(&data);
	while (end == false)
	{
		if (scanf("%c", &in) == 1)
		{
			switch (in)
			{
			case 's':
				if (arrow_pos < arr_menu[pos].num_of_entries)
				{
					fprintf(stderr, "'s' pressed\n");
					arrow_pos += 1;
					menu(&arr_menu[pos], 'M', font_size);
				}
				break;
			case 'w':
				if (arrow_pos > arr_menu[pos].def_pos)
				{
					fprintf(stderr, "'w' pressed\n");
					arrow_pos -= 1;
					menu(&arr_menu[pos], 'M', font_size);
				}
				break;
			case 'l':
				switch (arrow_pos)
				{
				case 2://snake
					my_snake_menu(&arr_menu[0]);
					menu(&arr_menu[0], 'M', font_size);
					break;
				case 3://space impact
					arrow_pos = 0;
					if (space_impact_menu(&arr_menu[0]) != true)
					{
						fprintf(stderr, "ERROR while calling space impact\n");
					}
					pos = arrow_pos - 2;
					menu(&arr_menu[0], 'M', font_size);
					break;
				case 4://high scores
					arrow_pos = 0;
					if (high_menu(&arr_menu[0]) != true)
					{
						fprintf(stderr, "ERROR while calling high scores\n");
					}
					pos = arrow_pos - 2;
					menu(&arr_menu[0], 'M', font_size);
					break;
				case 5://settings
					arrow_pos = 0;
					if (settings_menu(&arr_menu[0]) != true)
					{
						fprintf(stderr, "ERROR while calling settings\n");
					}
					pos = arrow_pos - 2;
					menu(&arr_menu[0], 'M', font_size);
					break;
				case 6://quit
					fprintf(stderr, "Quit sequence started\n");
					pthread_mutex_lock(&mtx);
					data.quit = true;
					pthread_mutex_unlock(&mtx);
					break;
				}
				break;
			case 'q':
				fprintf(stderr, "Quit sequence started\n");
				pthread_mutex_lock(&mtx);
				data.quit = true;
				pthread_mutex_unlock(&mtx);
				break;
			default:
				break;
			}
			pthread_cond_broadcast(&cond);
			end = check_end(&data);
			printf("%d\n", arrow_pos);

		}
	}
	return NULL;

}
void* window_thread(void* d)
{
	bool end = check_end(&data);
	while (end == false)
	{
		pthread_cond_wait(&cond, &mtx);
		end = data.quit;
		display_update(parlcd_mem_base);
	}
	return NULL;
}
void* tinkyblinky_thread(void* d)
{
	welkom_lights(led_mem_base);
	bool end = check_end(&data);
	*(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB1_o) = 0x33ff33;
	*(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB2_o) = 0xff3399;
	uint32_t store = 0xff3399;
	struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 150 * 1000 * 1000};
	uint32_t val_line = 7;
	int count = 0;
	int off_on = 1;
	while (end == false) {
		if (leds == false)
		{
			val_line = 0;
			off_on = 0;
			* (volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB1_o) = 0;
			* (volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB2_o) = 0;
			store = 0;
		} if (leds == true && off_on == 0)
		{
			off_on = 1;
			*(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB1_o) = 0x33ff33;
			*(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB2_o) = 0xff3399;
			val_line = 7;
		}
		store = *(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB1_o);
		* (volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB1_o) =
		    *(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB2_o);
		* (volatile uint32_t*)(led_mem_base + SPILED_REG_LED_RGB2_o) = store;
		*(volatile uint32_t*)(led_mem_base + SPILED_REG_LED_LINE_o) = val_line;
		val_line <<= 1;
		if (count++ > 32)
		{
			val_line = 7;
			count = 0;
		}
		clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
		end = check_end(&data);
		printf("end: %d\n", end);
	}
	return NULL;
}

// - function -----------------------------------------------------------------
bool space_impact_menu(struct menu_t *arr)
{
	char in;
	int pos = 2;
	bool end = check_end(&data);
	while (end == false)
	{
		end = check_end(&data);
		menu(&arr[pos], 'M', font_size);
		if (scanf("%c", &in) == 1)
		{
			switch	(in) {
			case 's':
				if (arrow_pos < space_menu.num_of_entries)
				{
					arrow_pos += 1;
				}
				break;
			case 'w':
				if (arrow_pos > 0)
				{
					arrow_pos -= 1;
				}
				break;
			case 'l':
				switch (arrow_pos)
				{
				case 0://play
					space_impact(5);

					break;
				case 1://difficulty

					break;
				case 2://High scores

					break;
				case 3://pc player

					break;
				case 4:
					arrow_pos = 2;
					end = true;
					break;
				}
				break;
			}
		}
	}
	return true;

}

// - function -----------------------------------------------------------------
bool my_snake_menu(struct menu_t *arr)
{
	char in;
	int pos = 2;
	bool end = check_end(&data);
	while (end == false)
	{
		end = check_end(&data);
		menu(&arr[pos], 'M', font_size);
		if (scanf("%c", &in) == 1)
		{
			switch	(in) {
			case 's':
				if (arrow_pos < high_score.num_of_entries)
				{
					arrow_pos += 1;
				}
				break;
			case 'w':
				if (arrow_pos > 0)
				{
					arrow_pos -= 1;
				}
				break;
			case 'l':
				switch (arrow_pos)
				{
				case 0://play
					snake(0.2);
					break;
				case 1://difficulty
					
					break;
				case 2://High scores

					break;
				case 3://pc player

					break;
				case 4:
					arrow_pos = 2;
					end = true;
					break;
				}
				break;
			}
		}
	}
	return true;

}
// - function -----------------------------------------------------------------
bool high_menu(struct menu_t *arr)
{
	char in;
	int pos = 3;
	bool end = check_end(&data);
	while (end == false)
	{
		end = check_end(&data);
		menu(&arr[pos], 'M', font_size);
		if (scanf("%c", &in) == 1)
		{
			switch	(in) {
			case 's':
				if (arrow_pos < high_score.num_of_entries)
				{
					arrow_pos += 1;
				}
				break;
			case 'w':
				if (arrow_pos > 0)
				{
					arrow_pos -= 1;
				}
				break;
			case 'l':
				switch (arrow_pos)
				{
				case 0://snake
					display_clean(parlcd_mem_base);
					for (int i = 0; i < snake_high.saved; ++i)
					{
						draw_string(0, 10, "TOP 5 in SNAKE", 2, 'M', get_color(255, 255, 255));
						draw_string(0, 10 + ((i + 1) * 52), main_high.placaes[i], 2, 'L', get_color(255, 255, 0));
						draw_string(0, 10 + ((i + 1) * 52), snake_high.name[i], 2, 'M', get_color(255, 51, 51));
						draw_string(0, 10 + ((i + 1) * 52), snake_high.values[i], 2, 'R', get_color(255, 51, 255));
					}
					scanf("%c", &in);
					break;
				case 1://space impact
					display_clean(parlcd_mem_base);
					for (int i = 0; i < space_high.saved; ++i)
					{
						draw_string(0, 10, "TOP 5 in SPACE-IMPACT", 2, 'M', get_color(255, 255, 255));
						draw_string(0, 10 + ((i + 1) * 52), main_high.placaes[i], 2, 'L', get_color(255, 255, 0));
						draw_string(0, 10 + ((i + 1) * 52), space_high.name[i], 2, 'M', get_color(255, 51, 51));
						draw_string(0, 10 + ((i + 1) * 52), space_high.values[i], 2, 'R', get_color(255, 51, 255));
					}
					scanf("%c", &in);
					break;
				case 2://delete all

					break;
				case 3:
					arrow_pos = 2;
					end = true;
					break;
				}
				break;
			}
		}
	}
	return true;
}
// - function -----------------------------------------------------------------
bool settings_menu(struct menu_t *arr)
{
	char in;
	int pos = 4;
	bool end = check_end(&data);
	while (end == false)
	{
		menu(&arr[pos], 'M', font_size);
		end = check_end(&data);
		if (scanf("%c", &in) == 1)
		{
			switch	(in) {
			case 's':
				if (arrow_pos < 4)
				{
					arrow_pos += 1;
				}
				break;
			case 'w':
				if (arrow_pos > 0)
				{
					arrow_pos -= 1;
				}
				break;
			case 'l':
				switch (arrow_pos)
				{
				case 0:
					display_clean(parlcd_mem_base);
					draw_string(0, 80, "Choose size of font", font_size, 'M', get_color(255, 255, 255));
					draw_string(0, 120, "Size of text 1", 1, 'L', get_color(255, 255, 255));
					draw_string(0, 160, "Size of text 2", 2, 'L', get_color(255, 255, 255));
					draw_string(0, 220, "Size of text 3", 3, 'L', get_color(255, 255, 255));
					if (scanf("%c", &in) == 1)
					{
						if (in == '1') {font_size = 1;}
						if (in == '2') {font_size = 2;}
						if (in == '3') {font_size = 3;}
						if (in != '1' && in != '2' && in != '3') {
							display_clean(parlcd_mem_base);
							draw_string(0, 120, "Invalid choice", 4, 'm', get_color(255, 0, 0));
							sleep(1);
						}
					}
					break;
				case 1:
					display_clean(parlcd_mem_base);
					draw_string(0, 60, "Choose type of font", font_size, 'M', get_color(255, 255, 255));

					draw_string(0, 120, "1 - font_winFreeSystem14x16", font_size, 'L', get_color(255, 255, 255));
					fdes = &font_rom8x16;
					draw_string(0, 180, "2 - font_rom8x16", font_size, 'L', get_color(255, 255, 255));
					if (scanf("%c", &in) == 1)
					{
						if (in == '1') {fdes = &font_winFreeSystem14x16;}
						if (in != '1' && in != '2') {
							display_clean(parlcd_mem_base);
							draw_string(0, 120, "Invalid choice", 4, 'm', get_color(255, 0, 0));
							sleep(1);
						}
					}
					break;
				case 2:
					leds = !leds;
					break;
				case 3:
					invert = !invert;
					break;
				case 4:
					arrow_pos = 2;
					end = true;
					break;
				}
				break;
			case 'q':
				fprintf(stderr, "Quit sequence started\n");
				pthread_mutex_lock(&mtx);
				data.quit = true;
				pthread_mutex_unlock(&mtx);
				break;
			}
		}
	}
	return true;
}

// - function -----------------------------------------------------------------
void menu(struct menu_t *menu, char lining, int size)
{
	display_clean(parlcd_mem_base);
	int y = 5, x = 5;
	int arrow_size = (3 * char_width('-') + char_width('>')) * font_size;
	for (int i = 0; i <= menu->num_of_entries; ++i)
	{
		if (i == arrow_pos)
		{
			char *ch = menu->main_str[i];
			int str_width = 0;
			while (*ch != '\0')
			{
				str_width += size * char_width( *ch++);
			}
			draw_string((250 + (str_width / 2)), y, "<---", size, 'D', get_color(255, 0, 0));
			draw_string((230 - (str_width / 2) - arrow_size), y, "--->", size, 'D', get_color(255, 0, 0));
		}
		draw_string(x, y, menu->main_str[i], size, lining, get_color(0, 255, 0));

		y += 17 * size;
	}
}

// - function -----------------------------------------------------------------
void draw_string(int x, int y, char *str, int size, char lining, uint16_t color)
{
	char *ch = str;
	int str_width = 0;
	while (*ch != '\0')
	{
		str_width += size * char_width(*ch++);
	}
	ch = str;
	switch (lining)
	{
	case 'M':
		x = (480 - str_width) / 2;
		break;
	case 'L':
		x = 10;
		break;
	case 'R':
		x = (480 - str_width - 10);
		break;
	default:
		break;
	}
	while (*ch != '\0')
	{
		draw_char(x, y, *ch, size, color);
		x += size * char_width(*ch);
		ch++;

	}
	pthread_cond_broadcast(&cond);

}

// - function -----------------------------------------------------------------
void display_clean(unsigned char *parlcd_mem_base)
{
	//cleans the screen
	unsigned int c;
	int ptr = 0;
	for (int i = 0; i < 320 ; i++) {
		for (int j = 0; j < 480 ; j++) {
			c = get_color(0, 0, 0);
			pthread_mutex_lock(&mtx);
			data.fb[ptr] = c;
			parlcd_write_data(parlcd_mem_base, data.fb[ptr++]);
			pthread_mutex_unlock(&mtx);
		}
	}
}

// - function -----------------------------------------------------------------
void display_update(unsigned char *parlcd_mem_base)
{
	//prints out the frame buffer
	int ptr = 0;
	for (int i = 0; i < 320 ; i++) {
		for (int j = 0; j < 480 ; j++) {
			parlcd_write_data(parlcd_mem_base, data.fb[ptr++]);
		}
	}
}

// - function -----------------------------------------------------------------
uint16_t get_color(int r, int g, int b) {
	if (invert == true)
	{
		r = 255 - r;
		g = 255 - g;
		b = 255 - b;
	}
	uint16_t color = (r << 11) | (g << 6) | b;
	return color;
}

// - function -----------------------------------------------------------------
bool welkom_lights(unsigned char *mem_base)
{
//LED startup
	uint32_t val_line = 1;
	fprintf(stderr, "Lights Out and away we go !!");
	struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 35 * 1000 * 1000};
	uint32_t store = 0xff3399;
	*(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = 0x33ff33;
	*(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB2_o) = 0xff3399;
	for (int i = 0; i < 60; i++) {
		*(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = val_line;
		if (i % 4 == 0) {
			store = *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o);
			* (volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) =
			    *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB2_o);
			* (volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB2_o) = store;
		}
		if (i < 30)
			val_line <<= 1;
		else if (i < 60)
			val_line >>= 1;
		else
			val_line = 0;
		clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
	}
	printf("\n");
	return true;
}

// - function -----------------------------------------------------------------
bool draw_pixel(int x, int y, unsigned short color) {
	if (x >= 0 && x < 480 && y >= 0 && y < 320) {
		pthread_mutex_lock(&mtx);
		data.fb[x + 480 * y] = color;
		pthread_mutex_unlock(&mtx);
	} else
	{
		fprintf(stderr, "ERROR pixel write is out of range\n");
		return false;
	}
	return true;
}

// - function -----------------------------------------------------------------
void draw_char(int x, int y, char ch, int size, uint16_t color) {
	if (ch < fdes->firstchar)
		return;
	ch -= fdes->firstchar;
	if (ch >= fdes->size)
		return;

	int w = (fdes->width == 0) ? fdes->maxwidth : fdes->width[ch];
	int position = ch * fdes->height;
	for (int iy = 0; iy < (fdes->height * size); iy += size)
	{
		int d = fdes->bits[position++];
		int m = 0x8000;
		//-------------DEBUG----------------------------------
		//printf("m:%d d:%d w:%d\n", m, d, w);
		//----------------------------------------------------
		for (int ix = 0; ix < (w * size); ix += size)
		{
			if (d & m)
			{
				for (int sy = 0; sy < size; ++sy)
				{
					for (int sx = 0; sx < size; ++sx)
					{
						draw_pixel(x + ix + sx ,
						           y + iy + sy, color);
					}
				}

			} else
			{
				for (int sy = 0; sy < size; ++sy)
				{
					for (int sx = 0; sx < size; ++sx)
					{
						draw_pixel(x + ix + sx ,
						           y + iy + sy, get_color(0, 0, 0));
					}
				}
			}

			m >>= 01;
		}
	}
}

// - function -----------------------------------------------------------------
int char_width(int ch) {
	int width = 0;
	if ((ch >= fdes->firstchar) && (ch - fdes->firstchar < fdes->size)) {
		ch -= fdes->firstchar;
		if (!fdes->width) {
			width = fdes->maxwidth;
		} else {
			width = fdes->width[ch];
		}
	}
	return width;
}

// - function -----------------------------------------------------------------
bool check_end(data_t *d)
{
	pthread_mutex_lock(&mtx);
	bool ret = d->quit;
	pthread_mutex_unlock(&mtx);
	return ret;
}

// - function -----------------------------------------------------------------
void call_termios(int reset)
{
	static struct termios tio, tioOld;
	tcgetattr(STDIN_FILENO, &tio);
	if (reset) {
		tcsetattr(STDIN_FILENO, TCSANOW, &tioOld);
	} else {
		tioOld = tio; //backup
		cfmakeraw(&tio);
		tio.c_oflag |= OPOST;
		tcsetattr(STDIN_FILENO, TCSANOW, &tio);
	}
}
// - function -----------------------------------------------------------------
bool read_high_scores(void)
{
	FILE *high_saved = fopen("high_scores.txt", "r");
	if (high_saved == NULL) {return false;}
	else
	{
		char in;
		char name[30];
		char score[10];
		fscanf(high_saved, "%c\n", &in);
		for (int i = 0; i < 5; ++i)
		{
			if (fscanf(high_saved, "%s %s\n", name, score) == 2)
			{
				space_high.saved += 1;
				strcpy(space_high.name[i], name);
				strcpy(space_high.values[i], score);
			}
		}
		fscanf(high_saved, "%c\n", &in);
		for (int i = 0; i < 5; ++i)
		{
			if (fscanf(high_saved, "%s %s\n", name, score) == 2)
			{
				snake_high.saved += 1;
				strcpy(snake_high.name[i], name);
				strcpy(snake_high.values[i], score);
			}
		}
	}
	if (fclose(high_saved) != 0)
	{
		return false;
	}
	return true;
}

// - function -----------------------------------------------------------------
bool write_high_scores(void)
{
	FILE *high_save = fopen("high_scores.txt", "w");
	if (high_save == NULL)
	{return false;}
	fprintf(high_save, "I\n");
	for (int i = 0; i < 5; ++i)
	{
		fprintf(high_save, "%s %s\n", space_high.name[i], space_high.values[i]);

	}
	fscanf(high_save, "S\n");
	for (int i = 0; i < 5; ++i)
	{
		fprintf(high_save, "%s %s\n", snake_high.name[i], snake_high.values[i]);
	}
	return true;
}


