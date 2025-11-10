// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stddef.h>
#include <stdint.h>

void *memset(void *destination, int32_t c, uint64_t length) {
	uint8_t chr = (uint8_t) c;
	char *dst = (char *) destination;

	while (length--)
		dst[length] = chr;

	return destination;
}

void *memcpy(void *destination, const void *source, uint64_t length) {
	uint64_t i;

	if ((uint64_t) destination % sizeof(uint32_t) == 0 && (uint64_t) source % sizeof(uint32_t) == 0 &&
		length % sizeof(uint32_t) == 0) {
		uint32_t *d = (uint32_t *) destination;
		const uint32_t *s = (const uint32_t *) source;

		for (i = 0; i < length / sizeof(uint32_t); i++)
			d[i] = s[i];
	}
	else {
		uint8_t *d = (uint8_t *) destination;
		const uint8_t *s = (const uint8_t *) source;

		for (i = 0; i < length; i++)
			d[i] = s[i];
	}

	return destination;
}

size_t strlen(const char *s) {
	size_t len = 0;
	if (!s)
		return 0;
	while (s[len] != '\0') {
		len++;
	}
	return len;
}
