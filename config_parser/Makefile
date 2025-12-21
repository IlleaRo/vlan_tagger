CC=gcc

LIB_NAME=libconfig_parser.a

SOURCES=config_parser.h config_parser.c
LIB_OBJ=config_parser.o


all: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJ)
	ar rc $(LIB_NAME) $(LIB_OBJ)

$(LIB_OBJ): $(SOURCES)
	$(CC) -c $(SOURCES)

clean:
	rm -rf *.o *.gch *.a
