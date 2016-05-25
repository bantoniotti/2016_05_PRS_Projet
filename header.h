#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include "semaphore.h"

#define RCVSIZE 1494
#define DATAPORT 7500
#define MAX_CLIENT 20
#define CLESEM 200
