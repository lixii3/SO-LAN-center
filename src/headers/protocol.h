#ifndef PROTOCOL_H
#define PROTOCOL_H

// --- ICP ---
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// rete
#define SERVER_PORT		1234
#define SERVER_ADDRESS	"127.0.0.1"

// limiti dei campi
enum	e_limits
{
	MAX_LINE = 256, // max len linea di comando
	MAX_USERNAME = 32, // max len username
	MAX_PASSWORD = 32, // max len password
	MAX_STATION	 = 16, //max len id postazione
	DATE_LEN = 11, // GG-MM-AAAA (10 caratteri + '\0')
	TIME_LEN = 6, // HH:MM (5 caratteri + '\0')
	OPENING_H = 10, //ora di apertura
	CLOSING_H = 23, //ora di chiusura
	MIN_DURATION = 30, // durata minima di una prenotazione (minuti)
	MAX_DURATION = 240, // durata massima di una prenotazione (minuti, 4 ore)
};

// scaglioni orari permessi
enum e_booking_minutes
{
	THIRTY = 30,
	ZERO = 0
};

// stati prenotazione
enum e_status
{
	STATUS_PENDING = 'P',
	STATUS_APPROVED = 'A',
	STATUS_REJECTED = 'R'
};

// ruoli
#define ROLE_USER "USER"
#define ROLE_ADMIN "ADMIN"

// comandi
#define CMD_LOGIN		"LOGIN"
#define CMD_LOGOUT		"LOGOUT"
#define CMD_BOOK		"BOOK"
#define CMD_STATIONS	"STATIONS"
#define CMD_QUIT		"QUIT"
#define CMD_MY			"MY"

// comandi admin
#define CMD_ALL			"ALL"
#define CMD_APPROVE		"APPROVE"
#define CMD_REJECT		"REJECT"
#define CMD_SET			"SET"
#define CMD_CONFLICTS	"CONFLICTS"

// risposte server
#define RESP_OK		"OK"
#define RESP_ERR	"ERR"
#define RESP_BYE	"BYE"

// codici di errore
#define ERR_UNKNOWN_COMMAND		"UNKNOWN_COMMAND"
#define ERR_SYNTAX				"SYNTAX"
#define ERR_LINE_TOO_LONG		"LINE_TOO_LONG"
#define ERR_NOT_LOGGED			"NOT_LOGGED"
#define ERR_ALREADY_LOGGED		"ALREADY_LOGGED"
#define ERR_BAD_CREDENTIALS		"BAD_CREDENTIALS"
#define ERR_TOO_MANY_ATTEMPTS	"TOO_MANY_ATTEMPTS"
#define ERR_UNAUTHORIZED		"UNAUTHORIZED"
#define ERR_NO_SUCH_STATION		"NO_SUCH_STATION"
#define ERR_INVALID_DATE		"INVALID_DATE"
#define ERR_INVALID_TIME		"INVALID_TIME"
#define ERR_INVALID_STATUS		"INVALID_STATUS"
#define ERR_NOT_FOUND			"NOT_FOUND"
#define ERR_CONFLICT			"CONFLICT"
#define ERR_SERVER				"SERVER"

#endif