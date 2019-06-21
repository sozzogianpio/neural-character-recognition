// g++ 16_semafori-pc2.cpp -o client -lrt -pthread
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
	
	int mc_fd;
	char * ptr;
	
	mc_fd = shm_open(MEMORIA, O_RDONLY, 0666);	// creazione memoria condivisa tra processi
	if(mc_fd == -1) {
		cerr << "Fallimento nell'apertura della memoria condivisa\n";
		exit(-1);
	}
	
	ptr = (char*) mmap(0,SIZE,PROT_READ,MAP_SHARED, mc_fd,0);
	if(ptr == MAP_FAILED) {
		cout << "Fallimento nella mappatura\n";
		return -1;
	}
	
	// apre semaforo già inizializzato
	sem_t * sem_id = sem_open(SEMAFORO,0);

	while(true) {
		
		sleep(3);
		
		cout << "CLIENT WAIT\n";
		sem_wait(sem_id);	// --------- CHIAMA LA WAIT su sem_id (puntatore al semaforo appena creato)
		
		cout << "CLIENT: " << ptr;	// Legge TUTTA la memoria (eliminare valore già letto o posizionarsi al prossimo mess)
		
		cout << "\nCLIENT POST\n";
		sem_post(sem_id);	// ---------- CHIAMA LA SIGNAL
	}
	
	shm_unlink(MEMORIA);	// è possibile lasciare la memoria condivisa
	sem_unlink(SEMAFORO);	// è possibile lasciare il semaforo
	return 0;
}
