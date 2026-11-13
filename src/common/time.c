#include "../headers/structs.h"
#include "../headers/utils.h"
#include <time.h>
#include <ctype.h>

// inizializza e ritorna un s_time, validando in automatico i dati
t_time	*init_time(int h, int min)
{
	t_time *time;

	if (!((h <= 23 && h >= 0) && (min < 60 && min >= 0)))
		return (NULL);
	time = calloc(1, sizeof(t_time));
	if (!time)
		return (NULL);
	time->h = h;
	time->min = min;
	return (time);
}

// converte un orario in minuti dalla mezzanotte (18:30 -> 1110):
// semplifica il confronto tra orari
int	time_to_min(const t_time *time)
{
	return (time->h * 60 + time->min);
}

// ritorna 0 se time supera il limite dato (o se time è null), 1 altrimenti
int		is_within_time_limits(t_time *time, int h, int min)
{
	if (!time)
		return (0);
	return (time->h * 60 + time->min <= h * 60 + min);
}

// libera il blocco allocato per time usando free()
void	free_time(t_time *time)
{
	if (!time)
		return;
	free(time);
}

// scrive in buf (almeno TIME_LEN byte) il tempo in formato HH:MM e ritorna buf
char	*time_to_str(const t_time *time, char *buf)
{
	if (!time)
		return (NULL);
	snprintf(buf, TIME_LEN, "%02d:%02d", time->h, time->min);
	return buf;
}


static int is_leap_year(int year)
{
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// validazione dei dati in d
static int	is_valid_date(t_date *d)
{
	int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (d->a < 1 || d->m < 1 || d->m > 12 || d->g < 1)
		return (0);
	if (d->m == 2 && is_leap_year(d->a))
		days_in_month[2] = 29;
	return (d->g <= days_in_month[d->m]);
}

t_date	*init_date(int g, int m, int a)
{
	t_date	*date;
	
	date = calloc(1, sizeof(t_date));
	if (!date)
		return (NULL);
	date->g = g;
	date->m = m;
	date->a = a;
	if (is_valid_date(date))
		return (date);
	free(date);
	return (NULL);
}

// scrive in buf (almeno DATE_LEN byte) la data in formato GG-MM-AAAA e ritorna buf
char	*date_to_str(const t_date *date, char *buf)
{
	if (!date)
		return (NULL);
	snprintf(buf, DATE_LEN, "%02d-%02d-%04d", date->g, date->m, date->a);
	return buf;
}

// converte due cifre consecutive in un numero ("18" -> 18), -1 se non sono cifre
static int	two_digits(const char *s)
{
	if (!isdigit((unsigned char)s[0]) || !isdigit((unsigned char)s[1]))
		return (-1);
	return ((s[0] - '0') * 10 + (s[1] - '0'));
}

// legge un orario "HH:MM" (es. "18:30") e lo scrive in *time.
// Accetta da 00:00 a 23:59, piu' 24:00 (mezzanotte, usata come ora di fine).
// ritorna 0 se ok, -1 se la stringa non e' un orario valido
int	str_to_time(const char *str, t_time *time)
{
	int	h;
	int	min;

	// "HH:MM" e' lungo esattamente TIME_LEN - 1 caratteri, con ':' in posizione 2
	if (!str || !time || strlen(str) != TIME_LEN - 1 || str[2] != ':')
		return (-1);
	h = two_digits(str);
	min = two_digits(str + 3);
	if (h < 0 || min < 0 || min > 59 || h > 24 || (h == 24 && min != 0))
		return (-1);
	time->h = h;
	time->min = min;
	return (0);
}

// legge una data "GG-MM-AAAA" (es. "20-12-2026") e la scrive in *date,
// controllando che esista davvero (no 30 febbraio, anni bisestili...).
// ritorna 0 se ok, -1 se la stringa non e' una data valida
int	str_to_date(const char *str, t_date *date)
{
	t_date	d;
	int		century;
	int		year;

	// "GG-MM-AAAA" e' lungo esattamente DATE_LEN - 1 caratteri, '-' in posizione 2 e 5
	if (!str || !date || strlen(str) != DATE_LEN - 1 || str[2] != '-' || str[5] != '-')
		return (-1);
	d.g = two_digits(str);
	d.m = two_digits(str + 3);
	century = two_digits(str + 6); // "2026" -> 20 e 26
	year = two_digits(str + 8);
	if (d.g < 0 || d.m < 0 || century < 0 || year < 0)
		return (-1);
	d.a = century * 100 + year;
	if (!is_valid_date(&d))
		return (-1);
	*date = d;
	return (0);
}

// confronta due date: < 0 se a viene prima di b, 0 se sono uguali, > 0 se dopo
int	date_cmp(const t_date *a, const t_date *b)
{
	if (a->a != b->a)
		return (a->a - b->a);
	if (a->m != b->m)
		return (a->m - b->m);
	return (a->g - b->g);
}

// ritorna la data di oggi (ora locale), NULL in caso di errore
t_date	*init_curr_date(void)
{
	time_t		now;
	struct tm	*tm_info;
	t_date		*today;

	now = time(NULL); // secondi dal 1/1/1970
	if (now == (time_t)-1)
		return (NULL);
	tm_info = localtime(&now); // li scompone in giorno/mese/anno...
	if (!tm_info)
		return (NULL);
	today = calloc(1, sizeof(t_date));
	if (!today)
		return (NULL);
	today->g = tm_info->tm_mday;
	today->m = tm_info->tm_mon + 1; // tm_mon va da 0-11
	today->a = tm_info->tm_year + 1900; // tm_year anni dal 1900
	return (today);
}


void	free_date(t_date *date)
{
	if (!date)
		return;
	free(date);
	date = NULL;
}







/*
static void	test_time(void)
{
	t_time *time;
	char	*s;
	int		h = 11;
	int		min = 31;

	printf("\n--- TIME TEST ---\n");
	time = init_time(9, 45);
	if (!time)
	{
		perror("UPS!");
		return;
	}
	s = time_to_str(time);
	printf("Is %s within %i:%i ? %i\n", s, h, min, is_within_time_limits(time, h, min));
	free(time);
	free(s);
	return;
}

static void	test_date(void)
{
	t_date	*date;
	char	*s;

	printf("\n--- DATE TEST ---\n");
	date = init_date(9, 8, 2005);
	if (!date)
	{
		perror("UPS!");
		return;
	}
	s = date_to_str(date);
	printf("%s\n", s);
	free(date);
	free(s);
	date = init_curr_date();
	s = date_to_str(date);
	printf("%s\n", s);
	free(date);
	free(s);
	return;
}

int main(void)
{
	test_time();
	test_date();
}
*/

