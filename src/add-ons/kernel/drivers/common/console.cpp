/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <Drivers.h>
#include <KernelExport.h>

#include <console.h>
#include <lock.h>

#include <string.h>
#include <stdio.h>


#define DEVICE_NAME "console"

#define TAB_SIZE 8
#define TAB_MASK 7

#define FMASK 0x0f
#define BMASK 0x70

typedef enum {
	CONSOLE_STATE_NORMAL = 0,
	CONSOLE_STATE_GOT_ESCAPE,
	CONSOLE_STATE_SEEN_BRACKET,
	CONSOLE_STATE_NEW_ARG,
	CONSOLE_STATE_PARSING_ARG,
} console_state;

typedef enum {
	SCREEN_ERASE_WHOLE,
	SCREEN_ERASE_UP,
	SCREEN_ERASE_DOWN
} erase_screen_mode;

typedef enum {
	LINE_ERASE_WHOLE,
	LINE_ERASE_LEFT,
	LINE_ERASE_RIGHT
} erase_line_mode;

#define MAX_ARGS 8

static struct console_desc {
	mutex	lock;

	int32	lines;
	int32	columns;

	uint8	attr;
	uint8	saved_attr;
	bool	bright_attr;
	bool	reverse_attr;

	int32	x;						/* current x coordinate */
	int32	y;						/* current y coordinate */
	int32	saved_x;				/* used to save x and y */
	int32	saved_y;

	int32	scroll_top;	/* top of the scroll region */
	int32	scroll_bottom;	/* bottom of the scroll region */

	/* state machine */
	console_state state;
	int32	arg_ptr;
	int32	args[MAX_ARGS];

	char	module_name[B_FILE_NAME_LENGTH];
	console_module_info *module;
} gconsole;

int32 api_version = B_CUR_DRIVER_API_VERSION;


static inline void
update_cursor(struct console_desc *con, int x, int y)
{
	con->module->move_cursor(x, y);
}


static void
gotoxy(struct console_desc *con, int new_x, int new_y)
{
	if (new_x >= con->columns)
		new_x = con->columns - 1;
	if (new_x < 0)
		new_x = 0;
	if (new_y >= con->lines)
		new_y = con->lines - 1;
	if (new_y < 0)
		new_y = 0;

	con->x = new_x;
	con->y = new_y;
}


static void
reset_console(struct console_desc *con)
{
	con->attr = 0x0f;
	con->scroll_top = 0;
	con->scroll_bottom = con->lines - 1;
	con->bright_attr = true;
	con->reverse_attr = false;
}


/** scroll from the cursor line up to the top of the scroll region up one line */

static void
scrup(struct console_desc *con)
{
	// see if cursor is outside of scroll region
	if (con->y < con->scroll_top || con->y > con->scroll_bottom)
		return;

	if (con->y - con->scroll_top > 1) {
		// move the screen up one
		con->module->blit(0, con->scroll_top + 1, con->columns,
			con->y - con->scroll_top, 0, con->scroll_top);
	}

	// clear the bottom line
	con->module->fill_glyph(0, con->y, con->columns, 1, ' ', con->attr);
}


/** scroll from the cursor line down to the bottom of the scroll region down one line */

static void
scrdown(struct console_desc *con)
{
	// see if cursor is outside of scroll region
	if (con->y < con->scroll_top || con->y > con->scroll_bottom)
		return;

	if (con->scroll_bottom - con->y > 1) {
		// move the screen down one
		con->module->blit(0, con->y, con->columns, con->scroll_bottom - con->y, 0, con->y + 1);
	}

	// clear the top line
	con->module->fill_glyph(0, con->y, con->columns, 1, ' ', con->attr);
}


static void
lf(struct console_desc *con)
{
	//dprintf("lf: y %d x %d scroll_top %d scoll_bottom %d\n", con->y, con->x, con->scroll_top, con->scroll_bottom);

	if (con->y == con->scroll_bottom ) {
 		// we hit the bottom of our scroll region
 		scrup(con);
	} else if(con->y < con->scroll_bottom) {
		con->y++;
	}
}


static void
rlf(struct console_desc *con)
{
	if (con->y == con->scroll_top) {
 		// we hit the top of our scroll region
 		scrdown(con);
	} else if (con->y > con->scroll_top) {
		con->y--;
	}
}


static void
cr(struct console_desc *con)
{
	con->x = 0;
}


static void
del(struct console_desc *con)
{
	if (con->x > 0) {
		con->x--;
	} else if (con->y > 0) {
        con->y--;
        con->x = con->columns - 1;
    } else {
        //This doesn't work...
        //scrdown(con);
        //con->y--;
        //con->x = con->columns - 1;
        return;
    }
	con->module->put_glyph(con->x, con->y, ' ', con->attr);
}


static void
erase_line(struct console_desc *con, erase_line_mode mode)
{
	switch (mode) {
		case LINE_ERASE_WHOLE:
			con->module->fill_glyph(0, con->y, con->columns, 1, ' ', con->attr);
			break;
		case LINE_ERASE_LEFT:
			con->module->fill_glyph(0, con->y, con->x+1, 1, ' ', con->attr);
			break;
		case LINE_ERASE_RIGHT:
			con->module->fill_glyph(con->x, con->y, con->columns - con->x, 1, ' ', con->attr);
			break;
		default:
			return;
	}
}


static void
erase_screen(struct console_desc *con, erase_screen_mode mode)
{
	switch (mode) {
		case SCREEN_ERASE_WHOLE:
			con->module->fill_glyph(0, 0, con->columns, con->lines, ' ', con->attr);
			break;
		case SCREEN_ERASE_UP:
			con->module->fill_glyph(0, 0, con->columns, con->y + 1, ' ', con->attr);
			break;
		case SCREEN_ERASE_DOWN:
			con->module->fill_glyph(con->y, 0, con->columns, con->lines - con->y, ' ', con->attr);
			break;
		default:
			return;
	}
}


static void
save_cur(struct console_desc *con, bool save_attrs)
{
	con->saved_x = con->x;
	con->saved_y = con->y;
	if (save_attrs)
		con->saved_attr = con->attr;
}


static void
restore_cur(struct console_desc *con, bool restore_attrs)
{
	con->x = con->saved_x;
	con->y = con->saved_y;
	if (restore_attrs)
		con->attr = con->saved_attr;
}


static char
console_putch(struct console_desc *con, const char c)
{
	if (++con->x >= con->columns) {
		cr(con);
		lf(con);
	}
	con->module->put_glyph(con->x-1, con->y, c, con->attr);
	return c;
}


static void
tab(struct console_desc *con)
{
	con->x = (con->x + TAB_SIZE) & ~TAB_MASK;
	if (con->x >= con->columns) {
		con->x -= con->columns;
		lf(con);
	}
}


static void
set_scroll_region(struct console_desc *con, int top, int bottom)
{
	if (top < 0)
		top = 0;
	if (bottom >= con->lines)
		bottom = con->lines - 1;
	if (top > bottom)
		return;

	con->scroll_top = top;
	con->scroll_bottom = bottom;
}


static void
set_vt100_attributes(struct console_desc *con, int32 *args, int32 argCount)
{
	for (int32 i = 0; i < argCount; i++) {
		//dprintf("set_vt100_attributes: %ld\n", args[i]);
		switch (args[i]) {
			case 0: // reset
				con->attr = 0x0f;
				con->bright_attr = true;
				con->reverse_attr = false;
				break;
			case 1: // bright
				con->bright_attr = true;
				con->attr |= 0x08; // set the bright bit
				break;
			case 2: // dim
				con->bright_attr = false;
				con->attr &= ~0x08; // unset the bright bit
				break;
			case 4: // underscore we can't do
				break;
			case 5: // blink
				con->attr |= 0x80; // set the blink bit
				break;
			case 7: // reverse
				con->reverse_attr = true;
				con->attr = ((con->attr & BMASK) >> 4) | ((con->attr & FMASK) << 4);
				if (con->bright_attr)
					con->attr |= 0x08;
				break;
			case 8: // hidden?
				break;

			/* foreground colors */
			case 30: con->attr = (con->attr & ~FMASK) | 0 | (con->bright_attr ? 0x08 : 0); break; // black
			case 31: con->attr = (con->attr & ~FMASK) | 4 | (con->bright_attr ? 0x08 : 0); break; // red
			case 32: con->attr = (con->attr & ~FMASK) | 2 | (con->bright_attr ? 0x08 : 0); break; // green
			case 33: con->attr = (con->attr & ~FMASK) | 6 | (con->bright_attr ? 0x08 : 0); break; // yellow
			case 34: con->attr = (con->attr & ~FMASK) | 1 | (con->bright_attr ? 0x08 : 0); break; // blue
			case 35: con->attr = (con->attr & ~FMASK) | 5 | (con->bright_attr ? 0x08 : 0); break; // magenta
			case 36: con->attr = (con->attr & ~FMASK) | 3 | (con->bright_attr ? 0x08 : 0); break; // cyan
			case 37: con->attr = (con->attr & ~FMASK) | 7 | (con->bright_attr ? 0x08 : 0); break; // white

			/* background colors */
			case 40: con->attr = (con->attr & ~BMASK) | (0 << 4); break; // black
			case 41: con->attr = (con->attr & ~BMASK) | (4 << 4); break; // red
			case 42: con->attr = (con->attr & ~BMASK) | (2 << 4); break; // green
			case 43: con->attr = (con->attr & ~BMASK) | (6 << 4); break; // yellow
			case 44: con->attr = (con->attr & ~BMASK) | (1 << 4); break; // blue
			case 45: con->attr = (con->attr & ~BMASK) | (5 << 4); break; // magenta
			case 46: con->attr = (con->attr & ~BMASK) | (3 << 4); break; // cyan
			case 47: con->attr = (con->attr & ~BMASK) | (7 << 4); break; // white
		}
	}
}


static bool
process_vt100_command(struct console_desc *con, const char c,
	bool seen_bracket, int32 *args, int32 argCount)
{
	bool ret = true;

	//dprintf("process_vt100_command: c '%c', argCount %ld, arg[0] %ld, arg[1] %ld, seen_bracket %d\n",
	//	c, argCount, args[0], args[1], seen_bracket);

	if (seen_bracket) {
		switch(c) {
			case 'H': /* set cursor position */
			case 'f': {
				int32 row = argCount > 0 ? args[0] : 1;
				int32 col = argCount > 1 ? args[1] : 1;
				if (row > 0)
					row--;
				if (col > 0)
					col--;
				gotoxy(con, col, row);
				break;
			}
			case 'A': { /* move up */
				int32 deltay = argCount > 0 ? -args[0] : -1;
				if (deltay == 0)
					deltay = -1;
				gotoxy(con, con->x, con->y + deltay);
				break;
			}
			case 'e':
			case 'B': { /* move down */
				int32 deltay = argCount > 0 ? args[0] : 1;
				if (deltay == 0)
					deltay = 1;
				gotoxy(con, con->x, con->y + deltay);
				break;
			}
			case 'D': { /* move left */
				int32 deltax = argCount > 0 ? -args[0] : -1;
				if (deltax == 0)
					deltax = -1;
				gotoxy(con, con->x + deltax, con->y);
				break;
			}
			case 'a':
			case 'C': { /* move right */
				int32 deltax = argCount > 0 ? args[0] : 1;
				if (deltax == 0)
					deltax = 1;
				gotoxy(con, con->x + deltax, con->y);
				break;
			}
			case '`':
			case 'G': { /* set X position */
				int32 newx = argCount > 0 ? args[0] : 1;
				if (newx > 0)
					newx--;
				gotoxy(con, newx, con->y);
				break;
			}
			case 'd': { /* set y position */
				int32 newy = argCount > 0 ? args[0] : 1;
				if (newy > 0)
					newy--;
				gotoxy(con, con->x, newy);
				break;
			}
			case 's': /* save current cursor */
				save_cur(con, false);
				break;
			case 'u': /* restore cursor */
				restore_cur(con, false);
				break;
			case 'r': { /* set scroll region */
				int32 low = argCount > 0 ? args[0] : 1;
				int32 high = argCount > 1 ? args[1] : con->lines;
				if (low <= high)
					set_scroll_region(con, low - 1, high - 1);
				break;
			}
			case 'L': { /* scroll virtual down at cursor */
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrdown(con);
					lines--;
				}
				break;
			}
			case 'M': { /* scroll virtual up at cursor */
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					scrup(con);
					lines--;
				}
				break;
			}
			case 'K':
				if (argCount < 0) {
					// erase to end of line
					erase_line(con, LINE_ERASE_RIGHT);
				} else {
					if (args[0] == 1)
						erase_line(con, LINE_ERASE_LEFT);
					else if (args[0] == 2)
						erase_line(con, LINE_ERASE_WHOLE);
				}
				break;
			case 'J':
				if (argCount < 0) {
					// erase to end of screen
					erase_screen(con, SCREEN_ERASE_DOWN);
				} else {
					if (args[0] == 1)
						erase_screen(con, SCREEN_ERASE_UP);
					else if (args[0] == 2)
						erase_screen(con, SCREEN_ERASE_WHOLE);
				}
				break;
			case 'm':
				if (argCount >= 0)
					set_vt100_attributes(con, args, argCount);
				break;
			default:
				ret = false;
		}
	} else {
		switch (c) {
			case 'c':
				reset_console(con);
				break;
			case 'D':
				rlf(con);
				break;
			case 'M':
				lf(con);
				break;
			case '7':
				save_cur(con, true);
				break;
			case '8':
				restore_cur(con, true);
				break;
			default:
				ret = false;
		}
	}

	return ret;
}


static ssize_t
_console_write(struct console_desc *con, const void *buf, size_t len)
{
	const char *c;
	size_t pos = 0;

	while (pos < len) {
		c = &((const char *)buf)[pos++];

		switch (con->state) {
			case CONSOLE_STATE_NORMAL:
				// just output the stuff
				switch (*c) {
					case '\n':
						lf(con);
						break;
					case '\r':
						cr(con);
						break;
					case 0x8: // backspace
						del(con);
						break;
					case '\t':
						tab(con);
						break;
					case '\0':
						break;
					case 0x1b:
						// escape character
						con->arg_ptr = -1;
						con->state = CONSOLE_STATE_GOT_ESCAPE;
						break;
					default:
						console_putch(con, *c);
				}
				break;
			case CONSOLE_STATE_GOT_ESCAPE:
				// look for either commands with no argument, or the '[' character
				switch (*c) {
					case '[':
						con->state = CONSOLE_STATE_SEEN_BRACKET;
						break;
					default:
						con->args[con->arg_ptr] = 0;
						process_vt100_command(con, *c, false, con->args, con->arg_ptr + 1);
						con->state = CONSOLE_STATE_NORMAL;
				}
				break;
			case CONSOLE_STATE_SEEN_BRACKET:
				switch (*c) {
					case '0'...'9':
						con->arg_ptr = 0;
						con->args[con->arg_ptr] = *c - '0';
						con->state = CONSOLE_STATE_PARSING_ARG;
						break;
					default:
						process_vt100_command(con, *c, true, con->args, con->arg_ptr + 1);
						con->state = CONSOLE_STATE_NORMAL;
				}
				break;
			case CONSOLE_STATE_NEW_ARG:
				switch (*c) {
					case '0'...'9':
						con->arg_ptr++;
						if (con->arg_ptr == MAX_ARGS) {
							con->state = CONSOLE_STATE_NORMAL;
							break;
						}
						con->args[con->arg_ptr] = *c - '0';
						con->state = CONSOLE_STATE_PARSING_ARG;
						break;
					default:
						process_vt100_command(con, *c, true, con->args, con->arg_ptr + 1);
						con->state = CONSOLE_STATE_NORMAL;
				}
				break;
			case CONSOLE_STATE_PARSING_ARG:
				// parse args
				switch (*c) {
					case '0'...'9':
						con->args[con->arg_ptr] *= 10;
						con->args[con->arg_ptr] += *c - '0';
						break;
					case ';':
						con->state = CONSOLE_STATE_NEW_ARG;
						break;
					default:
						process_vt100_command(con, *c, true, con->args, con->arg_ptr + 1);
						con->state = CONSOLE_STATE_NORMAL;
				}
			}
	}

	return pos;
}


//	#pragma mark -


static status_t
console_open(const char *name, uint32 flags, void **cookie)
{
	*cookie = &gconsole;

	status_t status = get_module(gconsole.module_name, (module_info **)&gconsole.module);
	if (status == B_OK)
		gconsole.module->clear(0x0f);

	return status;
}


static status_t
console_freecookie(void *cookie)
{
	if (gconsole.module != NULL) {
		put_module(gconsole.module_name);
		gconsole.module = NULL;
	}

	return B_OK;
}


static status_t
console_close(void *cookie)
{
//	dprintf("console_close: entry\n");

	return 0;
}


static status_t
console_read(void *cookie, off_t pos, void *buffer, size_t *_length)
{
	return B_NOT_ALLOWED;
}


static status_t
console_write(void *cookie, off_t pos, const void *buffer, size_t *_length)
{
	struct console_desc *con = (struct console_desc *)cookie;
	ssize_t written;

	mutex_lock(&con->lock);

	update_cursor(con, -1, -1); // hide it
	written = _console_write(con, buffer, *_length);
	update_cursor(con, con->x, con->y);

	mutex_unlock(&con->lock);

	if (written >= 0) {
		*_length = written;
		return B_OK;
	}
	return written;
}


static status_t
console_ioctl(void *cookie, uint32 op, void *buffer, size_t length)
{
	return B_BAD_VALUE;
}


//	#pragma mark -


status_t
init_hardware(void)
{
	// iterate through the list of console modules until we find one that accepts the job
	void *cookie = open_module_list("console");
	if (cookie == NULL)
		return B_ERROR;

	bool found = false;

	char buffer[B_FILE_NAME_LENGTH];
	size_t bufferSize = sizeof(buffer);

	while (read_next_module_name(cookie, buffer, &bufferSize) == B_OK) {
		dprintf("con_init: trying module %s\n", buffer);
		if (get_module(buffer, (module_info **)&gconsole.module) == B_OK) {
			strlcpy(gconsole.module_name, buffer, sizeof(gconsole.module_name));
			put_module(buffer);
			found = true;
			break;
		}

		bufferSize = sizeof(buffer);
	}

	if (found) {
		// set up the console structure
		mutex_init(&gconsole.lock, "console lock");
		gconsole.module->get_size(&gconsole.columns, &gconsole.lines);

		reset_console(&gconsole);
		gotoxy(&gconsole, 0, 0);
		save_cur(&gconsole, true);
	}

	close_module_list(cookie);
	return found ? B_OK : B_ERROR;
}


const char **
publish_devices(void)
{
	static const char *devices[] = {
		DEVICE_NAME, 
		NULL
	};

	return devices;
}


device_hooks *
find_device(const char *name)
{
	static device_hooks hooks = {
		&console_open,
		&console_close,
		&console_freecookie,
		&console_ioctl,
		&console_read,
		&console_write,
	};

	if (!strcmp(name, DEVICE_NAME))
		return &hooks;

	return NULL;
}


status_t
init_driver(void)
{
	return B_OK;
}


void
uninit_driver(void)
{
}

