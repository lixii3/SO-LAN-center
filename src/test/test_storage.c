/*
** Test dell'archivio delle prenotazioni (senza rete).
** Uso: make tests
*/
#include "../headers/storage.h"
#include "../headers/utils.h"
#include <fcntl.h>
#include <sys/wait.h>

#define TEST_FILE	"test_bookings.csv"

static int	g_failed = 0;

// stampa OK/FAIL per ogni controllo e conta i fallimenti
static void	check(int condition, const char *description)
{
	printf("[%s] %s\n", condition ? " OK " : "FAIL", description);
	if (!condition)
		g_failed++;
}

// crea una prenotazione PENDING (abbreviazione per i test)
// date nel formato del protocollo: "GG-MM-AAAA"
static t_booking	make(const char *station, const char *user, const char *date,
	int h1, int m1, int h2, int m2)
{
	t_booking	*tmp;
	t_booking	b;
	t_date		d;
	t_time		start;
	t_time		end;

	str_to_date(date, &d);
	start.h = h1;
	start.min = m1;
	end.h = h2;
	end.min = m2;
	tmp = init_booking(station, user, &d, &start, &end, STATUS_PENDING);
	b = *tmp; // copio la struct e libero quella allocata
	free(tmp);
	return (b);
}

static void	test_conversions(void)
{
	t_date	d;
	t_date	d2;
	t_time	t;
	char	buf[DATE_LEN];
	int		n;

	printf("\n--- conversioni (time.c, utils.c) ---\n");
	check(str_to_date("20-12-2026", &d) == 0 && d.g == 20 && d.m == 12 && d.a == 2026,
		"str_to_date legge 20-12-2026");
	check(strcmp(date_to_str(&d, buf), "20-12-2026") == 0, "date_to_str riscrive 20-12-2026");
	check(str_to_date("29-02-2026", &d2) < 0, "29-02-2026 rifiutata (2026 non bisestile)");
	check(str_to_date("29-02-2028", &d2) == 0, "29-02-2028 accettata (bisestile)");
	check(str_to_date("2026-12-20", &d2) < 0, "formato AAAA-MM-GG rifiutato");
	check(str_to_date("20-12-26", &d2) < 0, "anno a 2 cifre rifiutato");
	str_to_date("01-01-2027", &d2);
	check(date_cmp(&d, &d2) < 0, "date_cmp: 20-12-2026 viene prima di 01-01-2027");
	check(str_to_time("08:30", &t) == 0 && t.h == 8 && t.min == 30, "str_to_time legge 08:30");
	check(str_to_time("24:00", &t) == 0, "24:00 accettato (mezzanotte come fine)");
	check(str_to_time("24:30", &t) < 0, "24:30 rifiutato");
	check(str_to_time("18:60", &t) < 0, "18:60 rifiutato");
	check(str_to_time("8:30", &t) < 0, "8:30 (una cifra) rifiutato");
	check(str_to_int("17", &n) == 0 && n == 17, "str_to_int legge 17");
	check(str_to_int("17abc", &n) < 0 && str_to_int("", &n) < 0 && str_to_int("-3", &n) < 0,
		"str_to_int rifiuta 17abc, stringa vuota e -3");
}

static void	test_booking_functions(void)
{
	t_booking	a;
	t_booking	b;
	t_booking	parsed;
	t_time		t1 = {18, 0};
	t_time		t2 = {17, 0};
	t_date		d = {20, 12, 2026};
	char		buf[MAX_LINE];

	printf("\n--- booking.c ---\n");
	check(init_booking("PC-01", "mario", &d, &t1, &t2, STATUS_PENDING) == NULL,
		"init_booking rifiuta fine <= inizio");
	check(init_booking("UN-NOME-DI-POSTAZIONE-TROPPO-LUNGO", "mario", &d,
		&t2, &t1, STATUS_PENDING) == NULL, "init_booking rifiuta campi troppo lunghi");

	a = make("PC-01", "mario", "20-12-2026", 17, 0, 18, 0);
	b = make("PC-01", "luigi", "20-12-2026", 18, 0, 19, 0);
	check(!booking_overlaps(&a, &b), "17-18 e 18-19 NON sono in conflitto");
	b = make("PC-01", "luigi", "20-12-2026", 17, 30, 19, 0);
	check(booking_overlaps(&a, &b), "17-18 e 17:30-19 sono in conflitto");
	b = make("PC-02", "luigi", "20-12-2026", 17, 30, 19, 0);
	check(!booking_overlaps(&a, &b), "postazioni diverse non sono in conflitto");
	b = make("PC-01", "luigi", "21-12-2026", 17, 30, 19, 0);
	check(!booking_overlaps(&a, &b), "giorni diversi non sono in conflitto");
	b = make("PC-01", "luigi", "20-12-2026", 17, 30, 19, 0);
	b.status = STATUS_REJECTED;
	check(!booking_overlaps(&a, &b), "le prenotazioni REJECTED non contano");

	a.id = 7;
	booking_to_csv(&a, buf, sizeof(buf));
	check(strcmp(buf, "7,PC-01,mario,20-12-2026,17:00,18:00,PENDING") == 0,
		"booking_to_csv produce la riga attesa");
	check(booking_from_csv(buf, &parsed) == 0 && parsed.id == 7
		&& strcmp(parsed.username, "mario") == 0 && parsed.end_time.h == 18,
		"booking_from_csv rilegge la stessa prenotazione");
	check(parsed.date.g == 20 && parsed.date.m == 12 && parsed.date.a == 2026
		&& parsed.start_time.h == 17 && parsed.start_time.min == 0,
		"booking_from_csv converte data e orari in t_date / t_time");
	check(booking_from_csv("7,PC-01,mario", &parsed) < 0,
		"booking_from_csv rifiuta una riga incompleta");
	check(booking_from_csv("7,PC-01,mario,20-12-2026,17:00,18:00,BOH", &parsed) < 0,
		"booking_from_csv rifiuta uno stato sconosciuto");
	check(booking_from_csv("7,PC-01,mario,30-02-2026,17:00,18:00,PENDING", &parsed) < 0,
		"booking_from_csv rifiuta una data inesistente");
	check(booking_from_csv("7,PC-01,mario,20-12-2026,17:00,18:00,PENDING,extra", &parsed) < 0,
		"booking_from_csv rifiuta una riga con troppi campi");
	check(booking_from_csv("7,UNA-POSTAZIONE-DAL-NOME-LUNGHISSIMO,mario,20-12-2026,17:00,18:00,PENDING",
		&parsed) < 0, "booking_from_csv rifiuta una postazione troppo lunga");
}

static void	test_storage_functions(void)
{
	int			fd;
	int			id1;
	int			id2;
	int			id3;
	int			conflict;
	int			count;
	t_booking	b;
	t_booking	found;
	t_booking	*all;

	printf("\n--- storage.c ---\n");
	unlink(TEST_FILE);
	fd = storage_open(TEST_FILE);
	check(fd >= 0, "storage_open crea il file");

	all = storage_list(fd, &count);
	check(all != NULL && count == 0, "archivio nuovo: 0 prenotazioni");
	free(all);

	b = make("PC-03", "mario", "20-12-2026", 18, 0, 21, 0);
	id1 = storage_add(fd, &b, NULL);
	b = make("PC-03", "luigi", "20-12-2026", 20, 0, 22, 0);
	id2 = storage_add(fd, &b, NULL);
	b = make("PS5-1", "peach", "20-12-2026", 20, 0, 22, 0);
	id3 = storage_add(fd, &b, NULL);
	check(id1 == 1 && id2 == 2 && id3 == 3, "storage_add assegna id 1, 2, 3");

	check(storage_find(fd, 2, &found) == 0 && strcmp(found.username, "luigi") == 0,
		"storage_find trova la prenotazione 2");
	check(storage_find(fd, 99, &found) == STORAGE_NOT_FOUND,
		"storage_find: id inesistente -> NOT_FOUND");

	check(storage_set_status(fd, 1, STATUS_APPROVED, NULL) == 1,
		"approvare la 1 rifiuta automaticamente la 2 (in conflitto)");
	storage_find(fd, 2, &found);
	check(found.status == STATUS_REJECTED, "la 2 ora e' REJECTED");
	storage_find(fd, 3, &found);
	check(found.status == STATUS_PENDING, "la 3 (altra postazione) resta PENDING");

	b = make("PC-03", "toad", "20-12-2026", 19, 0, 20, 0);
	conflict = 0;
	check(storage_add(fd, &b, &conflict) == STORAGE_CONFLICT && conflict == 1,
		"storage_add rifiuta una sovrapposizione con la APPROVED 1");

	conflict = 0;
	check(storage_set_status(fd, 2, STATUS_APPROVED, &conflict) == STORAGE_CONFLICT
		&& conflict == 1, "non si puo' approvare la 2: confligge con la 1");

	check(storage_remove(fd, 3) == 0, "storage_remove elimina la 3");
	check(storage_remove(fd, 3) == STORAGE_NOT_FOUND, "rimuoverla di nuovo -> NOT_FOUND");
	close(fd);

	fd = storage_open(TEST_FILE); // riapro: i dati devono essere ancora li'
	all = storage_list(fd, &count);
	check(all != NULL && count == 2 && all[0].status == STATUS_APPROVED,
		"dopo la riapertura il file contiene i dati giusti");
	free(all);
	close(fd);
}

// N processi aggiungono una prenotazione nello stesso istante:
// alla fine devono esserci N prenotazioni con N id tutti diversi.
// Senza lock alcuni processi leggerebbero lo stesso "id massimo" e
// si sovrascriverebbero a vicenda.
static void	test_concurrency(void)
{
	int			n = 20;
	int			i;
	int			fd;
	int			count;
	int			ok;
	int			j;
	t_booking	*all;
	t_booking	b;

	printf("\n--- concorrenza (%d processi) ---\n", n);
	unlink(TEST_FILE);
	fd = storage_open(TEST_FILE);
	close(fd);
	// svuoto il buffer di printf PRIMA della fork: altrimenti ogni figlio
	// ne eredita una copia e la ristampa quando termina
	fflush(stdout);
	i = 0;
	while (i < n)
	{
		if (fork() == 0)
		{
			fd = storage_open(TEST_FILE); // ogni figlio ha il suo fd
			b = make("PC-01", "user", "20-12-2026", 18, 0, 19, 0);
			storage_add(fd, &b, NULL);
			close(fd);
			_exit(0); // _exit non svuota i buffer di stdio (ereditati dal padre)
		}
		i++;
	}
	while (wait(NULL) > 0) // aspetto tutti i figli
		;
	fd = storage_open(TEST_FILE);
	all = storage_list(fd, &count);
	check(all != NULL && count == n, "tutte le prenotazioni sono state salvate");
	ok = (all != NULL);
	i = 0;
	while (ok && i < count)
	{
		j = i + 1;
		while (j < count)
			if (all[i].id == all[j++].id)
				ok = 0;
		i++;
	}
	check(ok, "gli id sono tutti diversi");
	free(all);

	fflush(stdout);
	// 2 processi approvano contemporaneamente due prenotazioni in conflitto
	// tra loro: al massimo una delle due puo' risultare APPROVED
	i = 0;
	while (i < 2)
	{
		if (fork() == 0)
		{
			fd = storage_open(TEST_FILE);
			storage_set_status(fd, i + 1, STATUS_APPROVED, NULL);
			close(fd);
			_exit(0); // _exit non svuota i buffer di stdio (ereditati dal padre)
		}
		i++;
	}
	while (wait(NULL) > 0)
		;
	all = storage_list(fd, &count);
	j = 0;
	i = 0;
	while (all && i < count)
		if (all[i++].status == STATUS_APPROVED)
			j++;
	check(j == 1, "approvazioni simultanee: esattamente una APPROVED");
	free(all);
	close(fd);
	unlink(TEST_FILE);
}

int	main(void)
{
	test_conversions();
	test_booking_functions();
	test_storage_functions();
	test_concurrency();
	printf("\n%s (%d falliti)\n", g_failed ? "CI SONO ERRORI" : "TUTTO OK", g_failed);
	return (g_failed != 0);
}
