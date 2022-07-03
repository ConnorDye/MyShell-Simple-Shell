CC = gcc
CFLAGS = -Wall -g -pedantic -std=gnu99 -I ~pn-cs357/Given/Mush/include
LD = gcc
LDFLAGS = -g -Wall -L ~pn-cs357/Given/Mush/lib64

mush2: main.o process.o
	$(LD) $(LDFLAGS) -o mush2 main.o process.o libmush.a

main.o: main.c
	$(CC) $(CFLAGS) -c -o main.o main.c

process.o: process.c
	$(CC) $(CFLAGS) -c -o process.o process.c

clean:
	rm *.o mush2

git:
	git add main.c README Makefile libmush.a mush.h process.h process.c                                          
	git commit -m "$m"                                                          
	git push -u origin master

scp:
	scp main.c process.c libmush.a mush.h process.h README Makefile chdye@unix2.csc.calpoly.edu:./asgn6

gdb:
	gdb --args ./mush2 
