all : serveur1-XXX

serveur1-XXX: serveur1-XXX.c
	gcc -Wall -g $^ -o $@

clean :
	rm serveur1-XXX
