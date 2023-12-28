CC=gcc
CFLAGS=-Wall -Wextra -pthread

# Directories
SRCDIR=./
INCDIR1=./Dependencies/hashmap/
INCDIR2=./Dependencies/LineReader/


# Source files
PROJETO_SRC=$(SRCDIR)Projeto.c
HASHMAP_SRC=$(INCDIR1)hashmap.c
LINEREAD_SRC=$(INCDIR2)LineReader.C

# Object files
PROJETO_OBJ=Projeto.o
HASHMAP_OBJ=hashmap.o
LINEREADER_OBJ=LineReader.o

# Output executable
TARGET=Projeto

all: $(TARGET)

$(TARGET): $(PROJETO_OBJ) $(HASHMAP_OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(PROJETO_OBJ) $(HASHMAP_OBJ)

$(PROJETO_OBJ): $(PROJETO_SRC)
	$(CC) $(CFLAGS) -c $(PROJETO_SRC) -I$(INCDIR1)

$(HASHMAP_OBJ): $(HASHMAP_SRC)
	$(CC) $(CFLAGS) -c $(HASHMAP_SRC) -I$(INCDIR1)

$(LINEREADER_OBJ): $(LINEREADER_SRC)
	$(CC) $(CFLAGS) -c $(LINEREADER_SRC) -I$(INCDIR2)

clean:
	rm -f $(PROJETO_OBJ) $(HASHMAP_OBJ) $(LINEREADER_OBJ) $(TARGET)