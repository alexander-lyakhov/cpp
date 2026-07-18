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
	char* source_chars;
	char* target_chars;
	WORD* target_attrs;

} Buff;

// ================================================================================
// @@@ + Buff_init
// ================================================================================
Buff Buff_init(uint16_t size)
{
	return (Buff) {
		.source_chars = malloc(size * sizeof(char)),
		.target_chars = malloc(size * sizeof(char)),
		.target_attrs = malloc(size * sizeof(WORD)),
	};
}

// ================================================================================
// @@@ + Buff_free
// ================================================================================
void Buff_free(Buff *buff)
{
	free(buff->source_chars);
	free(buff->target_chars);
	free(buff->target_attrs);
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

	Buff buff = Buff_init(size);

	printf("0x%p\n", buff.source_chars);
	printf("0x%p\n", buff.target_chars);
	printf("0x%p\n", buff.target_attrs);

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
	srand(time(NULL));

	for (uint16_t i = 0; i < console->size; i++)
	{
		uint16_t RND = rand() % 101;

		console->buff.target_chars[i] = RND < 20 ? ALIVE_CELL_CHAR : EMPTY_CELL_CHAR;
		console->buff.target_attrs[i] = RND < 20 ? ALIVE_CELL_ATTR : EMPTY_CELL_ATTR;
	}
}

// =============================================================================
// @@@ + app_listen
// =============================================================================
int app_listen()
{
	if (_kbhit())
	{
		char key = _getch();
		if (key == 27 || ((key | 32) == 'q')) return 0;
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

				if (buff->source_chars[check_index] == ALIVE_CELL_CHAR) n++;
			}
		}
		
		switch(n) {
			// no changes
			case 2:
				buff->target_chars[index] = buff->source_chars[index];
				buff->target_attrs[index] = buff->source_chars[index] == ALIVE_CELL_CHAR ? ALIVE_CELL_ATTR : EMPTY_CELL_ATTR;

				break;
			
			// New cell is borning
			case 3:
				buff->target_chars[index] = ALIVE_CELL_CHAR;
				buff->target_attrs[index] = ALIVE_CELL_ATTR;

				break;
		
			// Cell dies (n < 2 or n > 3)
			default:
				buff->target_chars[index] = EMPTY_CELL_CHAR;
				buff->target_attrs[index] = EMPTY_CELL_ATTR;
		}
	}
}

// =============================================================================
// @@@ + app_render
// =============================================================================
void app_render(Console *console)
{
	Buff *buff = &console->buff;

	WriteConsoleOutputCharacter(console->handle, buff->target_chars, console->size, (COORD){0, 0}, &console->written);
	WriteConsoleOutputAttribute(console->handle, buff->target_attrs, console->size, (COORD){0, 0}, &console->written);

	char *p = buff->source_chars;
	buff->source_chars = buff->target_chars;
	buff->target_chars = p;
}


int main()
{
	system("cls");

	Console console = Console_create();

	CURSOR_INIT;
	CURSOR_HIDE;

	app_init(&console);
	app_render(&console);

	// GameLoop
	while(app_listen())
	{
		app_update(&console);
		app_render(&console);
		Sleep(50);
	}

	Buff_free(&console.buff);

	CURSOR_SHOW;

	_getch();

	return 0;
}
