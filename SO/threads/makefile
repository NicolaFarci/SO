# Nome dell'eseguibile
TARGET = frogger

# File sorgente
SRC = main.c game.c frog.c  consumer.c map.c timer.c crocodile.c grenade.c spawner.c buffer.c

# File oggetto (generati dai sorgenti)
OBJ = $(SRC:.c=.o)

# Header file
HEADERS = game.h frog.h map.h timer.h crocodile.h consumer.h grenade.h spawner.h buffer.h

# Flags del compilatore
CC = gcc
FLAGS = -lncurses -lpthread -Wall -Wextra

# Regola principale
all: $(TARGET)

# Regola per creare l'eseguibile
$(TARGET): $(OBJ)
	gcc -o $@ $^ $(FLAGS)

# Regola per creare i file oggetto
%.o: %.c $(HEADERS)
	$(CC) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -f $(OBJ) $(TARGET)

# Pulizia e ricompilazione completa
rebuild: clean all

