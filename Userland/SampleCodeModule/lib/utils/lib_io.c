// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../../include/lib.h"
#include "../../include/syscall.h"
#include <stdarg.h>
#include <stddef.h>

// ============================================================================
// BASIC I/O FUNCTIONS
// ============================================================================

int putchar(int c) {
	char ch = (char) c;
	sys_write(1, &ch, 1);
	return c;
}

int getchar(void) {
	char c;
	int n;

	do {
		n = sys_read(0, &c, 1);
	} while (n == 0);

	return (int) c;
}

// Función interna para leer líneas (no requiere FILE*)
static char *read_line_internal(char *s, int n) {
	int i = 0;
	char c;

	// Validar parámetros
	if (n <= 0 || s == NULL) {
		return NULL;
	}

	while (i < n - 1) {
		int read = sys_read(0, &c, 1); // stdin siempre es fd 0

		// Si read retorna error o EOF, terminar
		if (read <= 0) {
			break;
		}

		s[i++] = c;
		if (c == '\n') {
			break;
		}
	}

	// Si no se leyó nada, retornar NULL
	if (i == 0) {
		return NULL;
	}

	s[i] = '\0';
	return s;
}

char *fgets(char *s, int n, FILE *stream) {
	// En bare metal, ignoramos el parámetro stream
	// y leemos directamente de stdin (fd 0)
	(void) stream; // Suprimir warning de parámetro no usado
	return read_line_internal(s, n);
}

void printHex64(uint64_t value) {
	char hex[17];
	for (int i = 15; i >= 0; i--) {
		int digit = value & 0xF;
		hex[i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
		value >>= 4;
	}
	hex[16] = '\0';
	printf("0x%s", hex);
}

// ============================================================================
// SCANF IMPLEMENTATION
// ============================================================================

// Forward declaration needed by scanf
extern int atoi(const char *str);

int scanf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char input[BUFFER_SIZE];
	// Usar función interna que no requiere FILE*
	if (!read_line_internal(input, BUFFER_SIZE)) {
		va_end(args);
		return 0;
	}

	int index = 0;
	int count = 0;

	for (int i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] == ' ' || fmt[i] == '\t' || fmt[i] == '\n') {
			while (input[index] == ' ' || input[index] == '\t') {
				index++;
			}
			continue;
		}

		if (input[index] == '\n' || input[index] == '\0') {
			break;
		}

		if (fmt[i] == '%') {
			i++;
			if (fmt[i] == '[' && fmt[i + 1] == '^' && fmt[i + 2] == '\\' && fmt[i + 3] == 'n' && fmt[i + 4] == ']') {
				i += 4;
				char *dest = va_arg(args, char *);
				int start = index;
				while (input[index] != '\0' && input[index] != '\n') {
					index++;
				}
				int len = index - start;
				if (len > 0) {
					for (int j = 0; j < len; j++) {
						dest[j] = input[start + j];
					}
					dest[len] = '\0';
					count++;
				}
				continue;
			}

			switch (fmt[i]) {
				case 's': {
					char *dest = va_arg(args, char *);
					int start = index;
					while (input[index] != '\0' && input[index] != '\n') {
						index++;
					}
					int len = index - start;
					if (len > 0) {
						for (int j = 0; j < len; j++) {
							dest[j] = input[start + j];
						}
						dest[len] = '\0';
						count++;
					}
					break;
				}
				case 'd': {
					int *dest = va_arg(args, int *);

					while (input[index] == ' ' || input[index] == '\t') {
						index++;
					}

					int start = index;

					if (input[index] == '-' || input[index] == '+') {
						index++;
					}

					if (input[index] < '0' || input[index] > '9') {
						break;
					}

					while (input[index] >= '0' && input[index] <= '9') {
						index++;
					}

					int len = index - start;
					if (len > 0) {
						char temp[32];
						if (len >= (int) sizeof(temp))
							len = sizeof(temp) - 1;

						for (int j = 0; j < len; j++) {
							temp[j] = input[start + j];
						}
						temp[len] = '\0';

						*dest = atoi(temp);
						count++;
					}

					while (input[index] == ' ' || input[index] == '\t') {
						index++;
					}
					break;
				}
				case 'c': {
					char *dest = va_arg(args, char *);
					// En este punto, ya verificamos que input[index] no es '\n' ni '\0'
					*dest = input[index];
					index++;
					count++;
					break;
				}
				default:
					break;
			}
		}
		else {
			if (input[index] == fmt[i]) {
				index++;
			}
			else {
				break;
			}
		}

		if (input[index] == '\n' || input[index] == '\0') {
			break;
		}
	}

	va_end(args);
	return count;
}

// ============================================================================
// PRINTF IMPLEMENTATION
// ============================================================================

// Forward declaration needed by printf
extern size_t strlen(const char *s);

int printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int count = 0;

	for (int i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] == '%') {
			i++;
			switch (fmt[i]) {
				case 's': {
					char *s = va_arg(args, char *);
					while (*s) {
						putchar(*s++);
						count++;
					}
					break;
				}
				case 'd': {
					int n = va_arg(args, int);
					if (n == 0) {
						putchar('0');
						count++;
						break;
					}

					if (n < 0) {
						putchar('-');
						n = -n;
						count++;
					}

					char buf[12];
					int i = 0;

					while (n > 0) {
						buf[i++] = (n % 10) + '0';
						n /= 10;
					}

					while (i--) {
						putchar(buf[i]);
						count++;
					}

					break;
				}
				case 'c': {
					char c = (char) va_arg(args, int);
					putchar(c);
					count++;
					break;
				}
				case 'l':
					if (fmt[i + 1] == 'l') {
						if (fmt[i + 2] == 'x') {
							// %llx - hexadecimal
							unsigned long long v = va_arg(args, unsigned long long);
							char buf[17];
							buf[16] = '\0';
							for (int j = 15; j >= 0; j--) {
								buf[j] = "0123456789ABCDEF"[v & 0xF];
								v >>= 4;
							}
							char *p = buf;
							while (*p == '0' && *(p + 1))
								p++;
							printf("%s", p);
							count += strlen(p);
							i += 2;
							break;
						}
						else if (fmt[i + 2] == 'd') {
							// %lld - long long decimal con signo
							long long v = va_arg(args, long long);
							int64_t val = (int64_t) v;
							if (val == 0) {
								putchar('0');
								count++;
							}
							else {
								char buf[21];
								int buf_i = 0;
								uint64_t uval;

								if (val < 0) {
									putchar('-');
									count++;
									// Evitar overflow en INT64_MIN: usar conversión directa
									// Si val == INT64_MIN, -val causaría overflow
									if (val == -9223372036854775807LL - 1LL) {
										uval = 9223372036854775808ULL;
									}
									else {
										uval = (uint64_t) (-val);
									}
								}
								else {
									uval = (uint64_t) val;
								}

								while (uval > 0) {
									buf[buf_i++] = (uval % 10) + '0';
									uval /= 10;
								}
								while (buf_i--) {
									putchar(buf[buf_i]);
									count++;
								}
							}
							i += 2;
							break;
						}
						else if (fmt[i + 2] == 'u') {
							// %llu - long long decimal sin signo
							unsigned long long v = va_arg(args, unsigned long long);
							uint64_t val = (uint64_t) v;
							if (val == 0) {
								putchar('0');
								count++;
							}
							else {
								char buf[21];
								int buf_i = 0;
								while (val > 0) {
									buf[buf_i++] = (val % 10) + '0';
									val /= 10;
								}
								while (buf_i--) {
									putchar(buf[buf_i]);
									count++;
								}
							}
							i += 2;
							break;
						}
					}
					// Si es solo 'l' sin 'l' adicional, tratarlo como caso default
					// fallthrough
				default:
					putchar('%');
					putchar(fmt[i]);
					count += 2;
					break;
			}
		}
		else {
			putchar(fmt[i]);
			count++;
		}
	}

	va_end(args);
	return count;
}

// ============================================================================
// SPRINTF IMPLEMENTATION
// ============================================================================

static int append_unsigned_to_str(char *dest, int *index, uint64_t value) {
	if (value == 0) {
		dest[(*index)++] = '0';
		return 1;
	}

	char buf[21];
	int len = 0;
	while (value > 0) {
		buf[len++] = (char) ('0' + (value % 10));
		value /= 10;
	}

	for (int i = len - 1; i >= 0; i--) {
		dest[(*index)++] = buf[i];
	}
	return len;
}

static int append_signed_to_str(char *dest, int *index, long long value) {
	int written = 0;
	uint64_t magnitude;
	if (value < 0) {
		dest[(*index)++] = '-';
		written++;
		magnitude = (uint64_t) (-(value + 1)) + 1;
	}
	else {
		magnitude = (uint64_t) value;
	}
	written += append_unsigned_to_str(dest, index, magnitude);
	return written;
}

int sprintf(char *str, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int count = 0;
	int str_index = 0;

	for (int i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] != '%') {
			str[str_index++] = fmt[i];
			count++;
			continue;
		}

		i++;
		if (fmt[i] == '\0') {
			break;
		}

		if (fmt[i] == '%') {
			str[str_index++] = '%';
			count++;
			continue;
		}

		int long_modifiers = 0;
		while (fmt[i] == 'l') {
			long_modifiers++;
			i++;
		}

		char spec = fmt[i];
		switch (spec) {
			case 's': {
				char *s = va_arg(args, char *);
				while (s && *s) {
					str[str_index++] = *s++;
					count++;
				}
				break;
			}
			case 'c': {
				char c = (char) va_arg(args, int);
				str[str_index++] = c;
				count++;
				break;
			}
			case 'd': {
				if (long_modifiers >= 2) {
					long long val = va_arg(args, long long);
					count += append_signed_to_str(str, &str_index, val);
				}
				else if (long_modifiers == 1) {
					long val = va_arg(args, long);
					count += append_signed_to_str(str, &str_index, val);
				}
				else {
					int val = va_arg(args, int);
					count += append_signed_to_str(str, &str_index, val);
				}
				break;
			}
			case 'u': {
				if (long_modifiers >= 2) {
					unsigned long long val = va_arg(args, unsigned long long);
					count += append_unsigned_to_str(str, &str_index, val);
				}
				else if (long_modifiers == 1) {
					unsigned long val = va_arg(args, unsigned long);
					count += append_unsigned_to_str(str, &str_index, val);
				}
				else {
					unsigned int val = va_arg(args, unsigned int);
					count += append_unsigned_to_str(str, &str_index, val);
				}
				break;
			}
			default:
				str[str_index++] = '%';
				count++;
				if (fmt[i] != '\0') {
					str[str_index++] = fmt[i];
					count++;
				}
				break;
		}
	}

	str[str_index] = '\0';
	va_end(args);
	return count;
}
