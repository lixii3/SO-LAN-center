#ifndef PROTOCOL_H
#define PROTOCOL_H

// rete
#define SERVER_PORT		1234
#define SERVER_ADDRESS	"127.0.0.1"
#define MAX_LINE		256

// limiti dei campi
#define MAX_USERNAME	32
#define MAX_PASSWORD	32
#define MAX_STATION		16
#define DATE_LEN		11 // GG-MM-AA
#define TIME_LEN		6 // HH:MM


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
#define CMD_CONFILICTS	"CONFLICTS"

// risposte server
#define RESP_OK		"OK"
#define RESP_ERR	"ERR"
#define RESP_BYE	"BYE"

// stati prenotazione
#define STATUS_PENDING		"PENDING"
#define STATUS_APPROVED		"APPROVED"
#define STATUS_REJECTED		"REJECTED"

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