#include "../headers/structs.h"
#include "../headers/utils.h"

// crea una prenotazione (da liberare con free). L'id vale 0: lo assegna
// storage_add_booking (booking_op.c) al momento del salvataggio.
// ritorna NULL se un campo e' NULL o troppo lungo, o se fine <= inizio.
// NB: qui si controlla solo la coerenza dei dati, le regole del LAN center
// (orari di apertura, durata massima, data non passata...) le controlla il server.
t_booking	*init_booking(const char *station, const char *username,
	const t_date *date, const t_time *start_time, const t_time *end_time,
	enum e_status status)
{
	t_booking	*booking;

	if (!date || !start_time || !end_time)
		return (NULL);
	if (time_to_min(end_time) <= time_to_min(start_time))
		return (NULL);
	if (!status_to_str(status))
		return (NULL);
	booking = calloc(1, sizeof(t_booking));
	if (!booking)
		return (NULL);
	if (copy_str(booking->station, station, MAX_STATION) < 0
		|| copy_str(booking->username, username, MAX_USERNAME) < 0)
	{
		free(booking);
		return (NULL);
	}
	booking->id = 0;
	booking->date = *date; // copio le struct (non i puntatori)
	booking->start_time = *start_time;
	booking->end_time = *end_time;
	booking->status = status;
	return (booking);
}

// enum -> stringa del protocollo ("PENDING", ...). NULL se lo stato non esiste
const char	*status_to_str(enum e_status status)
{
	if (status == STATUS_PENDING)
		return ("PENDING");
	if (status == STATUS_APPROVED)
		return ("APPROVED");
	if (status == STATUS_REJECTED)
		return ("REJECTED");
	return (NULL);
}

// ritorna 0 se ok, -1 se la stringa non e' valida
int	str_to_status(const char *str, enum e_status *status)
{
	if (!str || !status)
		return (-1);
	if (strcmp(str, "PENDING") == 0)
		*status = STATUS_PENDING;
	else if (strcmp(str, "APPROVED") == 0)
		*status = STATUS_APPROVED;
	else if (strcmp(str, "REJECTED") == 0)
		*status = STATUS_REJECTED;
	else
		return (-1);
	return (0);
}

// 1 se a e b sono in conflitto, 0 altrimenti.
// Sono in conflitto due prenotazioni diverse, non rifiutate, per la stessa
// postazione nello stesso giorno, i cui intervalli si sovrappongono.
// Es: 17-18 e 18-19 NON si sovrappongono.
int	booking_overlaps(const t_booking *a, const t_booking *b)
{
	if (a->id == b->id && a->id != 0)
		return (0);
	if (a->status == STATUS_REJECTED || b->status == STATUS_REJECTED)
		return (0);
	if (strcmp(a->station, b->station) != 0 || datecmp(&a->date, &b->date) != 0)
		return (0);
	return (time_to_min(&a->start_time) < time_to_min(&b->end_time)
		&& time_to_min(&b->start_time) < time_to_min(&a->end_time));
}

// scrive in buf la riga CSV della prenotazione (senza '\n'), convertendo
// data e orari in testo:
//   17,PC-03,mario,20-12-2026,18:00,21:00,PENDING
// ritorna la lunghezza della riga, -1 se buf e' troppo piccolo o lo stato e' invalido
int	booking_to_csv(const t_booking *booking, char *buf, size_t size)
{
	const char	*status;
	char		date[DATE_LEN];
	char		start[TIME_LEN];
	char		end[TIME_LEN];
	int			len;

	status = status_to_str(booking->status);
	if (!status)
		return (-1);
	len = snprintf(buf, size, "%d,%s,%s,%s,%s,%s,%s",
			booking->id, booking->station, booking->username,
			date_to_str(&booking->date, date),
			time_to_str(&booking->start_time, start),
			time_to_str(&booking->end_time, end), status);
	if (len < 0 || (size_t)len >= size) // snprintf ha dovuto troncare
		return (-1);
	return (len);
}

// campi di una riga CSV dei bookings
enum e_csv_field
{
	CSV_ID,
	CSV_STATION,
	CSV_USERNAME,
	CSV_DATE,
	CSV_START,
	CSV_END,
	CSV_STATUS,
	CSV_FIELDS // numero di campi (vale 7)
};

// legge una riga CSV (senza '\n') e riempie booking, convertendo e validando ogni campo
// di testo nel suo tipo
// ritorna 0 se ok, -1 se la riga e' malformata
int	booking_from_csv(const char *line, t_booking *booking)
{
	char	copy[MAX_LINE];
	char	*fields[CSV_FIELDS];

	// split_fields modifica la stringa: lavoro su una copia
	if (copy_str(copy, line, MAX_LINE) < 0)
		return (-1);
	if (split_fields(copy, ',', fields, CSV_FIELDS) != CSV_FIELDS)
		return (-1);
	if (str_to_int(fields[CSV_ID], &booking->id) < 0
		|| copy_str(booking->station, fields[CSV_STATION], MAX_STATION) < 0
		|| copy_str(booking->username, fields[CSV_USERNAME], MAX_USERNAME) < 0
		|| str_to_date(fields[CSV_DATE], &booking->date) < 0
		|| str_to_time(fields[CSV_START], &booking->start_time) < 0
		|| str_to_time(fields[CSV_END], &booking->end_time) < 0
		|| str_to_status(fields[CSV_STATUS], &booking->status) < 0)
		return (-1);
	return (0);
}
