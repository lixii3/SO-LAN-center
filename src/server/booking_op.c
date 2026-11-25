#include "../headers/storage.h"
#include <signal.h>

// --- funzioni di supporto (solo per questo file) -----------------------------

// libera l'array, rilascia il lock e ritorna ret
static int	finish(int fd, t_booking *arr, int ret)
{
	free(arr);
	unlock_file(fd);
	return (ret);
}

// carica tutte le prenotazioni in un array allocato (da liberare con free).
// L'array ha sempre un posto libero in piu' (lo usa storage_add_booking).
// Ritorna NULL in caso di errore.
// DA CHIAMARE CON IL LOCK GIA' PRESO.
static t_booking	*load_all(int fd, int *count)
{
	t_lines		file;
	t_booking	*arr;
	int			i;

	if (storage_read_lines(fd, HEADER_BOOKING, &file) < 0)
		return (NULL);
	arr = calloc(file.count + 1, sizeof(t_booking));
	if (!arr)
	{
		storage_free_lines(&file);
		return (NULL);
	}
	i = 0;
	while (i < file.count)
	{
		if (booking_from_csv(file.lines[i], &arr[i]) < 0)
		{
			fprintf(stderr, "bookings: riga malformata: \"%s\"\n", file.lines[i]);
			storage_free_lines(&file);
			free(arr);
			return (NULL);
		}
		i++;
	}
	*count = file.count;
	storage_free_lines(&file);
	return (arr);
}

// riscrive tutto il file: intestazione + una riga per prenotazione.
// ritorna 0 se ok, -1 in caso di errore. Usata solo da save_all.
static int	write_records(int fd, const t_booking *arr, int count)
{
	char	buf[MAX_LINE];
	int		i;

	if (storage_rewrite(fd, HEADER_BOOKING) < 0)
		return (-1);
	i = 0;
	while (i < count)
	{
		if (booking_to_csv(&arr[i], buf, sizeof(buf)) < 0
			|| storage_write_line(fd, buf) < 0)
			return (-1);
		i++;
	}
	return (0);
}

// salva l'archivio con write_records, rimandando i segnali di terminazione.
// Tra lo svuotamento del file e l'ultima scrittura l'archivio e' incompleto: se
// il processo venisse terminato qui (es. CTRL-C) si perderebbero prenotazioni.
// Un segnale bloccato non va perso: viene consegnato quando si ripristina la
// maschera, cioe' a file completo.
// ritorna 0 se ok, -1 in caso di errore.
// DA CHIAMARE CON IL LOCK IN SCRITTURA GIA' PRESO.
static int	save_all(int fd, const t_booking *arr, int count)
{
	sigset_t	blocked;
	sigset_t	old;
	int			ret;

	sigemptyset(&blocked);
	sigaddset(&blocked, SIGINT);
	sigaddset(&blocked, SIGTERM);
	sigaddset(&blocked, SIGHUP); // chiusura del terminale
	if (sigprocmask(SIG_BLOCK, &blocked, &old) == -1)
		return (-1);
	ret = write_records(fd, arr, count);
	sigprocmask(SIG_SETMASK, &old, NULL);
	return (ret);
}

// posizione nell'array della prenotazione con quell'id, -1 se non c'e'
static int	find_index(const t_booking *arr, int count, int booking_id)
{
	int	i;

	i = 0;
	while (i < count)
	{
		if (arr[i].id == booking_id)
			return (i);
		i++;
	}
	return (-1);
}

// --- operazioni pubbliche ----------------------------------------------------

// aggiunge una prenotazione all'archivio, assegnandole un nuovo id
// (il piu' grande esistente + 1), che viene anche scritto in booking->id.
// Rifiuta la prenotazione se si sovrappone a una gia' APPROVED: in quel caso,
// se conflict_id non e' NULL, ci scrive l'id della prenotazione in conflitto.
// Le sovrapposizioni con prenotazioni PENDING sono permesse: le risolve
// l'admin (vedi storage_set_booking_status).
// ritorna il nuovo id (> 0), STORAGE_CONFLICT oppure STORAGE_ERROR
int	storage_add_booking(int fd, t_booking *booking, int *conflict_id)
{
	t_booking	*arr;
	int			count;
	int			max_id;
	int			i;

	if (!booking)
		return (STORAGE_ERROR);
	if (lock_file(fd, F_WRLCK) < 0)
		return (STORAGE_ERROR);
	arr = load_all(fd, &count);
	if (!arr)
		return (finish(fd, NULL, STORAGE_ERROR));
	booking->id = 0; // non e' ancora nell'archivio
	max_id = 0;
	i = 0;
	while (i < count)
	{
		if (arr[i].id > max_id)
			max_id = arr[i].id;
		if (arr[i].status == STATUS_APPROVED && booking_overlaps(&arr[i], booking))
		{
			if (conflict_id)
				*conflict_id = arr[i].id;
			return (finish(fd, arr, STORAGE_CONFLICT));
		}
		i++;
	}
	booking->id = max_id + 1;
	arr[count] = *booking; // c'e' spazio: load_all lascia sempre un posto libero
	if (save_all(fd, arr, count + 1) < 0)
		return (finish(fd, arr, STORAGE_ERROR));
	return (finish(fd, arr, booking->id));
}

// elimina la prenotazione con quell'id.
// ritorna 0 se ok, STORAGE_NOT_FOUND oppure STORAGE_ERROR
int	storage_remove_booking(int fd, int booking_id)
{
	t_booking	*arr;
	int			count;
	int			idx;

	if (lock_file(fd, F_WRLCK) < 0)
		return (STORAGE_ERROR);
	arr = load_all(fd, &count);
	if (!arr)
		return (finish(fd, NULL, STORAGE_ERROR));
	idx = find_index(arr, count, booking_id);
	if (idx < 0)
		return (finish(fd, arr, STORAGE_NOT_FOUND));
	// sposto indietro di un posto tutte le prenotazioni successive
	memmove(&arr[idx], &arr[idx + 1], (count - idx - 1) * sizeof(t_booking));
	if (save_all(fd, arr, count - 1) < 0)
		return (finish(fd, arr, STORAGE_ERROR));
	return (finish(fd, arr, 0));
}

// restituisce un array con TUTTE le prenotazioni (da liberare con free) e
// scrive in *count quante sono. Il filtro per utente/data/stato lo fa il
// chiamante (es. il comando MY mostra solo quelle dell'utente loggato).
// ritorna NULL in caso di errore
t_booking	*storage_list_bookings(int fd, int *count)
{
	t_booking	*arr;

	if (!count)
		return (NULL);
	if (lock_file(fd, F_RDLCK) < 0)
		return (NULL);
	arr = load_all(fd, count);
	unlock_file(fd);
	return (arr);
}

// copia in *out la prenotazione con quell'id.
// ritorna 0 se trovata, STORAGE_NOT_FOUND oppure STORAGE_ERROR
int	storage_find_booking(int fd, int booking_id, t_booking *out)
{
	t_booking	*arr;
	int			count;
	int			idx;

	if (!out)
		return (STORAGE_ERROR);
	if (lock_file(fd, F_RDLCK) < 0)
		return (STORAGE_ERROR);
	arr = load_all(fd, &count);
	if (!arr)
		return (finish(fd, NULL, STORAGE_ERROR));
	idx = find_index(arr, count, booking_id);
	if (idx < 0)
		return (finish(fd, arr, STORAGE_NOT_FOUND));
	*out = arr[idx];
	return (finish(fd, arr, 0));
}

// cambia lo stato della prenotazione con quell'id.
// Se il nuovo stato e' APPROVED:
//  - fallisce se la prenotazione si sovrappone a un'altra gia' APPROVED
//    (e scrive l'id di quella in *conflict_id, se non e' NULL);
//  - altrimenti rifiuta automaticamente tutte le PENDING in conflitto con essa.
// ritorna il numero di prenotazioni rifiutate automaticamente (>= 0),
// oppure STORAGE_NOT_FOUND, STORAGE_CONFLICT, STORAGE_ERROR
int	storage_set_booking_status(int fd, int booking_id,
	enum e_status new_status, int *conflict_id)
{
	t_booking	*arr;
	int			count;
	int			idx;
	int			rejected;
	int			i;

	if (!status_to_str(new_status))
		return (STORAGE_ERROR);
	if (lock_file(fd, F_WRLCK) < 0)
		return (STORAGE_ERROR);
	arr = load_all(fd, &count);
	if (!arr)
		return (finish(fd, NULL, STORAGE_ERROR));
	idx = find_index(arr, count, booking_id);
	if (idx < 0)
		return (finish(fd, arr, STORAGE_NOT_FOUND));
	arr[idx].status = new_status; // solo in memoria: il file cambia con save_all
	rejected = 0;
	if (new_status == STATUS_APPROVED)
	{
		i = 0;
		while (i < count) // prima passata: c'e' gia' un'approvata in conflitto?
		{
			if (arr[i].status == STATUS_APPROVED && booking_overlaps(&arr[idx], &arr[i]))
			{
				if (conflict_id)
					*conflict_id = arr[i].id;
				return (finish(fd, arr, STORAGE_CONFLICT)); // file non toccato
			}
			i++;
		}
		i = 0;
		while (i < count) // seconda passata: rifiuto le pending in conflitto
		{
			if (arr[i].status == STATUS_PENDING && booking_overlaps(&arr[idx], &arr[i]))
			{
				arr[i].status = STATUS_REJECTED;
				rejected++;
			}
			i++;
		}
	}
	if (save_all(fd, arr, count) < 0)
		return (finish(fd, arr, STORAGE_ERROR));
	return (finish(fd, arr, rejected));
}
