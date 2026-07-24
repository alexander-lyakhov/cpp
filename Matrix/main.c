#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <windows.h>
#include <time.h>
// #include <unistd.h>

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

	uint16_t width;
	uint16_t height;
	uint16_t size;

} Console;

typedef struct {
	Console *console;
	Drop *drops;

} App;

void App_skip_begin(App *app);

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

	return (Console) {
		.handle = handle,
		.width  = width,
		.height = height,
		.size   = size,
		.buff   = malloc(size * sizeof(char)),
		.attrs  = malloc(size * sizeof(WORD)),
	};
}

// =============================================================================
// @@@ + Console_new
// =============================================================================
Console* Console_new()
{
	Console *console = malloc(sizeof(Console));

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(handle, &csbi);

	uint16_t width  = csbi.srWindow.Right + 1;
	uint16_t height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	uint16_t size   = width * height;

	console->handle = handle;
	console->width  = width;
	console->height = height;
	console->size   = size;
	console->buff   = malloc(size * sizeof(char));
	console->attrs  = malloc(size * sizeof(WORD));

	return console;
}

// =============================================================================
// @@@ + Console_free
// =============================================================================
void Console_free(Console *console)
{
	free(console->buff);
	free(console->attrs);
}

// =============================================================================
// @@@ + App_create
// =============================================================================
App App_create()
{
	srand(time(NULL));

	Console *console = Console_new();
	Drop *drops      = malloc(sizeof(Drop) * console->width);

	for (int i = 0; i < console->size; i++)
		console->buff[i] = rand() % 94 + 32;

	memset(
		console->attrs,
		ATTR_BLACK,
		console->size * sizeof(*console->attrs)
	);

	for (int i = 0; i < console->width; i++)
	{
		drops[i] = Drop_create(console);

		console->attrs[i] = drops[i].attr != ATTR_BLACK
			? ATTR_WHITE
			: drops[i].attr;
	}

	return (App) {
		.console = console,
		.drops = drops,
	};
}

// =============================================================================
// @@@ + App_reset
// =============================================================================
void App_reset(App *app)
{
	free(app->drops);
	Console_free(app->console);

	system("cls");

	Console *console = app->console;
	Drop *drops      = malloc(sizeof(Drop) * console->width);

	console->buff   = malloc(console->size * sizeof(char));
	console->attrs  = malloc(console->size * sizeof(WORD));

	for (int i = 0; i < console->size; i++)
		console->buff[i] = rand() % 94 + 32;

	memset(
		console->attrs,
		ATTR_BLACK,
		console->size * sizeof(*console->attrs)
	);

	for (int i = 0; i < console->width; i++)
	{
		drops[i] = Drop_create(console);

		console->attrs[i] = drops[i].attr != ATTR_BLACK
			? ATTR_WHITE
			: drops[i].attr;
	}

	app->drops = drops;
	App_skip_begin(app);
}

// =============================================================================
// @@@ + App_destroy
// =============================================================================
void App_destroy(App *app)
{
	Console_free(app->console);

	free(app->drops);
	free(app->console);
}

// =============================================================================
// @@@ + App_check_resize
// =============================================================================
uint16_t App_check_resize(App *app)
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(handle, &csbi);

	uint16_t width  = csbi.srWindow.Right  + 1;
	uint16_t height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	if (app->console->width != width || app->console->height != height)
	{
		app->console->width  = width;
		app->console->height = height;
		app->console->size   = width * height;

		App_reset(app);

		return 1;
	}

	return 0;
}

// =============================================================================
// @@@ + App_listen
// =============================================================================
uint16_t App_listen(App *app)
{
	if (App_check_resize(app)) return 1;

	if (kbhit())
	{
		int key = getch();
		if (key == 27 || key == 'q') return 0;
	}
	return 1;
}

// =============================================================================
// @@@ + App_update
// =============================================================================
void App_update(App *app)
{
	Console *console = app->console;

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
		if (!app->drops[i].count--)
			app->drops[i] = Drop_create(console);
		else
		{
			console->attrs[i] = app->drops[i].attr;

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

// ================================================================================
// @@@ + App_skip_begin
// ================================================================================
void App_skip_begin(App *app)
{
	for (int row = 0; row < app->console->height - 1; row++)
		App_update(app);
}

// =============================================================================
// @@@ + App_render
// =============================================================================
void App_render(App *app)
{
	Console *console = app->console;

	WriteConsoleOutputCharacter(
		console->handle,
		console->buff,
		console->size,
		(COORD){0, 0},
		&console->written
	);

	WriteConsoleOutputAttribute(
		console->handle,
		console->attrs,
		console->size,
		(COORD){0, 0},
		&console->written
	);
}

int main()
{
	system("cls");

	App app = App_create();

	App_skip_begin(&app);

	while (App_listen(&app))
	{
		App_render(&app);
		App_update(&app);

		Sleep(30);
		// usleep(30000);
	}
	
	App_destroy(&app);

	return 0;
}