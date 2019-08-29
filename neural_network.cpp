#include <opencv2/highgui/highgui.hpp>
#include <leptonica/allheaders.h>
#include <linux/v4l2-controls.h>
#include <opencv2/core/core.hpp>
#include <tesseract/baseapi.h>
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
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <string>

using namespace std;
using namespace cv;

#define MEMORY_TWO "/shm_2"		// SEGMENTATION -> NEURAL NETWORK
#define MEMORY_THREE "/shm_3"	// NEURAL NETWORK -> PRINTER
#define SEMAPHORE_TWO "/sem_2"
#define SEMAPHORE_THREE "/sem_3"
#define SIZE 4000096
#define HEIGHT 720
#define WIDTH 1024
#define CHANNELS 3

////////////////////////////////////////////////////
/// Recognize
///
/// Funzione che utilizza l'OCR di Tesseract
/// per leggere il testo nell'immagine.
///////////////////////////////////////////////////

string recognize(Mat im){

	string outText;

	// Crea un oggetto di tipo Tesseract
	tesseract::TessBaseAPI *ocr = new tesseract::TessBaseAPI();
	// Inizializzo Tesseract per la lingua inglese 
	ocr->Init(NULL, "eng", tesseract::OEM_LSTM_ONLY);
	ocr->SetPageSegMode(tesseract::PSM_SINGLE_WORD);
	// Setta le impostazioni dell'immagine
	ocr->SetImage(im.data, im.cols, im.rows, 3, im.step);
	// Esegue Tesseract OCR sull'immagine
	outText = string(ocr->GetUTF8Text());

	return outText;

}

////////////////////////////////////////////////////
/// Main
///
/// Funzione che gestisce la memoria condivisa tra 
/// il segmentation, la rete neurale e il printer.
///////////////////////////////////////////////////

int main() {
	
	int mc_fd_two, mc_fd_three;
	char * ptr_two, * ptr_three;
	
	mc_fd_two = shm_open(MEMORY_TWO, O_RDWR, 0666);
	if(mc_fd_two == -1) {
		cerr << "Fallimento nell'APERTURA della memoria condivisa 2" << endl;
		exit(-1);
	}
	mc_fd_three = shm_open(MEMORY_THREE, O_RDWR, 0666);
	if(mc_fd_three == -1) {
		cerr << "Fallimento nell'APERTURA della memoria condivisa 3" << endl;
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

	sem_t * sem_id_two = sem_open(SEMAPHORE_TWO, 0);
	sem_t * sem_id_three = sem_open(SEMAPHORE_THREE, 1);

	cout << "NEURAL NETWORK avviata..." << endl;
	while(true) {
		sleep(1);
		if(strcmp(ptr_two, "?") != 0) {
			// ---------------------------------------------------------
			sem_wait(sem_id_two);
			cout << "--- Start -> SEZIONE CRITICA 2! ---" << endl;

				// -----------------------------------------------------
				sem_wait(sem_id_three);
				cout << "--- Start -> SEZIONE CRITICA 3!" << endl;

				// Scrivo la memoria convidisa 3
				Mat reverse = Mat(200, 300, CV_8UC3, ptr_two);
				string text = recognize(reverse);
				ftruncate(mc_fd_three, text.length());
				memcpy(ptr_three, text.c_str(), text.length());

				sem_post(sem_id_three);
				cout << "--- Stop -> SEZIONE CRITICA 3! ---" << endl;
				// -----------------------------------------------------
			
			// Pulisco la memoria convidisa 2
			char message_clean2[] = "?";
			ftruncate(mc_fd_two, strlen(message_clean2));
			sprintf(ptr_two, "%s", message_clean2);
			
			sem_post(sem_id_two);
			cout << "--- Stop -> SEZIONE CRITICA 2! ---" << endl;
			// ---------------------------------------------------------
		}
	}

	return 0;
}
