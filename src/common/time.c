#include "../headers/structs.h"
#include "../headers/utils.h"
#include <time.h>
#include <ctype.h>

// inizializza e ritorna un s_time, validando in automatico i dati.
// Accetta 0 <= h < 24 e 0 <= min < 60.
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

// scrive in *now_date e *now_time (se non sono NULL) la data e l'ora correnti,
// prese dallo STESSO istante (cosi' a mezzanotte non si mescolano giorni diversi).
// ritorna 0 se ok, -1 in caso di errore
static int	get_now(t_date *now_date, t_time *now_time)
{
	time_t		now;
	struct tm	*tm_info;

	now = time(NULL); // secondi dal 1/1/1970
	if (now == (time_t)-1)
		return (-1);
	tm_info = localtime(&now); // li scompone in giorno/mese/anno/ora/minuti...
	if (!tm_info)              // NB: punta a un buffer statico, NON va liberato
		return (-1);
	if (now_date)
	{
		now_date->g = tm_info->tm_mday;
		now_date->m = tm_info->tm_mon + 1;     // tm_mon va da 0 a 11
		now_date->a = tm_info->tm_year + 1900; // tm_year = anni dal 1900
	}
	if (now_time)
	{
		now_time->h = tm_info->tm_hour; // 0-23
		now_time->min = tm_info->tm_min; // 0-59
	}
	return (0);
}

// ritorna l'ora corrente (ora locale), NULL in caso di errore
t_time	*init_curr_time(void)
{
	t_time	now;

	if (get_now(NULL, &now) < 0)
		return (NULL);
	return (init_time(now.h, now.min));
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


// Compara due orari.
// ritorna 0 se uguali, valore negativo se t1 < t2, positivo se t1 > t2.
int	timecmp(t_time *t1, t_time *t2)
{
	if (!t1 || !t2)
		return (-1);
	return (time_to_min(t1) - time_to_min(t2));
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

// ritorna la data di oggi (ora locale), NULL in caso di errore
t_date	*init_curr_date(void)
{
	t_date	today;

	if (get_now(&today, NULL) < 0)
		return (NULL);
	return (init_date(today.g, today.m, today.a));
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

// legge un orario "HH:MM" e lo scrive in *time.
// ritorna 0 se ok, -1 se la stringa non e' un orario valido
int	str_to_time(const char *str, t_time *time)
{
	int	h;
	int	min;
	if (!str || !time || strlen(str) != TIME_LEN - 1 || str[2] != ':')
		return (-1);
	h = two_digits(str);
	min = two_digits(str + 3);
	if (h < 0 || h > 23 || min < 0 || min > 59) // -1 = non erano cifre
		return (-1);
	time->h = h;
	time->min = min;
	return (0);
}

// legge una data "GG-MM-AAAA" (es. "20-12-2026") e la scrive in *date,
// controllando che esista davvero.
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
int	datecmp(const t_date *a, const t_date *b)
{
	if (a->a != b->a)
		return (a->a - b->a);
	if (a->m != b->m)
		return (a->m - b->m);
	return (a->g - b->g);
}


void	free_date(t_date *date)
{
	if (!date)
		return;
	free(date);
	date = NULL;
}

// Dice se il momento indicato da date e time e' gia' passato.
//  - solo date (time NULL): passato se date e' prima di oggi (oggi NON e' passato)
//  - solo time (date NULL): passato se time e' prima dell'ora attuale di oggi
//  - entrambi: passato se date e' prima di oggi, oppure se e' oggi e time e'
//    prima dell'ora attuale
// Ritorna 1 se e' passato, 0 se no, -1 in caso di errore (o se sono entrambi NULL)
int	is_past(const t_date *date, const t_time *time)
{
	t_date	today;
	t_time	now;
	int		diff;

	if (!date && !time)
		return (-1);
	if (get_now(&today, &now) < 0) // data e ora prese insieme, dallo stesso istante
		return (-1);
	if (date)
	{
		diff = datecmp(date, &today);
		if (diff != 0 || !time) // giorno diverso da oggi, oppure l'ora non interessa
			return (diff < 0);
	}
	// qui la data e' oggi (o non e' stata data): decide l'ora
	return (time_to_min(time) < time_to_min(&now));
}
