playtkv : playtkv.o libtkv.o
	gcc -o playtkv playtkv.o libtkv.o -lm

readtkv : readtkv.o libtkv.o
	gcc -o readtkv readtkv.o libtkv.o -lm

readtkv.o : readtkv.c libtkv.h
	gcc -c readtkv.c

playtkv.o : playtkv.c libtkv.h
	gcc -c playtkv.c
	
libtkv.o : libtkv.c libtkv.h
	gcc -c libtkv.c
	
clean :
	rm readtkv playtkv readtkv.o playtkv.o libtkv.o
	
dotests: readtkv
	./readtkv dotests
	
