#include "header.h"

int accept(int desc,struct sockaddr_in *client){
	int syn, synack, ack, ackport;
	char connexion[10];
	char portdata[10];
	sprintf(portdata, "%d", PORTDATA);
	memset(connexion, '\0', 10);
	//memset(portdata, '\0', 10);
	int sizeconnexion = sizeof(connexion);
	int sizeclient = sizeof(*client);
	if (syn = recvfrom(desc, connexion, sizeof connexion - 1, 0, (struct sockaddr*) client, &sizeconnexion) > 0){
		printf("Tentative de Connexion en cours provenant de %s\n", inet_ntoa(client->sin_addr));
	} 
	else {
		printf("ERROR MSG SYN\n");
	}
	if(strcmp(connexion,"SYN") == 0){
		memset(connexion, '\0', 10);
		char connexion[10] = "SYN-ACK";
		synack = sendto(desc, connexion, strlen(connexion), 0, (struct sockaddr *) client, sizeclient);
		memset(connexion, '\0', 10);
		ackport = sendto(desc, portdata, strlen(portdata), 0, (struct sockaddr *) client, sizeclient);
		//ackport = sendport(desc, &client, sizeclient);
		printf("Acceptation de la connexion\n");
	} 
	else {
		printf("ERROR MSG SYN\n");
	}
	ack = recvfrom(desc, connexion, sizeof connexion - 1, 0, (struct sockaddr*) client, &sizeconnexion);
	if(strcmp(connexion,"ACK000000") == 0){
		printf(" \n\t Client connected \n\n");
		memset(connexion, '\0', 10);
		return 1;
	}
	else{
		printf("Connexion echouée \n");
		memset(connexion, '\0', 10);
		return 0;
	}
}
