#include "../headers/structs.h"
#include "../headers/utils.h"
#include <time.h>

// inizializza e ritorna un s_time, validando in automatico i dati
t_time	*init_time(int h, int min)
{
	t_time *time;

	if (!((h <= 23 && h >= 0) && (min < 60 && min >= 0)))
		return (NULL);
	time = calloc(sizeof(t_time), 1);
	if (!time)
		return (NULL);
	time->h = h;
	time->min = min;
	return (time);
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

// restituisce il tempo in formato HH:MIN
char	*time_to_str(t_time *time)
{
	char	*str;

	if (!time || !str)
		return (NULL);
	str = calloc(sizeof(char), TIME_LEN);
	snprintf(str, TIME_LEN, "%02d:%02d", time->h, time->min);
	return str;
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
	
	date = calloc(sizeof(t_date), 1);
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

char	*date_to_str(t_date *date)
{
	char	*str;

	if (!date || !str)
		return (NULL);
	str = calloc(sizeof(char), DATE_LEN);
	snprintf(str, DATE_LEN, "%02d/%02d/%04d", date->g, date->m, date->a);
	return str;
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

/*
int main(void)
{
	test_time();
	test_date();
}
*/

