#include "../headers/structs.h"
#include "../headers/utils.h"



t_booking	*init_booking(const char station, const char username,
	const char date, t_time start_time, t_time end_time,
	enum e_status status)
{
	t_booking	*booking;

	booking = calloc(sizeof(t_booking), 1);
	if (!booking)
		return (NULL);
	
}