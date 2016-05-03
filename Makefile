all : serveur1-XXX

serveur1-XXX: serveur1-XXX.c
	gcc -Wall $^ -o $@

clean :
	rm serveur1-XXX