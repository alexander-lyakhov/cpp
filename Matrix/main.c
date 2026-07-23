#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#define ATTR_BLACK 0x00
#define ATTR_WHITE 0x0F
#define ATTR_GREEN 0x0A

typedef struct {
	uint16_t count;
	WORD attr;
} Drop;

typedef struct {
	HANDLE handle;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD written;

	char* buff;
	WORD* attrs;
	Drop *drops;

	uint16_t width;
	uint16_t height;
	uint16_t size;

} Console;

// ================================================================================
// @@@ + Drop_create
// ================================================================================
Drop Drop_create(Console *console)
{
	uint8_t count = (rand() % console->height >> 1) + 3; // min length
	WORD attr = (rand() % 100) > 75 ? ATTR_GREEN : ATTR_BLACK;

	return (Drop) {
		.count = count,
		.attr = attr
	};
}

// ================================================================================
// @@@ + Console_create
// ================================================================================
Console Console_create()
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(handle, &csbi);

	uint16_t width  = csbi.srWindow.Right + 1;
	uint16_t height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	uint16_t size   = width * height;

	Drop *drops = malloc(sizeof(Drop) * width);

	return (Console) {
		.handle = handle,
		.width  = width,
		.height = height,
		.size   = size,
		.buff   = malloc(size * sizeof(char)),
		.attrs  = malloc(size * sizeof(WORD)),
		.drops  = drops,
	};
}

// =============================================================================
// @@@ + Console_destroy
// =============================================================================
void Console_destroy(Console *console)
{
	free(console->buff);
	free(console->attrs);
	free(console->drops);
}

// ================================================================================
// @@@ + App_init
// ================================================================================
void App_init(Console *console)
{
	srand(time(NULL));

	for (int i = 0; i < console->size; i++)
		console->buff[i] = rand() % 94 + 32;

	memset(
		console->attrs,
		ATTR_BLACK,
		console->size * sizeof(*console->attrs)
	);

	for (int i = 0; i < console->width; i++)
	{
		console->drops[i] = Drop_create(console);
		console->attrs[i] = console->drops[i].attr != ATTR_BLACK
			? ATTR_WHITE
			: console->drops[i].attr;
	}
}


// ================================================================================
// @@@ + render_buff
// ================================================================================
void render_buff(Console *console)
{
	WriteConsoleOutputCharacter(
		console->handle,
		console->buff,
		console->size,
		(COORD){0, 0},
		&console->written
	);
}

// ================================================================================
// @@@ + render_attrs
// ================================================================================
void render_attrs(Console *console)
{
	WriteConsoleOutputAttribute(
		console->handle,
		console->attrs,
		console->size,
		(COORD){0, 0},
		&console->written
	);
}

// =============================================================================
// @@@ + App_listen
// =============================================================================
uint16_t App_listen()
{
	int key = 0;

	if (kbhit())
	{
		key = getch();
		if (key == 27 || key == 'q') return 0;
	}
	return 1;
}

// =============================================================================
// @@@ + App_update
// =============================================================================
void App_update(Console *console)
{
	size_t trg_index = console->size - console->width;
	size_t src_index = trg_index - console->width;

	for (int i = console->height - 1; i > 0; i--)
	{
		memcpy(
			&console->attrs[trg_index],
			&console->attrs[src_index],
			console->width * sizeof(*console->attrs)
		);

		trg_index -= console->width;
		src_index -= console->width;
	}
		
	//
	// Very first top line
	//
	for (int i = 0; i < console->width; i++)
	{
		if (!console->drops[i].count--)
			console->drops[i] = Drop_create(console);
		else
		{
			console->attrs[i] = console->drops[i].attr;

			if (console->attrs[i] != ATTR_BLACK && console->attrs[i + console->width] == ATTR_BLACK)
				console->attrs[i] = ATTR_WHITE;
		}
		
		/*
		if (!drops[i].count--)
			drops[i] = create_drop(console);

		console->attrs[i] = drops[i].attr;

		if (console->attrs[i] != ATTR_BLACK && console->attrs[i + console->width] == ATTR_BLACK)
			console->attrs[i] = ATTR_WHITE;
		*/
	}
}

// =============================================================================
// @@@ + App_render
// =============================================================================
void App_render(Console *console)
{
	render_buff(console);
	render_attrs(console);
}

int main()
{
	system("cls");

	Console console = Console_create();
	App_init(&console);
	App_render(&console);

	while (App_listen())
	{
		App_update(&console);
		App_render(&console);

		Sleep(30);
	}

	Console_destroy(&console);

	return 0;
}