#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <linux/v4l2-controls.h>
#include <linux/v4l2-common.h>
#include <opencv2/opencv.hpp>
#include <linux/videodev2.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <stdio.h>
#include <fstream>
#include <string>
#include <list>

using namespace std;
using namespace cv;

#define MEMORY_ONE "/shm_1"		// WEBCAM -> SEGMENTATION
#define MEMORY_TWO "/shm_2"		// SEGMENTATION -> NEURAL NETWORK
#define SEMAPHORE_ONE "/sem_1"
#define SEMAPHORE_TWO "/sem_2"
#define SIZE 4000096
#define HEIGHT 720
#define WIDTH 1024
#define CHANNELS 3

////////////////////////////////////////////////////
/// Detect_Text
///
/// Funzione che ritaglia l'immagine arrivata dalla 
/// webcam in tante immagini rettangolari contenenti
/// del testo da inviare alla rete neurale
///////////////////////////////////////////////////

list<Mat> detect_text(char * buffer){

	CvMat cvmat = cvMat(WIDTH, HEIGHT, CV_8UC3, (void*)buffer);
	Mat large = cvDecodeImage(&cvmat, 1);
	Size size(300,200);

	list<Mat> frameList;
	Mat copy;
	Mat rgb;

	Mat small;
	large.copyTo(copy);
	cvtColor(large, small, CV_BGR2GRAY);

	Mat grad;
	Mat morphKernel = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	morphologyEx(small, grad, MORPH_GRADIENT, morphKernel);

	Mat bw;
	threshold(grad, bw, 0.0, 255.0, THRESH_BINARY | THRESH_OTSU);

	Mat connected;
	morphKernel = getStructuringElement(MORPH_RECT, Size(9, 1));
	morphologyEx(bw, connected, MORPH_CLOSE, morphKernel);

	Mat mask = Mat::zeros(bw.size(), CV_8UC1);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(connected, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	for(int idx = 0; idx >= 0; idx = hierarchy[idx][0]){
		Rect rect = boundingRect(contours[idx]);

		Mat maskROI(mask, rect);
		maskROI = Scalar(0, 0, 0);
		drawContours(mask, contours, idx, Scalar(255, 255, 255), CV_FILLED);
		RotatedRect rrect = minAreaRect(contours[idx]);
		double r = (double)countNonZero(maskROI) / (rrect.size.width * rrect.size.height);

		Scalar color;
		int thickness = 1;
		if (r > 0.25 && (rrect.size.height > 8 && rrect.size.width > 8)){
			thickness = 2;
			color = Scalar(0, 255, 0);
			Rect r = rrect.boundingRect();
			Rect bounds(0,0,large.cols,large.rows);
			Mat crop = large(r & bounds);
			Mat cropR;
			resize(crop,cropR,size);
			frameList.push_back(cropR);
			Point2f pts[4];
			rrect.points(pts);
        
			for (int i = 0; i < 4; i++)
			{
				line(copy, Point((int)pts[i].x, (int)pts[i].y), Point((int)pts[(i+1)%4].x, (int)pts[(i+1)%4].y), color, thickness);
			}
        }        
    }
	imshow("segmentation_text", copy);
	waitKey(0);
	destroyWindow("segmentation_text");
    return frameList;
}

////////////////////////////////////////////////////
/// Main
///
/// Funzione che gestisce la memoria condivisa tra 
/// il segmentation e la rete neurale.
///////////////////////////////////////////////////

int main() {
	
	int mc_fd_one, mc_fd_two;
	char * ptr_one, * ptr_two;
	
	mc_fd_one = shm_open(MEMORY_ONE, O_RDWR, 0666);
	if(mc_fd_one == -1) {
		cerr << "Fallimento nell'APERTURA della memoria condivisa 1" << endl;
		exit(-1);
	}
	mc_fd_two = shm_open(MEMORY_TWO, O_RDWR, 0666);
	if(mc_fd_one == -1) {
		cerr << "Fallimento nell'APERTURA della memoria condivisa 2" << endl;
		exit(-1);
	}
	
	ptr_one = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mc_fd_one, 0);
	if(ptr_one == MAP_FAILED) {
		cout << "Fallimento nella MAPPATURA della memoria condivisa 1" << endl;
		exit(-1);
	}
	ptr_two = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mc_fd_two, 0);
	if(ptr_one == MAP_FAILED) {
		cout << "Fallimento nella MAPPATURA della memoria condivisa 2" << endl;
		exit(-1);
	}

	sem_t * sem_id_one = sem_open(SEMAPHORE_ONE, 0);
	sem_t * sem_id_two = sem_open(SEMAPHORE_TWO, 1);

	cout << "SEGMENTATION avviato..." << endl;
	while(true) {
		sleep(1);
		if(strcmp(ptr_one, "?") != 0) {
			// ---------------------------------------------------------
			sem_wait(sem_id_one);
			cout << "--- Start -> SEZIONE CRITICA 1! ---" << endl;

			// Lista di tutte le immagini contententi testo
			list<Mat> text = detect_text(ptr_one);
			list<Mat>::iterator it;
			
			for (it=text.begin(); it != text.end(); it++){
				sleep(2);

				// -----------------------------------------------------
				sem_wait(sem_id_two);
				cout << "--- Start -> SEZIONE CRITICA 2! ---" << endl;

				// Scrivo la memoria convidisa 2
				ftruncate(mc_fd_two, 300*200*CHANNELS);
				memcpy(ptr_two, it->data, 300*200*CHANNELS);

				sem_post(sem_id_two);
				cout << "--- Stop -> SEZIONE CRITICA 2! ---" << endl;
				// -----------------------------------------------------
			}

			// Pulisco la memoria convidisa 1
			char message_clean1[] = "?";
			ftruncate(mc_fd_one, strlen(message_clean1));
			sprintf(ptr_one, "%s", message_clean1);

			sem_post(sem_id_one);
			cout << "--- Stop -> SEZIONE CRITICA 1! ---" << endl;
			// ---------------------------------------------------------
		}
	}

	return 0;
}
