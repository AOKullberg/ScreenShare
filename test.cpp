#define _CRT_SECURE_NO_WARNINGS
extern "C" {
#include "bmpfuncs.h"
}

#include <iostream>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#include "AppScreen.h"
#elif defined __unix__
#include "LinScreen.h"
#elif defined _WIN32
#include <CL/cl.h>
#include "WinScreen.h"
#endif
//using namespace std;
#include <SFML\Network.hpp>
#include <iostream>
#include "imgCompare.h"

/* Global variables */
bool endProgram = false;

void connectToServer(sf::UdpSocket &socket, sf::IpAddress address, unsigned short port) {
	std::cout << "Connecting to " << address << " on port " << port << "." << std::endl;
	socket.bind(port);
	sf::Packet packet;
	sf::IpAddress ad = sf::IpAddress::Broadcast;
	socket.setBlocking(true);
	if (socket.receive(packet, ad, port) != sf::Socket::Done) {
		perror("Could not receive data.");
	}
}

void broadcastScreen(sf::UdpSocket &socket, sf::Packet data, int port) {
	sf::IpAddress ad;
	std::cout << "Broadcasting on " << ad.Broadcast << "." << std::endl;
	socket.send(data, ad.Broadcast, port);
	std::cout << "Completed sending" << std::endl;
}

int main() {
	sf::TcpSocket socket;
	sf::UdpSocket udpSocket;
	imgCompare compare;
	blockInfo result;
	Screen screen;
	unsigned char* imageOne;
	
	unsigned char* imageTwo;
	unsigned char* screenTest;
	sf::Packet packet;
	int w, h;

	imageOne = readRGBImage("test.bmp", &w, &h);
	packet << imageOne << w << h;

	std::cout << "Choose to send or receive: ";
	std::string command, ip, port;
	int port_num = 8080;
	getline(std::cin, command);
	if (command == "receive") {
		std::cout << "Specify the ip of the host (xxx.xxx.x.x): ";
		getline(std::cin, ip);
		std::cout << "Specify the port of the host, leave blank for default (default 8080): ";
		getline(std::cin, port);
		if (port != "") {
			port_num = stoi(port);
		}
	}
	else if (command == "send") {
		std::cout << "Specify the port, leave blank for default (default 8080): ";
		getline(std::cin, port);
		if (port != "") {
			port_num = stoi(port);
		}
		sf::IpAddress ad;
		std::cout << "Local IP: " << ad.getLocalAddress().toString() << std::endl;
		std::cout << "Public IP: " << ad.getPublicAddress().toString() << std::endl;
		std::cout << "Broadcast IP: " << ad.Broadcast << std::endl;
		std::cout << "Sending on port " << port_num << std::endl;
	}
	
	if (command == "receive") {
		connectToServer(udpSocket, (sf::IpAddress)ip, port_num);
	}
	else if (command == "send") {
		broadcastScreen(udpSocket, packet, port_num);
	}
	
	fseek(stdin, 0, SEEK_END);
	getchar();

	/*
	imageOne = readRGBImage("test.bmp", &w, &h);
	imageTwo = readRGBImage("test2.bmp", &w, &h);
	compare.setDimensions(w, h);

	compare.setProgramFile("comparison.cl");
	compare.setImage(0, imageOne);
	compare.setImage(1, imageTwo);

	if (compare.initCL() < 0) {
		perror("Couldn't initiate CL");
	}
	try {
		result = compare.compare();
	}
	catch (std::runtime_error &e) {
		perror(e.what());
	}

	screenTest = screen.getScreenshot();
	Dims dimensions = screen.getDims();
	
	storeRGBImage(screenTest, "out.bmp", dimensions.height, dimensions.width, "refscreen.bmp");

	fseek(stdin, 0, SEEK_END);
	getchar();

	endProgram = true;
	compare.releaseCL();
	free(result.blockData);
	free(imageOne);
	free(imageTwo);*/
}