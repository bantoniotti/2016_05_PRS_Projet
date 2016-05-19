#include "header.h"

int idbal;

int waitForString (int desc, const struct sockaddr_in* client, struct timeval waitTime, char* stringExpected){

    fd_set descriptors; // On initialise de set de descripteurs pour le select
    char buffer[strlen(stringExpected)];
    memset(buffer, '\0', sizeof(buffer));
    int fromsize = sizeof(client);
    FD_SET(desc, &descriptors);
    select(6,&descriptors,NULL,NULL,&waitTime);
    recvfrom(desc, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*) client, &fromsize);
    return strncmp(buffer,stringExpected, strlen(stringExpected));
}


void customAccept(int desc, struct sockaddr_in* client, int newPort){ //La fonction recrée un échange de SYN, SYN-ACK, ACK comme dans un protocole TCP
    char buffer[RCVSIZE]; //Buffer contenant le message
    char* syn = "SYN"; 
    char* synack = "SYN-ACK"; //Trois chaines de caractères contenant les messages à échanger pour établir la connection
    char* ack = "ACK";
    int fromsize = sizeof(*client); //Taille de la structure contenant l'adrese du client
    struct timeval time;
    time.tv_sec = 1;
    time.tv_usec = 0;
    char* synackport = (char*) malloc((strlen(synack)+6)*sizeof(char));
    sprintf(synackport, "%s%d", synack, newPort);
  
    /* Attente du SYN, on boucle tant qu'on a pas un message correct*/

    while (waitForString(desc, (const struct sockaddr_in*) client, time, syn) != 0){
    }

    fprintf(stderr,"SYN received");

    int valid = 1;

    while (valid){
        time.tv_sec = 4; //Temps d'attente avant le réenvoi d'un message
        if(sendto(desc, synackport, strlen(synackport), 0, (struct sockaddr *) client, fromsize) < 0){
            perror("\nError while sending SYN-ACK, please restart server.\n");
            exit(-1);
        }
        else {
            fprintf(stderr,", sending %s\n", synackport);
        }

        /*On commence par appller la fonction select, si elle recoit une modification 
        * du descripteur, ou que le timeout arrive à expiration, le programme passe
        * à la fonction recvfrom avec le paramètre MSG_DONTWAIT qui fait passer la fonction
        * si elle n'a pas de message à transmettre. On compare alors la chaine reçue. 
        * Si elle ne correspond pas ou est vide, on recommence l'envoi d'un SYN-ACK*/

        valid = waitForString(desc, (const struct sockaddr_in*) client, time, ack);

        if(valid == 0){
            fprintf(stderr,"ACK received, connection established\n");
        }
        else {
            fprintf(stderr,"Error, the client does not respond, reseting connection");
            memset(buffer, '\0', sizeof(buffer));
        }
    }
}


/*******************************************************************/
//Cette fonction crée un descripteur et un socket puis les associe  /
//Il faut rentrer le port et l'adresse au format host               /
/*******************************************************************/
int createDesc(int port, int adress, struct sockaddr_in* sockaddress){
    int desc = socket(AF_INET, SOCK_DGRAM, 0); //On crée un premier descripteur UDP

    if (desc < 0) { //On teste si le descripteur a bien été ouvert, sinon, fin du pro gramme
        perror("cannot create socket\n");
        return -1;
    }
  
    int valid = 1;
  
    setsockopt(desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int)); //Permet de signaler à l'OS que le descripteur peut être réutilisé

    (*sockaddress).sin_family= AF_INET; //L'adresse est IP
    (*sockaddress).sin_port= htons(port); //Le port est passé au format serveur
    (*sockaddress).sin_addr.s_addr= htonl(adress);//L'adresse est passée au format serveur

    if (bind(desc, (struct sockaddr*) sockaddress, sizeof(*sockaddress)) == -1) { //On lie l'addresse avec le descripteur
        perror("Bind fail\n"); //Si erreur, on termine le programme
        close(desc);
        return -1;
    }
  
    return desc; //On retourne le descripteur prêt pour utilisation
}

int main(int argc,char *argv[]){
    
    struct sockaddr_in server, client;

    int port;

    if (argc != 2){
        fprintf(stderr, "Error, you must specify only the port number\n");
        exit(-1);
    }
    
    port = atoi(argv[1]);

    int publicDesc = createDesc(port, INADDR_ANY, &server);
    int newPID;
    int dataDesc = createDesc(DATAPORT, INADDR_ANY, &server);
    
    while(1){
        customAccept(publicDesc, &client, DATAPORT);
        newPID = fork();
        if (newPID == 0){
            
            char fileName[100];
            int sizeOfClient = sizeof(client);
            int sequenceNumber = 1;
            char buffer[RCVSIZE];
            char seqNumBuffer[7];
            char bufferPacket[RCVSIZE+7];
            char ackBuffer[10];
            int sizeOfDataSent = 0;
            char* fin = "FIN";
            int maxACK = 0;
            int timeout = 200;
            int ackNumber = 0;
            int window = 1;
            int i;
            clock_t tStart, tCurrent;
            recvfrom(dataDesc, fileName, sizeof(fileName), 0, (struct sockaddr *) &client, &sizeOfClient);
            fprintf(stderr, "FileName : %s\n", fileName);
            FILE* file = NULL;
            file = fopen(fileName, "rb");
            if (file < 0){
                fprintf(stderr, "Couldn't open file\nPlease, restart server");
                exit(-1);
            }
            errno = 0;
            tStart = clock();
            tCurrent = clock();
            
            while((maxACK!=sequenceNumber-1)||(!feof(file))){
                
                while (maxACK < sequenceNumber-1 && ((1000*(tCurrent - tStart))/CLOCKS_PER_SEC)< timeout){
                    tCurrent = clock();
                    recvfrom(dataDesc, ackBuffer, sizeof(ackBuffer), MSG_DONTWAIT, (struct sockaddr *) &client, &sizeOfClient);
                    sscanf(ackBuffer, "ACK%d", &ackNumber);
                    if (ackNumber>maxACK)
                        maxACK = ackNumber;
                    
                }
                fprintf(stderr, "Sortie de la boucle : maxACK = %d, sequenceNumber = %d\n", maxACK, sequenceNumber);
                if (maxACK == sequenceNumber-1){
                    fprintf(stderr, "Sortie de la boucle : maxACK = %d, sequenceNumber = %d\n", maxACK, sequenceNumber);
                    if (window < 8)
                        window *= 2;
                    else 
                        window += 2;
                    fprintf(stderr, "J'augmente la fenetre\n");
                }
                else {
                    if (window > 1)
                        window /= 2;
                    fprintf(stderr, "Je diminue la fenetre\n");
                }
                sequenceNumber = maxACK + window;
                i = maxACK+1;
                while(i<sequenceNumber && !feof(file)){
                    
                    fseek(file, ((i-1)*RCVSIZE), SEEK_SET);
                    fprintf(stderr, "i = %d, window : %d, position : %d\n", i, window, ftell(file));
                    memset(buffer, 0, sizeof(buffer));
                    memset(seqNumBuffer, 0, sizeof(seqNumBuffer));
                    memset(bufferPacket, 0, sizeof(bufferPacket));
                    sprintf(seqNumBuffer, "%06d", i);
                    fprintf(stderr, "SeqNum : %s\n", seqNumBuffer);
                    sizeOfDataSent = fread(buffer, 1, RCVSIZE, file);
                    //fprintf(stderr,"%s\n\n",buffer);
                    strcat(bufferPacket, seqNumBuffer);
                    memcpy(&bufferPacket[6], buffer, RCVSIZE);
                    //fprintf(stderr, "%s\n", bufferPacket);
                    if (sendto(dataDesc, bufferPacket , sizeOfDataSent+6, 0, (struct sockaddr *) &client, sizeOfClient) <= 0){
                        perror("Error while sending packet");
                    }
                    i++;
                }
                
                tStart = clock();
            }
            
            sendto(dataDesc, fin , sizeof(fin), 0, (struct sockaddr *) &client, sizeOfClient);
                
        }
        else {
            //Gestion des ACK + nouveaux clients
        }
        // fprintf(stderr, "Coucou\n");
    }
}
