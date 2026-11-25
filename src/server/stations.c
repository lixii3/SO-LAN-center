#include "../headers/storage.h"

// campi di una riga di stations_db.csv: id,tipo
enum e_station_field
{
	ST_ID,
	ST_TYPE,
	ST_FIELDS // numero di campi (vale 2)
};

// legge tutte le righe del file delle postazioni (da liberare con
// storage_free_lines). Il file viene chiuso subito: i dati sono in memoria.
// ritorna 0 se ok, -1 in caso di errore
int	storage_read_stations(t_lines *out)
{
	int	fd;
	int	ret;

	fd = storage_open(STATIONS_PATH, HEADER_STATIONS);
	if (fd < 0)
		return (-1);
	ret = lock_file(fd, F_RDLCK);
	if (ret == 0)
		ret = storage_read_lines(fd, HEADER_STATIONS, out);
	close(fd); // chiudere il file rilascia anche il lock
	return (ret);
}

// trasforma la riga CSV "PC-01,PC" in "STATION PC-01 PC"
// ritorna 0 se ok, -1 se la riga e' malformata
int	station_row(const char *csv, char *row, size_t size)
{
	char	copy[MAX_LINE];
	char	*fields[ST_FIELDS];

	if (copy_str(copy, csv, sizeof(copy)) < 0
		|| split_fields(copy, ',', fields, ST_FIELDS) != ST_FIELDS
		|| fields[ST_ID][0] == '\0' || fields[ST_TYPE][0] == '\0')
		return (-1);
	if (snprintf(row, size, "STATION %s %s", fields[ST_ID], fields[ST_TYPE]) >= (int)size)
		return (-1);
	return (0);
}

// cerca la postazione con quell'id (es. "PC-01") nel file delle postazioni.
// ritorna 0 se esiste, STORAGE_NOT_FOUND oppure STORAGE_ERROR
int	storage_find_station(const char *id)
{
	t_lines	file;
	size_t	len;
	int		ret;
	int		i;

	if (!id || storage_read_stations(&file) < 0)
		return (STORAGE_ERROR);
	len = strlen(id);
	ret = STORAGE_NOT_FOUND;
	i = 0;
	while (i < file.count && ret == STORAGE_NOT_FOUND)
	{
		// la riga deve iniziare con "<id>,": senza la virgola "PC-0"
		// risulterebbe uguale a "PC-01"
		if (strncmp(file.lines[i], id, len) == 0 && file.lines[i][len] == ',')
			ret = 0;
		i++;
	}
	storage_free_lines(&file);
	return (ret);
}
