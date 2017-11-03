#ifndef CLIENT_H
#define CLIENT_H

#include <SFML\Network.hpp>
#include "imgCompare.h"

void clientLoop(unsigned short port, sf::IpAddress address);

/* Class for setting up client side of the application */

class Client {
public:
	Client() {}
	~Client() {}
	
	int initNetwork(unsigned short new_port, sf::IpAddress new_address);
	void releaseNetwork();

	void receive_data();
	void update_image();

	blockInfo get_info();
	unsigned char* get_image();
	

private:
	sf::UdpSocket socket;
	unsigned short port;
	sf::IpAddress address;
	bool initiated = false;

	blockInfo image_info;
	unsigned char* image;
};



#endif