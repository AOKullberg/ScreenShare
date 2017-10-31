extern "C" {
#include "bmpfuncs.h"
}

#include "WinScreen.h"
#include <SFML\Graphics.hpp>
#include <SFML\Window.hpp>
#include <iostream>
#include <time.h>
using namespace std;
unsigned char* getBlockData(unsigned char*& image, int block_num, size_t* local_size);

unsigned char** getNonEqualBlocks(unsigned char*& image, unsigned short* block_ids, int num_blocks_nonequal, size_t* local_size) {
	size_t block_size = local_size[0] * local_size[1] * 4;
	//unsigned char** tmp = (unsigned char**)malloc(sizeof(unsigned char*) * block_size * num_blocks_nonequal);
	unsigned char** tmp = (unsigned char**)malloc(sizeof(unsigned char*) * num_blocks_nonequal);
	int offset = 0;
	for (int i = 0; i < num_blocks_nonequal; i++) {
		offset = block_size * i;
		tmp[i] = getBlockData(image, block_ids[i], local_size);
	}
	return tmp;
}

unsigned char * getBlockData(unsigned char *& image, int block_num, size_t * local_size)
{
	int width = 512;
	int height = 512;
	int groups_per_row = width / local_size[0];
	int groups_per_col = height / local_size[1];
	int row = (int)floor(block_num / groups_per_row);
	int col = block_num - row*groups_per_row;

	int block_size = local_size[0] * local_size[1];
	unsigned char* tmp = (unsigned char*)malloc(sizeof(unsigned char) * block_size * 4);

	/* This offset is crazy but to explain it briefly:
	* First you need to identify which row and column the group is at and how
	* many groups per row there is. This way you can make the initial offset to come
	* to the correct row by doing 4*block_size*row*groups_per_row. This sets you on the
	* right row, (*4 for RGBA values). To then arrive at the right column you need to take
	* the column_id multiplied by the width of a group times 4 (RGBA).
	* Finally, to be able to iterate over the entire group, offset by multiplying i with the width
	* of the picture. This will iterate over the rows while "j" iterates over the columns. */
	for (size_t i = 0; i < local_size[1] * 4; i += 4) {
		for (size_t j = 0; j < local_size[0] * 4; j += 4) {
			tmp[i*local_size[0] + j] = image[(i*width) + (4 * block_size * row * groups_per_row) +
				(col * local_size[0] * 4) + j];
			tmp[i*local_size[0] + j + 1] = image[(i*width) + (4 * block_size * row * groups_per_row) +
				(col * local_size[0] * 4) + j + 1];
			tmp[i*local_size[0] + j + 2] = image[(i*width) + (4 * block_size * row * groups_per_row) +
				(col * local_size[0] * 4) + j + 2];
			tmp[i*local_size[0] + j + 3] = image[(i*width) + (4 * block_size * row * groups_per_row) +
				(col * local_size[0] * 4) + j + 3];
		}
	}
	return tmp;
}

void insertPixels(sf::Texture& texture, unsigned char** data, size_t* local_size, unsigned short* block_nums, int num_blocks) {
	int width = 512;
	int height = 512;
	int groups_per_row = width / local_size[0];
	int groups_per_col = height / local_size[1];
	for (int i = 0; i < num_blocks; i++) {
		int row = (int)floor(block_nums[i] / groups_per_row);
		int col = block_nums[i] - row*groups_per_row;
		texture.update(data[i], local_size[0], local_size[1], col*local_size[0], row*local_size[1]);
	}
}

int main() {
	
	sf::Texture texture;
	Screen screen;
	size_t* local_size = (size_t*)malloc(sizeof(size_t) * 2);
	local_size[0] = 16;
	local_size[1] = 16;
	
	unsigned char* img;
	unsigned char* img2;
	//img = screen.getScreenshot();
	int w, h;
	
	img = readRGBImage("test.bmp", &w, &h);
	img2 = readRGBImage("test2.bmp", &w, &h);

	texture.loadFromFile("test.bmp", sf::IntRect(0, 0, 512, 512));
	texture.create(w,h);
	texture.update(img);
	sf::Sprite sprite(texture);
	sf::RenderWindow window(sf::VideoMode(200, 200), "Screenshare");
	int window_width = window.getSize().x;
	int window_height = window.getSize().y;
	int texture_width = texture.getSize().x;
	int texture_height = texture.getSize().y;

	sprite.setScale({ ((float)window_width / texture_width), ((float)window_height / texture_height) });
	bool first = true;
	
	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.draw(sprite);
		window.display();
		//lock to 30-ish fps
		Sleep(33);

		if (first) {
			unsigned short* block_ids = (unsigned short*)malloc(sizeof(unsigned short) * 2);
			block_ids[0] = 176; block_ids[1] = 178;
			unsigned char** tmp = getNonEqualBlocks(img2, block_ids, 2, local_size);
			insertPixels(texture, tmp, local_size, block_ids, 2);
			first = false;
		}
		
		//unsigned char* tmp = getBlockData(img2, 176, local_size);
		
		//window.display();
	}

	return 0;
}