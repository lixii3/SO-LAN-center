# ---------------------------------------------------------------------------
#  LAN Center Booking System - Makefile
# ---------------------------------------------------------------------------

SERVER_NAME = server
CLIENT_NAME = client

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g

SRC_DIR = src
OBJ_DIR = .obj

# file in comune
COM_SRC = net.c time.c booking.c

# file esclusivi
SRV_SRC = server.c
CLI_SRC = client.c

# --- Oggetti -----------------------------------------------------------------
COM_OBJ    = $(addprefix $(OBJ_DIR)/common/, $(COM_SRC:.c=.o))

SERVER_OBJ = $(addprefix $(OBJ_DIR)/server/, $(SRV_SRC:.c=.o)) \
             $(COM_OBJ)

CLIENT_OBJ = $(addprefix $(OBJ_DIR)/client/, $(CLI_SRC:.c=.o)) \
             $(COM_OBJ)

# --- Regole ------------------------------------------------------------------
all: $(SERVER_NAME) $(CLIENT_NAME)

$(SERVER_NAME): $(SERVER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(CLIENT_NAME): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Compila
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run-server: $(SERVER_NAME)
	./$(SERVER_NAME)

# uso: make run-client MSG="hello world"
MSG ?= hello world
run-client: $(CLIENT_NAME)
	./$(CLIENT_NAME) "$(MSG)"

# Avvia il server in background, manda un messaggio col client, poi chiude il server
test: all
	./$(SERVER_NAME) & SRV_PID=$$!; \
	sleep 1; \
	./$(CLIENT_NAME) "$(MSG)"; \
	kill $$SRV_PID

valgrind: $(SERVER_NAME)
	valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./$(SERVER_NAME)

# Cancella i file oggetto
clean:
	rm -rf $(OBJ_DIR)

# Cancella file oggetto ed eseguibili
fclean: clean
	rm -f $(SERVER_NAME) $(CLIENT_NAME)

# Ricompila tutto da zero
re: fclean all

.PHONY: all run-server run-client test valgrind clean fclean re
