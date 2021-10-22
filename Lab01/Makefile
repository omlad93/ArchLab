OBJS	= iss.o
SOURCE	= iss.c
HEADER	= iss.h
OUT	= iss.exe
CC	 = gcc
LFLAGS	 = 

all: $(OBJS)
	$(CC) $(OBJS) -o $(OUT)
iss.o: iss.c
	$(CC) iss.c 


clean:
	rm -f $(OBJS) $(OUT)