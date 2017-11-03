#include "Client.h"

#include <iostream>

#include <SFML\Graphics.hpp>
#include <SFML\Window.hpp>
//extern bool endProgram;
bool endProgram = false;

sf::Packet& operator <<(sf::Packet& packet, const unsigned short* data) {
	int i = 0;
	bool first = true;
	while (data[i] != 0 || first) {
		packet << data[i++];
		first = false;
	}
	return packet << data[i];
}

sf::Packet& operator >> (sf::Packet& packet, unsigned short* data) {
	int i = 0;
	sf::Uint16 tmp = 1;
	bool first = true;
	while (tmp != 0) {
		packet >> tmp;
		data[i++] = tmp;
		if (first) {
			tmp = 1;
			first = false;
		}
	}
	return packet;
}

void clientLoop(unsigned short port, sf::IpAddress address) {
	sf::Texture texture;
	Client client;
	if (client.initNetwork(port, address) != 0) {
		perror("Could not initiate network, restart application.");
	}
	blockInfo image_info;
	unsigned char* image;

	sf::RenderWindow window(sf::VideoMode(100, 100), "Screenshare");
	while (!endProgram) {
		client.receive_data();
		image_info = client.get_info();
		image = client.get_image();
		//std::cout << image_info.block_ids[1];
	}
}

int Client::initNetwork(unsigned short new_port, sf::IpAddress new_address)
{
	port = new_port;
	address = new_address;
	if (socket.bind(port) != sf::Socket::Done) {
		return 1;
	}
}

void Client::releaseNetwork()
{
	socket.unbind();
}

void Client::receive_data()
{
	sf::Packet packet;
	socket.receive(packet, address, port);
	if (initiated) {
		packet >> (char*)image_info.blockData >> image_info.num_blocks_nonequal >> image_info.block_ids;
	}
	else {
		packet >> (char*)image >> image_info.image_height >> image_info.image_width >> image_info.block_size;
		initiated = true;
	}
}

void Client::update_image() {

}

blockInfo Client::get_info()
{
	return blockInfo();
}

unsigned char * Client::get_image()
{
	return image;
}
