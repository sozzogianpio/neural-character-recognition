#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <linux/v4l2-controls.h>
#include <linux/v4l2-common.h>
#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <fstream>
#include <errno.h>
#include <cstring>
#include <string>

using namespace std;

#define MEMORY_ONE "/shm_1"		// WEBCAM -> SEGMENTATION
#define MEMORY_TWO "/shm_2"		// SEGMENTATION -> NEURAL NETWORK
#define MEMORY_THREE "/shm_3"	// NEURAL NETWORK -> PRINTER
#define SEMAPHORE_ONE "/sem_1"
#define SEMAPHORE_TWO "/sem_2"
#define SEMAPHORE_THREE "/sem_3"
#define SIZE 4000096

////////////////////////////////////////////////////
/// Main
///
/// Funzione che gestisce la creazione di tutte le 
/// memorie condivise e di tutti i semafori.
/// Inoltre, legge i risultati della rete neurale e
/// li stampa in output.
///////////////////////////////////////////////////

int main() {
	
	int mc_fd_one, mc_fd_two, mc_fd_three;
	char * ptr_one, * ptr_two, * ptr_three;

	shm_unlink(MEMORY_ONE);
	shm_unlink(MEMORY_TWO);
	shm_unlink(MEMORY_THREE);
	sem_unlink(SEMAPHORE_ONE);
	sem_unlink(SEMAPHORE_TWO);
	sem_unlink(SEMAPHORE_THREE);
	
	mc_fd_one = shm_open(MEMORY_ONE, O_RDWR | O_CREAT, 0666);
	if(mc_fd_one == -1) {
		cerr << "Fallimento nella CREAZIONE della memoria condivisa 1" << endl;
		exit(-1);
	}
	mc_fd_two = shm_open(MEMORY_TWO, O_RDWR | O_CREAT, 0666);
	if(mc_fd_two == -1) {
		cerr << "Fallimento nella CREAZIONE della memoria condivisa 2" << endl;
		exit(-1);
	}
	mc_fd_three = shm_open(MEMORY_THREE, O_RDWR | O_CREAT, 0666);
	if(mc_fd_three == -1) {
		cerr << "Fallimento nella CREAZIONE della memoria condivisa 3" << endl;
		exit(-1);
	}
	
	ptr_one = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mc_fd_one, 0);
	if(ptr_one == MAP_FAILED) {
		cout << "Fallimento nella MAPPATURA della memoria condivisa 1" << endl;
		exit(-1);
	}
	ptr_two = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mc_fd_two, 0);
	if(ptr_two == MAP_FAILED) {
		cout << "Fallimento nella MAPPATURA della memoria condivisa 2" << endl;
		exit(-1);
	}
	ptr_three = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mc_fd_three, 0);
	if(ptr_three == MAP_FAILED) {
		cout << "Fallimento nella MAPPATURA della memoria condivisa 3" << endl;
		exit(-1);
	}
	
	char message[] = "?";
	ftruncate(mc_fd_one, strlen(message));
	sprintf(ptr_one, "%s", message);
	
	char message2[] = "?";
	ftruncate(mc_fd_two, strlen(message2));
	sprintf(ptr_two, "%s", message2);
	
	char message3[] = "?";
	ftruncate(mc_fd_three, strlen(message3));
	sprintf(ptr_three, "%s", message3);
	
	sem_t * sem_id_one = sem_open(SEMAPHORE_ONE, O_CREAT, S_IRUSR | S_IWUSR, 1);
	sem_t * sem_id_two = sem_open(SEMAPHORE_TWO, O_CREAT, S_IRUSR | S_IWUSR, 1);
	sem_t * sem_id_three = sem_open(SEMAPHORE_THREE, O_CREAT, S_IRUSR | S_IWUSR, 1);
	
	cout << "PRINTER avviato..." << endl;
	while(true) {
		sleep(1);
		if(strcmp(ptr_three, "?") != 0) {
			// ---------------------------------------------------------
			sem_wait(sem_id_three);
			
			// Leggo la memoria condivisa 3
			cout << ptr_three << "\r";
			
			// Pulisco la memoria convidisa 3
			char message_clean3[] = "?";
			ftruncate(mc_fd_three, strlen(message_clean3));
			sprintf(ptr_three, "%s", message_clean3);
			
			sem_post(sem_id_three);
			// ---------------------------------------------------------
		}
	}
		
	return 0;
}
