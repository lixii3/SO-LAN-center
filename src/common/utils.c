#include "../headers/utils.h"

// comleta str a sx con il carattere scelto se strlen(str) è minore di target_len
void pad_left(char *str, size_t target_len, char fill_char)
{
	size_t len = strlen(str);

	// Se la stringa è già lunga abbastanza, non facciamo nulla
	if (len >= target_len) {
		return;
	}

	size_t pad_len = target_len - len;

	// Spostiamo la stringa originale a destra.
	memmove(str + pad_len, str, len + 1);

	// Riempiamo lo spazio vuoto a sinistra con il carattere scelto
	memset(str, fill_char, pad_len);
}