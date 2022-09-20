/* See LICENSE for copyright and license details. */

#define CURSOR_BUF_SIZE 16 /* Used for cursor movement directives */
#define UTF8_BYTES 4

enum {
	VT_DEF,
	VT_CHR,
	VT_BKSPC,
	VT_DLT,
	VT_EOF,
	VT_RET,
	VT_TAB,
	VT_UP,
	VT_DWN,
	VT_LFT,
	VT_RGHT,
	VT_HOME,
	VT_END
};

void vt100_ln_write(char *buf, size_t size, const char *src);
void vt100_ln_new(void);

void vt100_cur_mov_left(char *buf);
void vt100_cur_mov_right(char *buf);
void vt100_cur_mov_home(void);
void vt100_cur_mov_end(char *buf);
size_t vt100_cur_get_end_pos(char *buf);

void vt100_utf8_delete(char *buf, size_t size, int bsmode);
void vt100_utf8_insert(char *buf, size_t size, const char *utf8);

int vt100_read_key(char *utf8);
