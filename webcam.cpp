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
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include <typeinfo>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <string>

using namespace std;

#define MEMORY_ONE  "/shm_1"        // WEBCAM -> SEGMENTATION
#define SEMAPHORE_ONE "/sem_1"
#define SIZE 4000096
#define HEIGHT 720
#define WIDTH 1024
#define CHANNELS 3

////////////////////////////////////////////////////
/// Cattura
///
/// Funzione che scatta una foto dalla webcam e 
/// ritorna il buffer contenente il frame cattuato
///////////////////////////////////////////////////

char * cattura(){
    
    // 1.  Apre il device
    int fd;
    fd = open("/dev/video0",O_RDWR);
    if(fd < 0){
        perror("Failed to open device, OPEN");
        exit(-1);
    }

    // 2. Chiede al device se può scattare una foto
    v4l2_capability capability;
    if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0){
        perror("Failed to get device capabilities, VIDIOC_QUERYCAP");
        exit(-1);
    }

    // 3. Imposta il formato dell'immagine
    v4l2_format imageFormat;
    imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    imageFormat.fmt.pix.width = WIDTH;
    imageFormat.fmt.pix.height = HEIGHT;
    imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    imageFormat.fmt.pix.field = V4L2_FIELD_NONE;
    if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
        perror("Device could not set format, VIDIOC_S_FMT");
        exit(-1);
    }

    // 4. Richiede il buffer del device
    v4l2_requestbuffers requestBuffer = {0};
    requestBuffer.count = 1;
    requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestBuffer.memory = V4L2_MEMORY_MMAP;

    if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0){
        perror("Could not request buffer from device, VIDIOC_REQBUFS");
        exit(-1);
    }

    // 5. Chiede al buffer di passargli l'immagine raw
    v4l2_buffer queryBuffer = {0};
    queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queryBuffer.memory = V4L2_MEMORY_MMAP;
    queryBuffer.index = 0;
    if(ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0){
        perror("Device did not return the buffer information, VIDIOC_QUERYBUF");
        exit(-1);
    }
    char* buffer = (char*)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, queryBuffer.m.offset);
    memset(buffer, 0, queryBuffer.length);
    
    // 6. Prende il frame dal buffer
    v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));
    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;
    
    // Attiva lo streaming
    int type = bufferinfo.type;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
        perror("Could not start streaming, VIDIOC_STREAMON");
        exit(-1);
    }
    
    // Mette in coda il buffer
    if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0){
        perror("Could not queue buffer, VIDIOC_QBUF");
        exit(-1);
    }
    
     // Toglie dalla coda il buffer
    if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0){
        perror("Could not dequeue the buffer, VIDIOC_DQBUF");
        exit(-1);
    }
		
	// Termina lo streaming
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
        perror("Could not end streaming, VIDIOC_STREAMOFF");
        exit(-1);;
    }

    close(fd);

    return buffer;
}

////////////////////////////////////////////////////
/// Main
///
/// Funzione che gestisce la memoria condivisa tra 
/// la webcam e il segmentation.
///////////////////////////////////////////////////

int main() {
    	
    int mc_fd_one;
    char * ptr_one;
	
    mc_fd_one = shm_open(MEMORY_ONE, O_RDWR, 0666);
    if(mc_fd_one == -1) {
        cerr << "Fallimento nell'apertura della memoria condivisa 1" << endl;
        exit(-1);
    }

    ptr_one = (char*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mc_fd_one, 0);
    if(ptr_one == MAP_FAILED) {
        cout << "Fallimento nella mappatura della memoria condivisa 1" << endl;
        exit(-1);
    }

    sem_t * sem_id_one = sem_open(SEMAPHORE_ONE, 1);
    
    cout << "WEBCAM avviata..." << endl;
    // -----------------------------------------------------------------
    sem_wait(sem_id_one);
    cout << "--- Start -> SEZIONE CRITICA 1! ---" << endl;

    char * buffer = cattura();
    ftruncate(mc_fd_one, WIDTH*HEIGHT*CHANNELS);
    memcpy(ptr_one, buffer, WIDTH*HEIGHT*CHANNELS);
    cout << "**** FOTO effetuata! ****" << endl;
    
    sem_post(sem_id_one);
    cout << "--- Stop  -> SEZIONE CRITICA 1! ---" << endl;
    // -----------------------------------------------------------------
    cout << "WEBCAM disattivata..." << endl;

    return 0;
}