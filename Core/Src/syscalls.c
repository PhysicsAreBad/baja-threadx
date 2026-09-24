/*
 * Minimal newlib system call stubs.
 *
 * printf()/puts() end up in _write(). To send stdout to a UART, implement
 * __io_putchar() somewhere else (e.g. with HAL_UART_Transmit); by default the
 * output is thrown away.
 */
#include <errno.h>
#include <sys/stat.h>

int __io_putchar(int ch) __attribute__((weak));
int __io_putchar(int ch)
{
    return ch;
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        __io_putchar(ptr[i]);
    }
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    errno = ENOSYS;
    return -1;
}

int _close(int file)
{
    (void)file;
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}
