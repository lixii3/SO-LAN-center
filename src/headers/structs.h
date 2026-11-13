#include "./protocol.h"

// --- time ---

// tempo in formato formato HH:MIN
typedef struct s_time
{
	int	h;
	int	min;
} t_time;

t_time	*init_time(int h, int min);
char	*time_to_str(t_time *time);
int		is_within_time_limits(t_time *time, int h, int min);
void	free_time(t_time *date);

// data in formato GG:MM:AA
typedef struct s_date
{
	int	g;
	int	m;
	int	a;
} t_date;

t_date	*init_date(int g, int m, int a);
t_date	*init_curr_date(void);
char	*date_to_str(t_date *date);
void	free_date(t_date *date);


// --- booking ---
typedef struct s_booking
{
	int				id;
	char			station[MAX_STATION];
	char			username[MAX_USERNAME];
	char			date[DATE_LEN];
	t_time			*start_time;
	t_time			*end_time;
	enum e_status	status;
}t_booking;


t_booking	*init_booking(const char station, const char username,
			const char date, t_time start_time, t_time end_time,
			enum e_status status);
int			storage_add(t_booking booking, int fd);
int			storage_remove(int booking_id, int fd);
t_booking	**storage_list(int fd);
t_booking	*storage_find(int fd);
int			storage_set_status(int booking_id,
			enum e_status new_status, int fd);
