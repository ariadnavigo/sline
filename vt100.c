/* See LICENSE for copyright and license details. */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "strlcpy.h"
#include "vt100.h"

static int esc_seq(char *seq);
static char *buf_slice(char *src, int pivot, size_t size);
static void ln_redraw(const char *str, size_t nbytes);
static int utf8_nbytes(const char *utf8);
static int utf8_nbytes_r(const char *utf8);

static size_t pos, buf_i;

/* Auxiliary subroutines */

static int
esc_seq(char *seq)
{
	if (read(STDIN_FILENO, &seq[0], 1) != 1)
		return -1;
	if (read(STDIN_FILENO, &seq[1], 1) != 1)
		return -1;

	if (seq[0] != '[')
		return -1;

	if (seq[1] >= '0' && seq[1] <= '9') {
		if (read(STDIN_FILENO, &seq[2], 1) != 1)
			return -1;
	}

	return 0;
}

static char *
buf_slice(char *src, int pivot, size_t size)
{
	char *suff, *ptr;
	size_t ptr_size;

	ptr = src + pivot;
	ptr_size = strlen(ptr) + 1;

	if ((suff = calloc(size, sizeof(char))) == NULL)
		return NULL;

	strlcpy(suff, ptr, ptr_size);
	memset(ptr, 0, ptr_size);

	return suff;
}

static void
ln_redraw(const char *str, size_t nbytes)
{
	write(STDOUT_FILENO, "\x1b[0K", 4);
	write(STDOUT_FILENO, "\x1b", 1);
	write(STDOUT_FILENO, "7", 1); /* ESC 7: portable save cursor */
	write(STDOUT_FILENO, str, nbytes);
	write(STDOUT_FILENO, "\x1b", 1);
	write(STDOUT_FILENO, "8", 1); /* ESC 8: portable restore cursor */
}

static int
utf8_nbytes(const char *utf8)
{
	if (utf8[0] >= '\xc0' && utf8[0] <= '\xdf')
		return 2;
	else if (utf8[0] >= '\xe0' && utf8[0] <= '\xef')
		return 3;
	else if (utf8[0] >= '\xf0' && utf8[0] <= '\xf7')
		return 4;

	return 1;
}

static int
utf8_nbytes_r(const char *utf8)
{
	const char *ptr;
	int nbytes;

	for (ptr = utf8; (nbytes = utf8_nbytes(ptr)) == 1; --ptr) {
		/* We stop if we're an ASCII char */
		if ((unsigned char)ptr[0] <= '\x7f')
			return 1;
	}

	return nbytes;
}

/* VT100 driver API */

void
vt100_ln_write(char *buf, size_t size, const char *src)
{
	size_t buf_len;

	memset(buf, 0, size);
	strlcpy(buf, src, size);
	buf_len = strlen(buf);

	vt100_cur_mov_home();
	write(STDOUT_FILENO, "\x1b[0K", 4);
	write(STDOUT_FILENO, buf, buf_len);
	
	buf_i = buf_len;
	pos = vt100_cur_get_end_pos(buf);
}

void
vt100_ln_new(void)
{
	write(STDOUT_FILENO, "\n", 1);
	pos = 0;
	buf_i = 0;
}

size_t
vt100_cur_get_end_pos(char *buf)
{
	int i;
	size_t end_pos;

	i = 0;
	end_pos = 0;
	while (buf[i] != '\0') {
		++end_pos;
		i += utf8_nbytes(&buf[i]);
	}

	return end_pos;
}

void
vt100_cur_mov_left(char *buf)
{
	int nbytes;

	if (pos == 0)
		return;

	nbytes = utf8_nbytes_r(&buf[buf_i - 1]);
	buf_i -= nbytes;

	write(STDOUT_FILENO, "\x1b[D", 3);
	--pos;
}

void
vt100_cur_mov_right(char *buf)
{
	int nbytes;

	if (pos == vt100_cur_get_end_pos(buf))
		return;

	nbytes = utf8_nbytes(&buf[buf_i]);
	buf_i += nbytes;

	write(STDOUT_FILENO, "\x1b[C", 3);
	++pos;
}

void
vt100_cur_mov_home(void)
{
	char cmd[CURSOR_BUF_SIZE];

	if (pos == 0)
		return;

	snprintf(cmd, CURSOR_BUF_SIZE, "\x1b[%zdD", pos);
	write(STDOUT_FILENO, cmd, strlen(cmd));
	pos = 0;
	buf_i = 0;
}

void
vt100_cur_mov_end(char *buf)
{
	size_t end_pos;
	char cmd[CURSOR_BUF_SIZE];

	if (buf[buf_i] == '\0')
		return;

	end_pos = vt100_cur_get_end_pos(buf);
	snprintf(cmd, CURSOR_BUF_SIZE, "\x1b[%zdC", end_pos - pos);
	write(STDOUT_FILENO, cmd, strlen(cmd));
	pos = end_pos;
	
	buf_i = strlen(buf);
}

void
vt100_utf8_delete(char *buf, size_t size, int bsmode)
{
	char *suff, *suff_new;
	int nbytes;
	size_t len;

	if (bsmode > 0) {
		if (buf_i == 0)
			return;

		nbytes = utf8_nbytes_r(&buf[buf_i - 1]);
		buf_i -= nbytes;
	} else {
		nbytes = utf8_nbytes_r(&buf[buf_i]);
	}

	if ((suff = buf_slice(buf, buf_i, size)) == NULL)
		return;

	suff_new = suff + nbytes; /* Deleting character from suff; way safer */
	len = strlen(suff_new);
	strlcpy(&buf[strlen(buf)], suff_new, len + 1);

	if (bsmode > 0) {
		write(STDOUT_FILENO, "\b", 1);
		--pos;
	}
	ln_redraw(suff_new, len);

	free(suff);
}

void
vt100_utf8_insert(char *buf, size_t size, const char *utf8)
{
	char *suff;
	size_t len;
	int i, nbytes;

	if (pos >= size)
		return;

	if ((suff = buf_slice(buf, buf_i, size)) == NULL)
		return;

	len = strlen(suff);
	nbytes = utf8_nbytes(utf8);
	for (i = 0; i < nbytes; ++i)
		buf[strlen(buf)] = utf8[i];

	strlcpy(&buf[strlen(buf)], suff, len + 1);
	buf_i += nbytes;

	write(STDOUT_FILENO, utf8, nbytes);
	++pos;
	ln_redraw(suff, len);

	free(suff);
}

int
vt100_read_key(char *utf8)
{
	char key;
	char seq[3];
	int nread, nbytes;

	while ((nread = read(STDIN_FILENO, &key, 1)) != 1) {
		if (nread == -1)
			return -1;
	}

	if (key == '\x1b') {
		if (esc_seq(seq) < 0)
			return VT_DEF;

		if (seq[1] == '3' && seq[2] == '~')
			return VT_DLT;

		if ((seq[1] == '1' || seq[1] == '7') && seq[2] == '~')
			return VT_HOME;

		if ((seq[1] == '4' || seq[1] == '8') && seq[2] == '~')
			return VT_END;

		switch (seq[1]) {
		case 'A':
			return VT_UP;
		case 'B':
			return VT_DWN;
		case 'C':
			return VT_RGHT;
		case 'D':
			return VT_LFT;
		case 'H':
			return VT_HOME;
		case 'F':
			return VT_END;
		default:
			return VT_DEF;
		}
	} else if (key == '\x7f') {
		return VT_BKSPC;
	} else if (key == '\x04') {
		return VT_EOF;
	} else if (key == '\x0a') {
		return VT_RET;
	} else if (key == '\t') {
		return VT_TAB;
	} else if ((nbytes = utf8_nbytes(&key)) > 1) {
		utf8[0] = key;
		read(STDIN_FILENO, utf8 + 1, nbytes - 1);
		return VT_CHR;
	} else {
		utf8[0] = key;
		return VT_CHR;
	}
}
