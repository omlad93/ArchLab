OBJS	= iss.o
SOURCE	= iss.c
HEADER	= iss.h
OUT	= iss.exe
CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

iss.o: iss.c
	$(CC) $(FLAGS) iss.c 


clean:
	rm -f $(OBJS) $(OUT)