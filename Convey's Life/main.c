#include <stdio.h>    
#include <stdint.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include <windows.h>

#define CURSOR_INIT CONSOLE_CURSOR_INFO cursorInfo; GetConsoleCursorInfo(console.handle, &cursorInfo);
#define CURSOR_HIDE cursorInfo.bVisible = 0; SetConsoleCursorInfo(console.handle, &cursorInfo);
#define CURSOR_SHOW cursorInfo.bVisible = 1; SetConsoleCursorInfo(console.handle, &cursorInfo);

#define EMPTY_CELL_CHAR '.'
#define EMPTY_CELL_ATTR 0x08

#define ALIVE_CELL_CHAR '*'
#define ALIVE_CELL_ATTR 0x0A


typedef struct {
	char* data;
	char* curr;
	char* next;
	WORD* attrs;

} Buff;

// ================================================================================
// @@@ + Buff_create
// ================================================================================
Buff Buff_create(uint16_t size)
{
	char *data = malloc(size * sizeof(WORD) + size * sizeof(char) + size * sizeof(char));

	return (Buff) {
		.attrs = (WORD*)data,
		.curr = &data[size * sizeof(WORD)],
		.next = &data[size * sizeof(WORD) + size * sizeof(char)],
	};
}

// ================================================================================
// @@@ + Buff_destroy
// ================================================================================
void Buff_destroy(Buff *buff)
{
	free(buff->data);

	buff->data  = NULL;
	buff->curr  = NULL;
	buff->next  = NULL;
	buff->attrs = NULL;
}

typedef struct {
	HANDLE handle;
	DWORD written;

	Buff buff;

	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	uint16_t size;

} Console;

void app_render(Console *console);

// ================================================================================
// @@@ + Console_create
// ================================================================================
Console Console_create()
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(handle, &csbi);

	uint16_t width  = csbi.srWindow.Right  + 1;
	uint16_t height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	uint16_t size   = width * height;

	Buff buff = Buff_create(size);

	printf("0x%p\n", buff.curr);
	printf("0x%p\n", buff.next);
	printf("0x%p\n", buff.attrs);

	return (Console) {
		.handle = handle,
		.buff   = buff,
		.x      = 0,
		.y      = 0,
		.width  = width,
		.height = height,
		.size   = size,
	};
}


// ================================================================================
// @@@ + app_init
// ================================================================================
void app_init(Console *console)
{
	// srand(time(NULL));

	for (uint16_t i = 0; i < console->size; i++)
	{
		uint16_t RND = rand() % 101;

		console->buff.next[i] = RND < 20 ? ALIVE_CELL_CHAR : EMPTY_CELL_CHAR;
		console->buff.attrs[i] = RND < 20 ? ALIVE_CELL_ATTR : EMPTY_CELL_ATTR;
	}
}

// =============================================================================
// @@@ + app_reset
// =============================================================================
int app_reset(Console *console)
{
	Buff_destroy(&(console->buff));
	console->buff = Buff_create(console->size);
	app_init(console);

	return 1;
}

// =============================================================================
// @@@ + app_listen
// =============================================================================
int app_listen(Console *console)
{
	if (_kbhit())
	{
		char key = _getch();
		if (key == 27 || ((key | 32) == 'q')) return 0;
		if ((key | 32) == 'r') return app_reset(console);
	}

	return 1;
}

// =============================================================================
// @@@ + app_update
// =============================================================================
void app_update(Console *console)
{
	Buff *buff = &console->buff;

	for (int index = 0; index < console->size; index++)
	{
		int row = index / console->width;
		int col = index % console->width;
		int n = 0;

		for (int y = row - 1; y <= row + 1; y++)
		{
			if (y < 0 || y > console->height - 1)
				continue;

			for (int x = col - 1; x <= col + 1; x++)
			{
				if (x < 0 || x > console->width - 1)
					continue;
				
				if (y == row && x == col)
					continue;
				
				int check_index = y * console->width + x;

				if (buff->curr[check_index] == ALIVE_CELL_CHAR) n++;
			}
		}
		
		switch(n) {
			// no changes
			case 2:
				buff->next[index] = buff->curr[index];
				buff->attrs[index] = buff->curr[index] == ALIVE_CELL_CHAR ? ALIVE_CELL_ATTR : EMPTY_CELL_ATTR;

				break;
			
			// New cell is borning
			case 3:
				buff->next[index] = ALIVE_CELL_CHAR;
				buff->attrs[index] = ALIVE_CELL_ATTR;

				break;
		
			// Cell dies (n < 2 or n > 3)
			default:
				buff->next[index] = EMPTY_CELL_CHAR;
				buff->attrs[index] = EMPTY_CELL_ATTR;
		}
	}
}

// =============================================================================
// @@@ + app_render
// =============================================================================
void app_render(Console *console)
{
	Buff *buff = &console->buff;
	
	WriteConsoleOutputCharacter(console->handle, buff->next, console->size, (COORD){0, 0}, &console->written);
	WriteConsoleOutputAttribute(console->handle, buff->attrs, console->size, (COORD){0, 0}, &console->written);
	
	char *p = buff->curr;
	buff->curr = buff->next;
	buff->next = p;
}


int main()
{
	system("cls");

	srand(time(NULL));
		
	Console console = Console_create();

	CURSOR_INIT;
	CURSOR_HIDE;

	app_init(&console);
	// app_render(&console);

	// GameLoop
	while (app_listen(&console))
	{
		app_render(&console);
		app_update(&console);

		Sleep(50);
	}
	Buff_destroy(&console.buff);
	
	CURSOR_SHOW;
	
	return 0;
}
