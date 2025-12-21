CC=gcc

LIB_NAME=libqueue.a

SOURCES=queue.h queue.c
LIB_OBJ=queue.o


all: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJ)
	ar rc $(LIB_NAME) $(LIB_OBJ)

$(LIB_OBJ): $(SOURCES)
	$(CC) -c $(SOURCES)

clean:
	rm -rf *.o *.gch *.a
