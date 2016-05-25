#include <stdlib.h>
#include <stdio.h>

//pour le multi thread
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>

//pour la creation de la socket
#include <sys/types.h> 
#include <sys/socket.h> 

//pour l'utilisation de struct sockaddr_in
#include <netinet/in.h>

//pour la fermeture des sockets
#include <unistd.h>

//pour le calcule du RTT
#include <time.h>
#include <math.h>

//pour la manipulation des buffers
#include <string.h>

#define RCVSIZE 1024
#define DROITS 0600

//TO DO SI LES PREMIERE OU DERNIER SONT PERDU...

char emplacementFile[100];
char dataToSend[RCVSIZE];

static int semLeSemaphore;


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct sockaddr_in adresseEchange;	
socklen_t socklen;
int descripteur_echange;
int portEchange;
float RTTVAR = 10;
float SRTT = 0;
int readyToSend = 1;

int tailleFenetre = 30;
int tailleFenetreMax = 30;
int success = 0;
int premiereSeq = 0;
int nbreTrame;

typedef struct{
	int* ack;
	int* frt;
	int* receptionFenetre;
	int sendTime;
	int seq;
}ForTimeOut;

typedef struct{
	int* ack;
	int* frt;
	int* emissionFenetre; 
	int* receptionFenetre;
}ThreadArgs;

int up(int id)
{
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = 0;
	
	return semop(id, &op, 1);
}

int down(int id)
{
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = 0;
	
	return semop(id, &op, 1);
}

int calcule_nbreTrame(char emplacement_fichier[])
{
	down(semLeSemaphore);
	FILE* fichier = fopen(emplacement_fichier,"rb");

	long size;
 
    	if(fichier)
    	{
        	fseek (fichier, 0, SEEK_END);  
            	size=ftell (fichier);
            	
    	}
	fclose(fichier);
	up(semLeSemaphore);
    return (int)size/RCVSIZE+1;
}


int selectTrame(int seq)
{
	int octet;
	memset(dataToSend, 0, sizeof(dataToSend));
	down(semLeSemaphore);
	FILE * fichier = fopen(emplacementFile, "rb");
	int offset = RCVSIZE * seq;
	fseek(fichier, offset, SEEK_SET);
	octet = fread(dataToSend,1, RCVSIZE, fichier);
	fclose(fichier);
	up(semLeSemaphore);
	return octet;
}

void envoieTrame(int seq, int* ack, int octet)
{
	char numSequence[6];
	char a [6];
	char trame [RCVSIZE + 6];
	int i;
	time_t time;

	memset(trame, 0, sizeof(trame));
	memset(numSequence, 0, sizeof(numSequence));
	//CREATION DE LA TRAME
	/****************************************/
	sprintf(a,"%d", seq+1);
	for(i = 0; i < 6-strlen(a); i++)
		numSequence[i]='0';
	for(i = 0 ; i < strlen(a); i++)
		numSequence[i+6-strlen(a)]=a[i];	
	strcpy(trame, numSequence);
	memcpy(trame+6, dataToSend, sizeof(dataToSend));
	/****************************************/
	sleep(1);
	sendto(descripteur_echange,trame,octet+6,0, (struct sockaddr*)&adresseEchange,sizeof(adresseEchange));
	time = clock();
	ack[seq] = time;
}

void *threadTimeOut(void *arg)
{
	ForTimeOut* fto = (ForTimeOut*)arg;
	int octet;
	const double pause = SRTT;
	time_t time;
	float delay;
	while(fto->ack[fto->seq] != 1)
	{ 
		if(success)
			pthread_exit(NULL);

		time = clock();
		delay = (float)(time - (fto->ack)[fto->seq])/CLOCKS_PER_SEC;
		
		if((delay > 2)||((fto->frt[fto->seq-1] > 3)&&(fto->seq != 0)))
		{
			pthread_mutex_lock (&mutex);
			octet = selectTrame(fto->seq);
			if(octet > 0)
				envoieTrame(fto->seq, fto->ack, octet);
			pthread_mutex_unlock (&mutex);
			sleep(pause);
		}
	}
	//printf("reception de SEQ%d\n", fto->seq+1);
	pthread_exit(NULL);
}

void *threadEnvoie(void *arg)
{
	ThreadArgs* threadArguments = (ThreadArgs * )arg;
	
	int seq = 0;
	time_t time;
	int octet;
	int i;
	int sent = 0;
	
	while(!success)
	{	
		if(readyToSend)
		{
			sent = 1;
				
			for(i = 0; i < tailleFenetreMax; i++)
			{
				sent = sent && (threadArguments->emissionFenetre[i] == 1);
			}
				
			if(!sent)
			{		
				pthread_mutex_lock (&mutex);
				octet = selectTrame(seq);
				if(octet>0)
				{
					time = clock();
					//CREATION THREAD TIMEOUT
					/***********************************************/
					pthread_t threadTO;	
					ForTimeOut * fto = malloc(sizeof(ForTimeOut));
					fto->seq = seq;
					fto->sendTime = time;
					fto->ack = threadArguments->ack;
					fto->receptionFenetre = threadArguments->receptionFenetre;
					fto->frt = threadArguments->frt;	
					if (pthread_create(&threadTO, NULL, threadTimeOut, (void*) fto) < 0) {
							exit (1);
					}
					/***********************************************/
				
						envoieTrame(seq, threadArguments->ack, octet);
						seq ++;
									
				}
				pthread_mutex_unlock (&mutex);
				
				threadArguments->emissionFenetre[seq-premiereSeq] = 1;
			}
		}
	}
	pthread_exit(NULL);
}

void *threadReception(void *arg)
{
	ThreadArgs* threadArguments = (ThreadArgs * )arg;
	time_t time;
	float delay;
	char sequence[6];
	char buffer[RCVSIZE];
	int ackRvd [nbreTrame];
	int seq;
	int i;
	int fenetreCompleted;
	int first = 1;
	
	for(i = 0; i < nbreTrame; i ++)
	{
		ackRvd[i] = 0;
	}

	while(!success)
	{
		recvfrom(descripteur_echange,buffer,RCVSIZE,0,(struct sockaddr*)&adresseEchange,&socklen);
		printf("received %s\n", buffer);
		time = clock();
		strcpy(sequence, &buffer[6]);
		seq = atoi(sequence);
		seq -=1;
		threadArguments->frt[seq] += 1;

		if(seq == nbreTrame+1)
		{			
			pthread_exit(NULL);
		}
		
		if(ackRvd[seq]!=1)
		{	
			delay = (float)(time - (threadArguments->ack)[seq])/CLOCKS_PER_SEC;
			if(!first)
			{
				RTTVAR = 1 - 0.25*RTTVAR + 0.25*fabs(SRTT-delay);
				SRTT = delay;
			}
			else
			{
				RTTVAR = delay/2;
				SRTT = delay;
				first = 0;
			}
			i = 0;
			while(i <= seq-premiereSeq)
			{
				threadArguments->receptionFenetre[i] = 1;
				i++;
			}
			i = premiereSeq;
			while(i <= seq)
			{
				ackRvd[i] = 1;
				threadArguments->ack[i] = 0;
				i ++;
			}	

			fenetreCompleted = 1;
			for(i = 0; i < tailleFenetreMax; i ++)
			{
				fenetreCompleted = fenetreCompleted && (threadArguments->receptionFenetre[i]==1);	
			}
			if(fenetreCompleted)
			{
				premiereSeq += tailleFenetreMax;
				
				readyToSend = 0;
				for(i = 0; i < tailleFenetreMax; i ++)
				{
					threadArguments->emissionFenetre[i] = 0;
				}
				readyToSend = 1;
				for(i = 0; i < tailleFenetreMax; i ++)
				{
					threadArguments->receptionFenetre[i] = 0;
				}
			}
		}

		success = 1;
		for(i = 0; i < nbreTrame; i ++)
		{
			success = success && (ackRvd[i] == 1);	
		}
	}
	pthread_exit(NULL);
}

int hand_shake(int sock, struct sockaddr_in serveur)
{
	 char buffer[RCVSIZE];

	 int success = 0;
	 portEchange = 1024 + rand()%8975;

	 memset(buffer,0,RCVSIZE);	
	 recvfrom(sock,buffer,RCVSIZE,0,(struct sockaddr*)&serveur, &socklen);	

	 if(strcmp(buffer,"SYN") == 0){
		
		//CREATION D'UNE SOCKET D'ECHANGE
		/***********************************************/
		descripteur_echange = socket(AF_INET, SOCK_DGRAM, 0);
		if(descripteur_echange < 0)
		{
			printf("(PID%d)PORT %d : erreur dans la création de la socket\n",getpid(), portEchange);
			exit(-1);
		}
		/***********************************************/
		
		//INITIALISATION DE LA SOCKET D'ECHANGE
		/***********************************************/
		int valid = 1;
		setsockopt(descripteur_echange, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
		adresseEchange.sin_family= AF_INET;
		adresseEchange.sin_port= htons(portEchange);
		adresseEchange.sin_addr.s_addr= htonl(INADDR_ANY);
		/***********************************************/
		
		//BINDING DE LA SOCKET DE CONNEXION À L'ADRESSE DE LOOPBACK
		/***********************************************/
		int res = bind(descripteur_echange, (struct sockaddr*)&adresseEchange, sizeof(adresseEchange));
		if(res < 0)
		{
			printf("(PID%d)PORT %d : erreur dans le bind de la socket\n", getpid(), portEchange);
			exit(-1);
		}
		/***********************************************/
	
		memset(buffer,0,RCVSIZE);	
	 	sprintf(buffer,"SYN-ACK%d",portEchange);
	 	sendto(sock,buffer,strlen(buffer)+1,0, (struct sockaddr*)&serveur,sizeof(serveur));
			
		memset(buffer,0,RCVSIZE);	
		recvfrom(sock,buffer,RCVSIZE,0,(struct sockaddr*)&serveur, &socklen);	
		if(strcmp(buffer,"ACK") == 0){
			success = 1;		 	
		}
	}
	return success;
}

int main (int argc, char *argv[]) {
	
	srand(time(NULL));
	char buffer[RCVSIZE];
	pthread_t threadRecep;
	pthread_t threadEnv;
	
	semLeSemaphore = semget(IPC_PRIVATE, 1, DROITS | IPC_CREAT);
	semctl(semLeSemaphore, 0, SETVAL, 1);
	
	if(argc != 2)
	{
		printf("Erreur dans l'argument\n");
		printf("Usage : ./Serveur port_connexion\n");
		exit(-1);
	}
	else
	{	
		
		//CREATION DE LA SOCKET D ECOUTE
		/***********************************************/
		int sock;
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if(sock < 0)
		{
			printf("SOCKET DE CO : erreur dans la création de la socket\n");
			exit(-1);
		}
		/***********************************************/

		//INITIALISATION DE LA SOCKET D ECOUTE
		/***********************************************/
		int valid = 1;
		struct sockaddr_in adresseEcoute;		
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int));
		adresseEcoute.sin_family= AF_INET;
		adresseEcoute.sin_port= htons(atoi(argv[1]));
		adresseEcoute.sin_addr.s_addr= htonl(INADDR_ANY);
		
		socklen = sizeof(adresseEcoute);
		/***********************************************/

		//BINDING DE LA SOCKET D ECOUTE À L'ADRESSE DE LOOPBACK
		/***********************************************/
		int res = bind(sock, (struct sockaddr*)&adresseEcoute, sizeof(adresseEcoute));
		if(res < 0)
		{
			printf("SOCKET DE CO : erreur dans le bind de la socket\n");
			exit(-1);
		}
		/***********************************************/
		
		while(1){
			//Connection avec le client sur le port public
			if (hand_shake(sock, adresseEcoute)) {
				printf("connection etablie avec le client\n");
			} else {
				printf("erreur de connection avec le client\n");
				close(sock);
				return -1;
			}
			
			int pid = fork();
			
			if(pid == -1) {
				printf("Erreur fork %d\n", pid);
			}
			
			if (pid == 0){
				
				//RECUPERATION DU NOM DU FICHIER
				/***********************************************/
				char fichier[100];
				recvfrom(descripteur_echange,fichier,100,0,(struct sockaddr*)&adresseEchange,&socklen);
				printf("(PID%d)PORT %d : Connection d'un client qui demande : %s\n",getpid(), portEchange, fichier);
				down(semLeSemaphore);
				FILE* file = fopen(fichier, "r");
				if(file == NULL){
				printf("(PID%d)PORT %d : erreur dans l'ouverture du fichier\n", getpid(), portEchange);
					sprintf(buffer,"FIN");
					sendto(descripteur_echange,buffer,strlen(buffer)+1,0, (struct sockaddr*)&adresseEchange,sizeof(adresseEchange));
					exit(-1);
				}
				fclose(file);
				up(semLeSemaphore);
				strcpy(emplacementFile,fichier);
				/***********************************************/
				nbreTrame = calcule_nbreTrame(emplacementFile);
				int listACK[nbreTrame];
				int frt[nbreTrame];

				int fenetreEmission[tailleFenetre];
				int fenetreReception[tailleFenetre];
				int i = 0;
				for(i = 0; i < nbreTrame; i ++)
				{
					listACK[i] = 0;
					frt[i] = 0;
				}
				for(i = 0; i < tailleFenetreMax; i ++)
				{	
					fenetreEmission[i] = 0;
					fenetreReception[i] = 0;
				}
			
				ThreadArgs*  l = malloc(sizeof(ThreadArgs));
				l->ack = listACK;
				l->emissionFenetre = fenetreEmission;
				l->receptionFenetre = fenetreReception;
				l->frt = frt;
				/***********************************************/
				//CREATION THREAD DE RECEPTION
				/***********************************************/
				if (pthread_create(&threadRecep, NULL, threadReception, (void*) l) < 0) {
						exit (1);
				}
				/***********************************************/
				
				//CREATION THREAD D'ENVOIE
				/***********************************************/
				if (pthread_create(&threadEnv, NULL, threadEnvoie, (void*) l) < 0) {
						exit (1);
				}
				/***********************************************/
				
				//ATTENTE DE LA FIN DES THREAD
				/***********************************************/
				pthread_join(threadRecep, NULL);
				pthread_join(threadEnv, NULL);
				/***********************************************/
				
				printf("(PID%d)PORT %d : Fin de transfert\n",getpid(), portEchange);
				sprintf(buffer,"FIN");
				sendto(descripteur_echange,buffer,strlen(buffer)+1,0, (struct sockaddr*)&adresseEchange,sizeof(adresseEchange));
				exit(0);
			}
		}
	}
	return 0;
}
