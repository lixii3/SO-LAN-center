#ifndef STORAGE_H
#define STORAGE_H

#include "./structs.h"
#include "./utils.h"
#include <errno.h>
#include <sys/stat.h>

/*
** Archivio su file CSV (solo lato server).
**
** Ogni file e' un CSV: una riga di intestazione e poi un record per riga.
**   bookings: id,postazione,richiedente,data,ora_inizio,ora_fine,stato
**             17,PC-03,mario,20-12-2026,18:00,21:00,PENDING
**   users:    username,password,role
**             lia,1234,admin
**
** Struttura:
**   storage.c     -> funzioni generiche, valide per QUALSIASI file CSV:
**                    apertura, lock, lettura riga per riga, riscrittura
**   booking_op.c  -> operazioni sul file delle prenotazioni
**   user_op.c     -> operazioni sul file degli utenti
**   stations.c    -> operazioni sul file delle postazioni
**
** Ogni operazione prende un lock sul file con fcntl() per tutta la sua durata
** (in scrittura per le modifiche, in lettura per le consultazioni). Cosi' piu'
** processi figli del server possono usare lo stesso file senza corrompere i
** dati e senza accettare "contemporaneamente" due prenotazioni in conflitto.
**
** Ogni processo deve usare un proprio fd (aperto con storage_open DOPO la fork).
*/

// percorsi relativi alla cartella da cui si lancia il server (la root del progetto)
#define BOOKINGS_PATH	"src/data/bookings_db.csv"
#define USERS_PATH		"src/data/users_db.csv"
#define STATIONS_PATH	"src/data/stations_db.csv"

#define HEADER_BOOKING	"id,postazione,richiedente,data,ora_inizio,ora_fine,stato"
#define HEADER_USERS	"username,password,role"
#define HEADER_STATIONS	"id,tipo"

// valori di ritorno (sempre negativi, per distinguerli da id/conteggi validi)
#define STORAGE_ERROR		-1	// errore di I/O, memoria o file malformato
#define STORAGE_NOT_FOUND	-2	// record inesistente (o credenziali sbagliate)
#define STORAGE_CONFLICT	-3	// sovrapposizione con una prenotazione APPROVED

// --- storage.c: funzioni generiche sui file CSV -----------------------------

// righe di un file CSV, gia' senza intestazione, righe vuote e '\r'.
// lines[i] punta dentro buf: libera con storage_free_lines
typedef struct s_lines
{
	char	*buf;
	char	**lines;
	int		count;
}	t_lines;

int			storage_open(const char *path, const char *header);
int			lock_file(int fd, short type);
void		unlock_file(int fd);
int			storage_read_lines(int fd, const char *header, t_lines *out);
void		storage_free_lines(t_lines *l);
int			storage_rewrite(int fd, const char *header);
int			storage_write_line(int fd, const char *line);

// --- booking_op.c: operazioni su bookings ------------------------------------

int			storage_add_booking(int fd, t_booking *booking, int *conflict_id);
int			storage_remove_booking(int fd, int booking_id);
t_booking	*storage_list_bookings(int fd, int *count);
int			storage_find_booking(int fd, int booking_id, t_booking *out);
int			storage_set_booking_status(int fd, int booking_id,
				enum e_status new_status, int *conflict_id);

// --- stations.c --------------------------------------------------------------
int			station_row(const char *csv, char *row, size_t size);
int			storage_read_stations(t_lines *out);
int			storage_find_station(const char *id);

// --- user_op.c: operazioni su users ------------------------------------------

typedef struct s_user
{
	char	username[MAX_USERNAME];
	char	password[MAX_PASSWORD];
	int		is_admin;
}	t_user;

int			storage_find_user(int fd, const char *username, t_user *out);
int			storage_check_credentials(int fd, const char *username,
				const char *pass, int *is_admin);
void		clear_user(t_user *user);
#endif