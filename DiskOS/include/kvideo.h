#ifndef KVIDEO
#define	KVIDEO

void cls (void);
void putchar (int c);
void printf (const char *format, ...);
void itoa (char *buf, int base, int d);

// FG colors
#define FG_COLOR_RESET            "\033[7m"

#define FG_COLOR_BLACK            "\033[30m"
#define FG_COLOR_DARK_RED         "\033[31m"
#define FG_COLOR_DARK_GREEN       "\033[32m"
#define FG_COLOR_ORANGE           "\033[33m"
#define FG_COLOR_DARK_BLUE        "\033[34m"
#define FG_COLOR_DARK_PINK        "\033[35m"
#define FG_COLOR_DARK_CYAN        "\033[36m"
#define FG_COLOR_LIGHT_GREY       "\033[37m"

#define FG_COLOR_DARK_GREY        "\033[90m"
#define FG_COLOR_LIGHT_RED        "\033[91m"
#define FG_COLOR_LIGHT_GREEN      "\033[92m"
#define FG_COLOR_YELLOW           "\033[93m"
#define FG_COLOR_LIGHT_BLUE       "\033[94m"
#define FG_COLOR_LIGHT_PINK       "\033[95m"
#define FG_COLOR_LIGHT_CYAN       "\033[96m"
#define FG_COLOR_WHITE            "\033[97m"


// BG COLORS
#define BG_COLOR_BLACK            "\033[40m"
#define BG_COLOR_DARK_RED         "\033[41m"
#define BG_COLOR_DARK_GREEN       "\033[42m"
#define BG_COLOR_ORANGE           "\033[43m"
#define BG_COLOR_DARK_BLUE        "\033[44m"
#define BG_COLOR_DARK_PINK        "\033[45m"
#define BG_COLOR_DARK_CYAN        "\033[46m"
#define BG_COLOR_LIGHT_GREY       "\033[47m"

#define BG_COLOR_DARK_GREY        "\033[100m"
#define BG_COLOR_LIGHT_RED        "\033[101m"
#define BG_COLOR_LIGHT_GREEN      "\033[102m"
#define BG_COLOR_YELLOW           "\033[103m"
#define BG_COLOR_LIGHT_BLUE       "\033[104m"
#define BG_COLOR_LIGHT_PINK       "\033[105m"
#define BG_COLOR_LIGHT_CYAN       "\033[106m"
#define BG_COLOR_WHITE            "\033[107m"

// BG colors
#define COLOR_RESET               "\033[40;97m"

#endif // KVIDEO
