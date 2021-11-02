all: llsim dma
llsim: llsim.c llsim.h sp.c
	gcc -Wall -o llsim -O2 llsim.c sp.c
dma:	
	gcc -Wall -O2 ./with_dma/sp.c ./with_dma/llsim.c -o dma
clean:
	\rm -f llsim *~
	\rm -f dma *~

