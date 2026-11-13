#ifndef STORAGE_H
#define STORAGE_H

#include "./structs.h"

/*
** Archivio delle prenotazioni (solo lato server).
**
** Il file e' un CSV: una riga di intestazione e poi una prenotazione per riga
**   id,postazione,richiedente,data,ora_inizio,ora_fine,stato
**   17,PC-03,mario,20-12-2026,18:00,21:00,PENDING
**
** CONCORRENZA: ogni funzione prende un lock sul file con fcntl() per tutta
** la durata dell'operazione (lock in scrittura per le modifiche, in lettura
** per le consultazioni). Cosi' piu' processi figli del server possono usare
** lo stesso file senza corrompere i dati e senza che due prenotazioni in
** conflitto vengano accettate "contemporaneamente".
**
** Ogni processo deve usare un proprio fd (aperto con storage_open dopo la fork).
*/

#define STORAGE_HEADER		"id,postazione,richiedente,data,ora_inizio,ora_fine,stato"

// valori di ritorno (sempre negativi, per distinguerli da id/conteggi validi)
#define STORAGE_ERROR		-1	// errore di I/O, memoria o file malformato
#define STORAGE_NOT_FOUND	-2	// nessuna prenotazione con quell'id
#define STORAGE_CONFLICT	-3	// sovrapposizione con una prenotazione APPROVED

int			storage_open(const char *path);
int			storage_add(int fd, t_booking *booking, int *conflict_id);
int			storage_remove(int fd, int booking_id);
t_booking	*storage_list(int fd, int *count);
int			storage_find(int fd, int booking_id, t_booking *out);
int			storage_set_status(int fd, int booking_id,
				enum e_status new_status, int *conflict_id);

#endif
