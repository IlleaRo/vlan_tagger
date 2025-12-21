CC=gcc

LIB_NAME=liblogger.a

SOURCES=logger.c logger.h
LIB_OBJ=logger.o


all: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJ)
	ar rc $(LIB_NAME) $(LIB_OBJ)

$(LIB_OBJ): $(SOURCES)
	$(CC) -c $(SOURCES)

clean:
	rm -rf *.o *.gch *.a