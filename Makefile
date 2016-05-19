all : serveur1-XXX serveur2-XXX serveur3-XXX

serveur1-XXX: serveur1-XXX.c
	gcc -Wall -g $^ -o $@

serveur2-XXX: serveur2-XXX.c
	gcc -Wall -g $^ -o $@

serveur3-XXX: serveur3-XXX.c
	gcc -Wall -g $^ -o $@

clean :
	rm serveur1-XXX
	rm serveur2-XXX
	rm serveur3-XXX