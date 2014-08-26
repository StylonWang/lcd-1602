#ifndef __LCDDRIVER_H__
#define __LCDDRIVER_H__

#include <stdint.h>

void lcd_init(int fd);

void lcd_display_string(int fd, char *str, int line);
void lcd_clear(int fd);

#endif // __LCDDDRIVER_H__
