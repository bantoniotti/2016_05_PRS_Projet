



/*******************************************************************/
//Cette fonction crée un descripteur et un socket puis les associe  /
//Il faut rentrer le port et l'adresse au format host               /
/*******************************************************************/
int createDesc(int port, int adress){
  int desc = socket(AF_INET, SOCK_DGRAM, 0); //On crée un premier descripteur UDP

  if (desc < 0) { //On teste si le descripteur a bien été ouvert, sinon, fin du programme
    perror("cannot create socket\n");
    return -1;
  }

  setsockopt(desc, SOL_SOCKET, SO_REUSEADDR, &valid, sizeof(int)); //Permet de signaler à l'OS que le descripteur peut être réutilisé

  adresse.sin_family= AF_INET; //L'adresse est IP
  adresse.sin_port= htons(port); //Le port est passé au format serveur
  adresse.sin_addr.s_addr= htonl(adress);//L'adresse est passée au format serveur

  if (bind(desc, (struct sockaddr*) &adresse, sizeof(adresse)) == -1) { //On lie l'addresse avec le descripteur
    perror("Bind fail\n"); //Si erreur, on termine le programme
    close(desc);
    return -1;
  }
  
  return desc; //On retourne le descripteur prêt pour utilisation
}

int main(int argc,char *argv[]){
	
}
