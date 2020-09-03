# Image-Edge-Detection-Raspberry

### Utilizing a Raspberry Pi3 B+ to take pictures in real time (making use of a Raspberry Pi Camera v2), process the image with the Sobel operator for edge detection and sending them to a client computer.

## Task

Using a Raspberry Pi3 Model B+ and a Raspberry Pi Camera v2 implement an application capable of taking photos when a client connects to the server, process them the Sobel operator for edge detection and send them to the client computer utilizing the TCP/IP protocol all in C language.

## Functionality

This app has two main parts, the client and the server.

The Raspberry acts as the server taking photos whenever a new client connects to it through a socket using the TCP/IP protocol. It internally keeps listening to a specific port for any new connection, takes a photo issuing the command to the Camera called ''raspistill''. After that it processes the image applying the Sobel operator for edge detection and sends the processed image to the client.

On the client side we only have to run the program, provided that we have the correct port specified, and immediately will receive the processed image.

## Test

An image before applying the algorithm:

![Image before algorithm](https://github.com/howlettjames/Image-Edge-Detection-Raspberry/blob/master/HPC_Image/calle1.bmp "Image before algorithm")

After applying the algorithm:

![Image after algorithm](https://github.com/howlettjames/Image-Edge-Detection-Raspberry/blob/master/HPC_Image/calle11.bmp "Image after algorithm")

An actual image taken with a Raspberry Pi Camera v2 and processed with the algorithm during a test:

![Actual test image](https://github.com/howlettjames/Image-Edge-Detection-Raspberry/blob/master/Cliente/recv.bmp "Actual test image")
