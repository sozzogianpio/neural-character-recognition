# Neural Character Recognition
In this project it's implemented a neural network that recognizes handwritten characters acquired by a camera. Image acquisition, image segmentation, neural network and graphical outputs are managed by different processes that share the memory.

## About the Neural Network
Tesseract is an optical character recognition engine. It is free software, released under the Apache License and development has been sponsored by Google since 2006. For more information visit: https://github.com/tesseract-ocr/tesseract

## Software Architeture
![Software Architecture](https://i.postimg.cc/t48ty7sd/ncr.jpg)

## Install
First of all, you have to download this repository. Use `git clone https://github.com/sozzogianpio/neural-character-recognition.git` to do it. After that, you have to install *Tesseract OCR*, *OpenCV* and their dependecies.

## How to run
You must have `g++` compiler installed.

Move into folder where files are in and **firstly** compile and run `printer.cpp`:
```
g++ printer.cpp -o printer -lrt -pthread
./printer
```

Then compile and run `segmentation.cpp`:
```
g++ segmentation.cpp -o segmentation -lrt -pthread
./segmentation
```

Then compile and run `neural_network.cpp`:
```
g++ neural_network.cpp -o neural_network -lrt -pthread
./neural_network
```

At last, compile `webcam.cpp` and run when you want to capture an image:
```
g++ webcam.cpp -o webcam -lrt -pthread
./webcam
```

## Authors
- Gianpio Sozzo
- Gianluca Bonifazi
- Mattia Campeggi
