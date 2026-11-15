# Protocollo LAN Center — versione 0.1

Questo documento descrive i messaggi scambiati tra **client** e **server** del
sistema di prenotazione delle postazioni (PC e console) del LAN center.
È il "contratto" tra i due programmi: se una cosa non è scritta qui, il client
non deve aspettarsela e il server non deve inviarla.

---

## 1. Regole generali

- La comunicazione avviene su socket **TCP** (`AF_INET`, `SOCK_STREAM`).
  Porta di default: **12345**.
- Ogni messaggio è **una riga di testo** terminata da `\n`.
  Lunghezza massima di una riga: **256 byte** (compreso `\n`).
- I campi sono separati da **un singolo spazio**. Nessun campo può contenere spazi
  (username, password e nomi delle postazioni non hanno spazi).
- I comandi si scrivono in **MAIUSCOLO** (`LOGIN`, `BOOK`, ...).
- Il dialogo è sempre **richiesta → risposta**: il client invia un comando,
  il server risponde, e solo allora il client può inviare il comando successivo.
  Il server non invia mai messaggi spontaneamente.

### Formato delle risposte

Ogni risposta inizia con `OK` oppure `ERR`.

```
OK [dati...]
ERR <CODICE> [messaggio]
```

- Il client deve guardare solo `ERR <CODICE>`. Il messaggio dopo il codice
  serve a chi fa debug e può cambiare.
- **Risposte con elenchi**: la prima riga è `OK <n>`, dove `n` è il numero
  di righe che seguono. Il client legge esattamente `n` righe in più.
  Se `n` vale 0, non segue nessuna riga.

### Formati dei dati

| Dato        | Formato        | Esempio      | Note |
|-------------|----------------|--------------|------|
| data        | `GG-MM-AAAA`   | `20-12-2026` | le date si confrontano convertendole in `t_date` (non con `strcmp`, che metterebbe il 20-12-2026 dopo l'01-01-2027) |
| ora         | `HH:MM`        | `18:30`      | minuti solo `00` o `30`, da `00:00` a `23:59` (`24:00` non e' valido) |
| postazione  | `TIPO-NN`      | `PC-03`      | tipi: `PC`, `PS5`, `XBOX`, `SWITCH` |
| stato       | parola         | `PENDING`    | `PENDING`, `APPROVED`, `REJECTED` |
| ID          | intero > 0     | `17`         | assegnato dal server |

### Regole di prenotazione

- Orario di apertura: **10:00 – 23:00**.
- Durata: minimo **30 minuti**, massimo **4 ore**, a multipli di 30 minuti.
- Non si può prenotare nel passato.
- Due prenotazioni sono **in conflitto** se riguardano la stessa postazione nella
  stessa data e gli intervalli si sovrappongono:
  `inizio_A < fine_B  &&  inizio_B < fine_A`.
  Le prenotazioni `REJECTED` non contano mai nei conflitti.
  Se una finisce alle 18:00 e un'altra inizia alle 18:00 **non** sono in conflitto.

### Formato di una prenotazione (nelle liste)

```
BOOKING <id> <postazione> <data> <inizio> <fine> <utente> <stato>
```
Esempio: `BOOKING 17 PC-03 20-12-2026 18:00 21:00 mario PENDING`

---

## 2. Sessione e autenticazione

Una connessione TCP corrisponde a una sessione. Appena connesso, il client
**non è autenticato**: gli unici comandi accettati sono `LOGIN` e `QUIT`.
Tutti gli altri ricevono `ERR NOT_LOGGED`.

Il server ricorda **chi** ha fatto login sulla connessione e usa quel nome per
tutti i controlli. Il client non invia mai il proprio username nei comandi
successivi al login. Così un utente non può fingersi un altro.

### `LOGIN <username> <password>`

| Risposta | Significato |
|----------|-------------|
| `OK USER` | login riuscito, utente standard |
| `OK ADMIN` | login riuscito, amministratore |
| `ERR BAD_CREDENTIALS` | username o password errati |
| `ERR TOO_MANY_ATTEMPTS` | 3° tentativo fallito: il server **chiude la connessione** |
| `ERR ALREADY_LOGGED` | c'è già un login attivo su questa connessione |
| `ERR SYNTAX` | numero di parametri sbagliato |

### `LOGOUT`

Chiude la sessione ma non la connessione: il client torna non autenticato e
può rifare `LOGIN`.
Risposta: `OK` (oppure `ERR NOT_LOGGED`).

### `QUIT`

Risposta: `OK BYE`, poi il server chiude la connessione.
Si può usare anche senza login.

---

## 3. Comandi per tutti gli utenti autenticati

### `STATIONS`

Elenca le postazioni esistenti.

```
C: STATIONS
S: OK 4
S: STATION PC-01 PC
S: STATION PC-02 PC
S: STATION PS5-1 PS5
S: STATION SWITCH-1 SWITCH
```

### `BOOK <postazione> <data> <inizio> <fine>`

Crea una nuova richiesta di prenotazione, che parte nello stato `PENDING`.

| Risposta | Significato |
|----------|-------------|
| `OK <id> PENDING` | richiesta registrata |
| `ERR SYNTAX` | parametri mancanti o in formato errato |
| `ERR NO_SUCH_STATION` | la postazione non esiste |
| `ERR INVALID_DATE` | data inesistente (es. `30-02-2026`) o nel passato |
| `ERR INVALID_TIME` | ora non valida, fuori orario, `fine <= inizio`, durata non ammessa |
| `ERR CONFLICT <id>` | si sovrappone alla prenotazione **APPROVED** `<id>` |

> **Politica sui conflitti.** Una richiesta che si sovrappone a una prenotazione
> **approvata** viene rifiutata subito (`ERR CONFLICT`). Una richiesta che si
> sovrappone solo a richieste **in attesa** viene accettata come `PENDING`.
> Sarà l'amministratore a risolvere il conflitto (vedi `CONFLICTS` e `APPROVE`).
>
> Controllo e inserimento avvengono mentre il server tiene il **lock**
> sull'archivio. Per questo due client che prenotano nello stesso istante non
> possono "passare entrambi".

### `MY [filtri...]`

Elenca **solo le prenotazioni dell'utente loggato**, eventualmente filtrate.
I filtri hanno la forma `chiave=valore` e si possono combinare (condizione AND):

| Filtro | Esempio |
|--------|---------|
| `id=` | `id=17` |
| `station=` | `station=PC-03` |
| `date=` | `date=20-12-2026` |
| `status=` | `status=PENDING` |

```
C: MY date=20-12-2026
S: OK 2
S: BOOKING 17 PC-03 20-12-2026 18:00 21:00 mario PENDING
S: BOOKING 21 PS5-1 20-12-2026 21:00 22:00 mario APPROVED
```

Errori: `ERR SYNTAX` (filtro scritto male o chiave sconosciuta).
Nessun risultato non è un errore: la risposta è `OK 0`.

> **Privacy.** Se un utente cerca `MY id=5` e la prenotazione 5 appartiene a
> un altro utente, la risposta è `OK 0`, esattamente come se non esistesse.
> Così non si rivela nemmeno l'esistenza delle prenotazioni altrui.

---

## 4. Comandi riservati all'amministratore

Se un utente standard invia uno di questi comandi, riceve `ERR UNAUTHORIZED`.

### `ALL [filtri...]`

Come `MY`, ma su **tutte** le prenotazioni. Accetta anche il filtro `user=`.

```
C: ALL status=PENDING
S: OK 2
S: BOOKING 17 PC-03 20-12-2026 18:00 21:00 mario PENDING
S: BOOKING 18 PC-03 20-12-2026 20:00 22:00 luigi PENDING
```

### `APPROVE <id>`

Approva una richiesta. Tutte le richieste `PENDING` in conflitto con essa
vengono **rifiutate automaticamente**.

| Risposta | Significato |
|----------|-------------|
| `OK <id> APPROVED <k>` | approvata; `<k>` = numero di richieste rifiutate automaticamente |
| `ERR NOT_FOUND` | l'ID non esiste |
| `ERR CONFLICT <id2>` | si sovrappone alla prenotazione già approvata `<id2>` |
| `ERR SYNTAX` | ID mancante o non numerico |

### `REJECT <id>`

Rifiuta una richiesta.
Risposte: `OK <id> REJECTED`, `ERR NOT_FOUND`, `ERR SYNTAX`.

### `SET <id> <stato>`

Imposta direttamente lo stato di una prenotazione (es. riportare in `PENDING`
una prenotazione approvata per errore). Impostare `APPROVED` fa gli **stessi
controlli** di `APPROVE`.

| Risposta | Significato |
|----------|-------------|
| `OK <id> <stato>` | stato modificato |
| `ERR NOT_FOUND` | l'ID non esiste |
| `ERR INVALID_STATUS` | stato diverso da `PENDING`/`APPROVED`/`REJECTED` |
| `ERR CONFLICT <id2>` | (solo per `APPROVED`) conflitto con una prenotazione approvata |
| `ERR SYNTAX` | parametri mancanti |

### `CONFLICTS`

Elenca le coppie di prenotazioni non rifiutate che si sovrappongono.
Ogni coppia compare una volta sola, con l'ID più piccolo per primo.

```
C: CONFLICTS
S: OK 1
S: CONFLICT 17 18
```

L'amministratore può poi usare `ALL id=17`, `ALL id=18` per vedere i dettagli,
e `APPROVE` o `REJECT` per risolvere il conflitto.

---

## 5. Codici di errore

| Codice | Quando |
|--------|--------|
| `UNKNOWN_COMMAND` | il comando non esiste |
| `SYNTAX` | numero o formato dei parametri errato |
| `LINE_TOO_LONG` | riga oltre i 256 byte (il resto della riga viene scartato) |
| `NOT_LOGGED` | comando che richiede il login, inviato senza login |
| `ALREADY_LOGGED` | `LOGIN` con una sessione già attiva |
| `BAD_CREDENTIALS` | login fallito |
| `TOO_MANY_ATTEMPTS` | 3 login falliti: la connessione viene chiusa |
| `UNAUTHORIZED` | comando da admin inviato da un utente standard |
| `NO_SUCH_STATION` | postazione inesistente |
| `INVALID_DATE` | data non valida o nel passato |
| `INVALID_TIME` | orario non valido o fuori dalle regole di prenotazione |
| `INVALID_STATUS` | stato sconosciuto in `SET` |
| `NOT_FOUND` | ID inesistente (lato admin) |
| `CONFLICT` | sovrapposizione con una prenotazione approvata |
| `SERVER` | errore interno del server (es. lettura/scrittura dell'archivio fallita) |

### Errori di rete (non sono messaggi, ma vanno gestiti)

- **Il client si disconnette**: `read()` sul server restituisce 0 o -1. Il
  processo figlio chiude il socket e termina, senza lasciare lock attivi.
- **Il server cade**: `read()` o `write()` sul client falliscono. Il client
  mostra "Connessione con il server persa" e termina in modo pulito, senza crash.

---

## 6. Sessione di esempio completa

```
C: BOOK PC-03 20-12-2026 18:00 21:00
S: ERR NOT_LOGGED
C: LOGIN mario pw123
S: OK USER
C: BOOK PC-03 20-12-2026 18:00 21:00
S: OK 17 PENDING
C: BOOK PC-03 20-12-2026 19:00 25:00
S: ERR INVALID_TIME
C: ALL
S: ERR UNAUTHORIZED
C: LOGOUT
S: OK
C: LOGIN admin admin
S: OK ADMIN
C: CONFLICTS
S: OK 1
S: CONFLICT 17 18
C: APPROVE 17
S: OK 17 APPROVED 1
C: QUIT
S: OK BYE
```

(Si assume che nel frattempo l'utente `luigi` abbia creato la richiesta 18,
che si sovrappone alla 17. Approvando la 17, la 18 viene rifiutata
automaticamente.)

---

## 7. Estensioni previste (non ancora nel protocollo)

- **Priorità**: campo `priority` (es. clienti VIP o tornei). `CONFLICTS` le
  ordinerà per priorità.
- **Disponibilità**: comando `AVAIL <data>` che mostra le fasce libere di ogni
  postazione, utile per la griglia dell'interfaccia grafica.
- **Annullamento**: comando `CANCEL <id>` con cui l'utente ritira una propria
  richiesta `PENDING`.
