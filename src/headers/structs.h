#ifndef STRUCTS_H
#define STRUCTS_H

#include "./protocol.h"
#include <stddef.h>

// --- time ---

// tempo in formato formato HH:MIN
typedef struct s_time
{
	int	h;
	int	min;
} t_time;

t_time	*init_time(int h, int min);
t_time	*init_curr_time(void);
char	*time_to_str(const t_time *time, char *buf);
int		str_to_time(const char *str, t_time *time);
int		time_to_min(const t_time *time);
int		is_within_time_limits(t_time *time, int h, int min);
int	timecmp(t_time *t1, t_time *t2);
void	free_time(t_time *time);

// --- date ---

// data formato GG-MM-AAAA)
typedef struct s_date
{
	int	g;
	int	m;
	int	a;
} t_date;

t_date	*init_date(int g, int m, int a);
t_date	*init_curr_date(void);
char	*date_to_str(const t_date *date, char *buf);
int		str_to_date(const char *str, t_date *date);
int		datecmp(const t_date *a, const t_date *b);
void	free_date(t_date *date);
int		is_past(const t_date *date, const t_time *time);


// --- booking ---

typedef struct s_booking
{
	int				id;
	char			station[MAX_STATION];
	char			username[MAX_USERNAME];
	t_date			date;
	t_time			start_time;
	t_time			end_time;
	enum e_status	status;
}	t_booking;

t_booking	*init_booking(const char *station, const char *username,
				const t_date *date, const t_time *start_time,
				const t_time *end_time, enum e_status status);
const char	*status_to_str(enum e_status status);
int			str_to_status(const char *str, enum e_status *status);
int			booking_overlaps(const t_booking *a, const t_booking *b);
int			booking_to_csv(const t_booking *booking, char *buf, size_t size);
int			booking_from_csv(const char *line, t_booking *booking);

#endif
