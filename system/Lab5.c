/* Lab5.c - experiment 5 keyboard and VGA console driver */

#include <xinu.h>
#include <kbd.h>

struct k2023202316_kbdvga_cblk k2023202316_kbdvga;

local void k2023202316_vga_ensure_init(void);
local void k2023202316_vga_draw(char, uint8);
local void k2023202316_vga_newline(void);
local void k2023202316_vga_scroll(void);
local void k2023202316_vga_backspace(void);
local void k2023202316_vga_set_cursor(void);
local bool8 k2023202316_vga_handle_ansi(char);
local void k2023202316_vga_apply_ansi(void);
local uint8 k2023202316_kbd_translate(uint8);
local void k2023202316_kbd_put_input(char, uint8);
local void k2023202316_kbd_erase_input(void);
local void k2023202316_kbd_echo(char, uint8);

devcall k2023202316_kbdvgainit(struct dentry *devptr) {
  if (k2023202316_kbdvga.isem <= 0) {
    k2023202316_kbdvga.isem = semcreate(0);
  }
  k2023202316_kbdvga.sem_ready = TRUE;
  k2023202316_kbdvga.ihead = k2023202316_kbdvga.itail =
      &k2023202316_kbdvga.ibuff[0];
  k2023202316_kbdvga.icursor = 0;
  k2023202316_kbdvga.shift = 0;
  k2023202316_vga_clear();

  set_evec(devptr->dvirq, (uint32)devptr->dvintr);
  while (inb(KBSTATP) & KBS_DIB) {
    (void)inb(KBDATAP);
  }
  return OK;
}

devcall k2023202316_kbdvgaread(struct dentry *devptr, void *buf,
                               uint32 count) {
  char *out;
  int32 nread;
  int32 ch;

  if (buf == NULL) {
    return SYSERR;
  }
  if (count == 0) {
    return 0;
  }

  out = (char *)buf;
  ch = k2023202316_kbdgetc(devptr);
  if (ch == EOF) {
    return EOF;
  }
  out[0] = (char)ch;
  nread = 1;
  while ((uint32)nread < count && ch != '\n' && ch != '\r') {
    ch = k2023202316_kbdgetc(devptr);
    out[nread++] = (char)ch;
  }
  return nread;
}

devcall k2023202316_kbdvgawrite(struct dentry *devptr, void *buf,
                                uint32 count) {
  char *str;

  if (buf == NULL && count != 0) {
    return SYSERR;
  }
  str = (char *)buf;
  for (; count > 0; count--) {
    k2023202316_vgaputc(devptr, *str++);
  }
  return OK;
}

devcall k2023202316_kbdgetc(struct dentry *devptr) {
  char ch;

  (void)devptr;
  k2023202316_vga_ensure_init();
  if (!k2023202316_kbdvga.sem_ready) {
    return SYSERR;
  }
  wait(k2023202316_kbdvga.isem);
  ch = *k2023202316_kbdvga.ihead++;
  if (k2023202316_kbdvga.ihead >=
      &k2023202316_kbdvga.ibuff[K2023202316_KBD_IBUFLEN]) {
    k2023202316_kbdvga.ihead = k2023202316_kbdvga.ibuff;
  }
  if (ch == TY_EOFCH) {
    return (devcall)EOF;
  }
  return (devcall)ch;
}

devcall k2023202316_vgaputc(struct dentry *devptr, char ch) {
  (void)devptr;
  return k2023202316_vga_polled_putc((byte)ch);
}

devcall k2023202316_kbdvgactl(struct dentry *devptr, int32 func, int32 arg1,
                              int32 arg2) {
  (void)devptr;
  (void)arg1;
  (void)arg2;

  if (func == TC_ICHARS) {
    return semcount(k2023202316_kbdvga.isem);
  }
  return SYSERR;
}

syscall k2023202316_vga_polled_putc(byte c) {
  k2023202316_vga_ensure_init();

  if (k2023202316_vga_handle_ansi((char)c)) {
    return OK;
  }

  switch (c) {
  case '\r':
    k2023202316_kbdvga.col = 0;
    break;

  case '\n':
    k2023202316_vga_newline();
    break;

  case '\t':
    do {
      k2023202316_vga_draw(' ', k2023202316_kbdvga.attr);
    } while ((k2023202316_kbdvga.col % K2023202316_TAB_WIDTH) != 0);
    break;

  case '\b':
  case '\177':
    k2023202316_vga_backspace();
    break;

  default:
    if (c >= ' ') {
      k2023202316_vga_draw((char)c, k2023202316_kbdvga.attr);
    }
    break;
  }

  k2023202316_vga_set_cursor();
  return OK;
}

syscall k2023202316_kbd_polled_getc(void) {
  intmask mask;
  uint8 ch;

  k2023202316_vga_ensure_init();
  while (TRUE) {
    mask = disable();
    if (inb(KBSTATP) & KBS_DIB) {
      ch = k2023202316_kbd_translate((uint8)inb(KBDATAP));
      restore(mask);
      if (ch != NO) {
        return ch;
      }
    } else {
      restore(mask);
    }
  }
}

void k2023202316_kbdhandler(void) {
  uint8 data;
  uint8 ch;

  if (!k2023202316_kbdvga.sem_ready) {
    return;
  }
  if ((inb(KBSTATP) & KBS_DIB) == 0) {
    return;
  }
  data = (uint8)inb(KBDATAP);
  ch = k2023202316_kbd_translate(data);
  if (ch == NO) {
    return;
  }

  if (ch == '\r') {
    ch = '\n';
  }
  if (ch >= KEY_HOME) {
    return;
  }

  if (ch == '\b' || ch == '\177' || ch == C('H')) {
    k2023202316_kbd_erase_input();
    return;
  }

  if (ch == C('D') && k2023202316_kbdvga.icursor == 0) {
    k2023202316_kbd_put_input((char)TY_EOFCH, 1);
    signal(k2023202316_kbdvga.isem);
    k2023202316_kbdvga.icursor = 0;
    return;
  }

  if (ch == C('U')) {
    while (k2023202316_kbdvga.icursor > 0) {
      k2023202316_kbd_erase_input();
    }
    return;
  }

  if (ch == '\n') {
    k2023202316_kbd_echo('\n', 1);
    k2023202316_kbd_put_input('\n', 1);
    signaln(k2023202316_kbdvga.isem, k2023202316_kbdvga.icursor);
    k2023202316_kbdvga.icursor = 0;
    return;
  }

  if (ch == '\t') {
    uint8 width;
    width = K2023202316_TAB_WIDTH -
            (k2023202316_kbdvga.col % K2023202316_TAB_WIDTH);
    k2023202316_kbd_put_input('\t', width);
    k2023202316_kbd_echo('\t', width);
    return;
  }

  if (ch < ' ') {
    k2023202316_kbd_put_input((char)ch, 2);
    k2023202316_vga_polled_putc('^');
    k2023202316_vga_polled_putc((char)(ch + '@'));
    return;
  }

  k2023202316_kbd_put_input((char)ch, 1);
  k2023202316_kbd_echo((char)ch, 1);
}

void k2023202316_vga_clear(void) {
  int32 i;
  volatile uint16 *vga;

  vga = (volatile uint16 *)K2023202316_VGA_BASE;
  for (i = 0; i < K2023202316_VGA_SIZE; i++) {
    vga[i] = ((uint16)K2023202316_VGA_ATTR << 8) | ' ';
  }
  k2023202316_kbdvga.row = 0;
  k2023202316_kbdvga.col = 0;
  k2023202316_kbdvga.attr = K2023202316_VGA_ATTR;
  k2023202316_kbdvga.ansi_state = 0;
  k2023202316_kbdvga.ansi_len = 0;
  k2023202316_kbdvga.initialized = TRUE;
  k2023202316_vga_set_cursor();
}

local void k2023202316_vga_ensure_init(void) {
  if (k2023202316_kbdvga.initialized) {
    return;
  }
  k2023202316_kbdvga.ihead = k2023202316_kbdvga.itail =
      &k2023202316_kbdvga.ibuff[0];
  k2023202316_vga_clear();
}

local void k2023202316_vga_draw(char ch, uint8 attr) {
  volatile uint16 *vga;

  if (k2023202316_kbdvga.col >= K2023202316_VGA_COLS) {
    k2023202316_vga_newline();
  }
  vga = (volatile uint16 *)K2023202316_VGA_BASE;
  vga[k2023202316_kbdvga.row * K2023202316_VGA_COLS +
      k2023202316_kbdvga.col] = ((uint16)attr << 8) | (uint8)ch;
  k2023202316_kbdvga.col++;
  if (k2023202316_kbdvga.col >= K2023202316_VGA_COLS) {
    k2023202316_vga_newline();
  }
}

local void k2023202316_vga_newline(void) {
  k2023202316_kbdvga.col = 0;
  k2023202316_kbdvga.row++;
  if (k2023202316_kbdvga.row >= K2023202316_VGA_ROWS) {
    k2023202316_vga_scroll();
  }
}

local void k2023202316_vga_scroll(void) {
  int32 i;
  char *ptr;
  uint32 index;
  volatile uint16 *vga;
  uint16 blank;

  vga = (volatile uint16 *)K2023202316_VGA_BASE;
  for (i = 0; i < K2023202316_VGA_SIZE - K2023202316_VGA_COLS; i++) {
    vga[i] = vga[i + K2023202316_VGA_COLS];
  }
  blank = ((uint16)k2023202316_kbdvga.attr << 8) | ' ';
  for (; i < K2023202316_VGA_SIZE; i++) {
    vga[i] = blank;
  }
  k2023202316_kbdvga.row = K2023202316_VGA_ROWS - 1;
  k2023202316_kbdvga.col = 0;
  ptr = k2023202316_kbdvga.ihead;
  while (ptr != k2023202316_kbdvga.itail) {
    index = (uint32)(ptr - k2023202316_kbdvga.ibuff);
    if (k2023202316_kbdvga.irow[index] > 0) {
      k2023202316_kbdvga.irow[index]--;
    }
    ptr++;
    if (ptr >= &k2023202316_kbdvga.ibuff[K2023202316_KBD_IBUFLEN]) {
      ptr = k2023202316_kbdvga.ibuff;
    }
  }
}

local void k2023202316_vga_backspace(void) {
  volatile uint16 *vga;
  int32 pos;

  if (k2023202316_kbdvga.col == 0) {
    if (k2023202316_kbdvga.row == 0) {
      return;
    }
    k2023202316_kbdvga.row--;
    k2023202316_kbdvga.col = K2023202316_VGA_COLS - 1;
  } else {
    k2023202316_kbdvga.col--;
  }
  vga = (volatile uint16 *)K2023202316_VGA_BASE;
  pos = k2023202316_kbdvga.row * K2023202316_VGA_COLS +
        k2023202316_kbdvga.col;
  vga[pos] = ((uint16)k2023202316_kbdvga.attr << 8) | ' ';
}

local void k2023202316_vga_set_cursor(void) {
  uint16 pos;

  pos = (uint16)(k2023202316_kbdvga.row * K2023202316_VGA_COLS +
                 k2023202316_kbdvga.col);
  outb(0x3d4, 0x0f);
  outb(0x3d5, pos & 0xff);
  outb(0x3d4, 0x0e);
  outb(0x3d5, (pos >> 8) & 0xff);
}

local bool8 k2023202316_vga_handle_ansi(char ch) {
  if (k2023202316_kbdvga.ansi_state == 0) {
    if (ch == '\033') {
      k2023202316_kbdvga.ansi_state = 1;
      k2023202316_kbdvga.ansi_len = 0;
      return TRUE;
    }
    return FALSE;
  }

  if (k2023202316_kbdvga.ansi_state == 1) {
    if (ch == '[') {
      k2023202316_kbdvga.ansi_state = 2;
      return TRUE;
    }
    k2023202316_kbdvga.ansi_state = 0;
    return TRUE;
  }

  if (k2023202316_kbdvga.ansi_len < sizeof(k2023202316_kbdvga.ansi_buf) - 1) {
    k2023202316_kbdvga.ansi_buf[k2023202316_kbdvga.ansi_len++] = ch;
  }
  if ((ch >= '@' && ch <= '~') || ch == 'm') {
    k2023202316_kbdvga.ansi_buf[k2023202316_kbdvga.ansi_len] = NULLCH;
    k2023202316_vga_apply_ansi();
    k2023202316_kbdvga.ansi_state = 0;
    k2023202316_kbdvga.ansi_len = 0;
  }
  return TRUE;
}

local void k2023202316_vga_apply_ansi(void) {
  char final;
  int32 i;
  char *buf;

  buf = k2023202316_kbdvga.ansi_buf;
  if (k2023202316_kbdvga.ansi_len == 0) {
    return;
  }
  final = buf[k2023202316_kbdvga.ansi_len - 1];
  if (final == 'J') {
    k2023202316_vga_clear();
    return;
  }
  if (final == 'H') {
    k2023202316_kbdvga.row = 0;
    k2023202316_kbdvga.col = 0;
    k2023202316_vga_set_cursor();
    return;
  }
  if (final != 'm') {
    return;
  }
  for (i = 0; i < k2023202316_kbdvga.ansi_len; i++) {
    if (buf[i] == '0') {
      k2023202316_kbdvga.attr = K2023202316_VGA_ATTR;
    }
    if (buf[i] == '3' && buf[i + 1] == '1') {
      k2023202316_kbdvga.attr = K2023202316_VGA_RED_ATTR;
    }
  }
}

local uint8 k2023202316_kbd_translate(uint8 data) {
  uint8 ch;
  uint8 *map;

  if (data == 0xe0) {
    k2023202316_kbdvga.shift |= E0ESC;
    return NO;
  }
  if (data & 0x80) {
    data = (k2023202316_kbdvga.shift & E0ESC) ? data : (data & 0x7f);
    k2023202316_kbdvga.shift &= ~(shiftcode[data] | E0ESC);
    return NO;
  }

  if (k2023202316_kbdvga.shift & E0ESC) {
    data |= 0x80;
    k2023202316_kbdvga.shift &= ~E0ESC;
  }

  k2023202316_kbdvga.shift |= shiftcode[data];
  k2023202316_kbdvga.shift ^= togglecode[data];
  map = normalmap;
  if (k2023202316_kbdvga.shift & CTL) {
    map = ctlmap;
  } else if (k2023202316_kbdvga.shift & SHIFT) {
    map = shiftmap;
  }
  ch = map[data];
  if (ch == NO) {
    return NO;
  }
  if ((k2023202316_kbdvga.shift & CAPSLOCK) && ch >= 'a' && ch <= 'z') {
    ch += 'A' - 'a';
  } else if ((k2023202316_kbdvga.shift & CAPSLOCK) && ch >= 'A' &&
             ch <= 'Z') {
    ch += 'a' - 'A';
  }
  return ch;
}

local void k2023202316_kbd_put_input(char ch, uint8 width) {
  char *next;
  uint32 index;

  next = k2023202316_kbdvga.itail + 1;
  if (next >= &k2023202316_kbdvga.ibuff[K2023202316_KBD_IBUFLEN]) {
    next = k2023202316_kbdvga.ibuff;
  }
  if (next == k2023202316_kbdvga.ihead) {
    k2023202316_vga_polled_putc(TY_BELL);
    return;
  }
  *k2023202316_kbdvga.itail = ch;
  index = (uint32)(k2023202316_kbdvga.itail - k2023202316_kbdvga.ibuff);
  k2023202316_kbdvga.iwid[index] = width;
  k2023202316_kbdvga.irow[index] = (uint8)k2023202316_kbdvga.row;
  k2023202316_kbdvga.icol[index] = (uint8)k2023202316_kbdvga.col;
  k2023202316_kbdvga.itail = next;
  k2023202316_kbdvga.icursor++;
}

local void k2023202316_kbd_erase_input(void) {
  volatile uint16 *vga;
  uint32 index;
  uint8 width;
  int32 start;
  int32 end;
  int32 i;

  if (k2023202316_kbdvga.icursor <= 0) {
    return;
  }
  if (k2023202316_kbdvga.itail == k2023202316_kbdvga.ibuff) {
    k2023202316_kbdvga.itail =
        &k2023202316_kbdvga.ibuff[K2023202316_KBD_IBUFLEN - 1];
  } else {
    k2023202316_kbdvga.itail--;
  }
  index = (uint32)(k2023202316_kbdvga.itail - k2023202316_kbdvga.ibuff);
  width = k2023202316_kbdvga.iwid[index];
  if (width == 0) {
    width = 1;
  }
  k2023202316_kbdvga.row = k2023202316_kbdvga.irow[index];
  k2023202316_kbdvga.col = k2023202316_kbdvga.icol[index];
  start = k2023202316_kbdvga.row * K2023202316_VGA_COLS +
          k2023202316_kbdvga.col;
  end = start + width;
  if (end > K2023202316_VGA_SIZE) {
    end = K2023202316_VGA_SIZE;
  }
  vga = (volatile uint16 *)K2023202316_VGA_BASE;
  for (i = start; i < end; i++) {
    vga[i] = ((uint16)k2023202316_kbdvga.attr << 8) | ' ';
  }
  k2023202316_vga_set_cursor();
  k2023202316_kbdvga.icursor--;
}

local void k2023202316_kbd_echo(char ch, uint8 width) {
  uint8 i;

  (void)width;
  if (ch == '\t') {
    k2023202316_vga_polled_putc('\t');
    return;
  }
  for (i = 0; i < width; i++) {
    k2023202316_vga_polled_putc(ch);
  }
}
