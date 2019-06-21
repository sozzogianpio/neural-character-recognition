#include <cstring>
#include <sys/types.h>
#include <sys/wait.h> 
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

using namespace std;

#define MEMORIA "/pippo"
#define SEMAFORO "/sem_paperino"
#define SIZE 4096

int main() {
	
	//const char * message[]={"Sono", "scritto", "nella", "memoria", "condivisa\n"};	//da scrivere nella memoria condivisa
	int mc_fd;
	char *base, *ptr;
	
	mc_fd = shm_open(MEMORIA, O_CREAT | O_RDWR, 0666);	// creazione memoria condivisa tra processi
	if(mc_fd == -1) {
		cerr << "Fallimento nell'apertura della memoria condivisa\n";
		exit(-1);
	}
	
	//ftruncate(mc_fd, sizeof(message));	// DA DECIDERE E SAPERE MEGLIO CHE FA
	
	base = (char*) mmap(NULL,SIZE,PROT_READ | PROT_WRITE,MAP_SHARED, mc_fd,0);
	if(ptr == MAP_FAILED) {
		cout << "Fallimento nella mappatura\n";
		return -1;
	}
	
	sem_unlink(SEMAFORO);	// DISTRUGGE SEMAFORO CREATO IN UN'ESECUZIONE PRECEDENTE (da valutare se dà errore se il sem non esiste)
	
	// crea il semaforo col nome SEMAFORO e lo inizializza a 1, in modo che l'utente può leggere o scrivere
	sem_t * sem_id = sem_open(SEMAFORO,O_CREAT, S_IRUSR | S_IWUSR,1);
	
	ptr = base;	// va all'inizio della memoria condivisa
	
	while(true) { 
		
		const char * message[]={"Sono ", "scritto ", "nella ", "memoria ", "condivisa \n"};	//da scrivere nella memoria condivisa
				
		sleep(6);
				
		cout << "SERVER WAIT\n";
		sem_wait(sem_id);	// --------- CHIAMA LA WAIT su sem_id (puntatore al semaforo appena creato)
		cout << "SERVER WAIT 2\n";
		
		for (int i = 0; i < strlen(*message); ++i) {
			sprintf(ptr, "%s\n", message[i]);	// Scrive in memoria
			ptr += strlen(message[i]);
		}
		
		cout << "SERVER POST\n";
		sem_post(sem_id);	// ---------- CHIAMA LA SIGNAL
	}
		
	shm_unlink(MEMORIA);	// è possibile lasciare la memoria condivisa
	sem_unlink(SEMAFORO);	// è possibile lasciare il semaforo
	cout << "STOP\n";
	munmap(ptr,SIZE);	// "smappo" la memoria condivisa
	return 0;
}
