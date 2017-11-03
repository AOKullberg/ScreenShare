#include "imgCompare.h"
#include <vector>

class Network {
public:
	Network() {}
	~Network() {}

	void addClient();
	void releaseNetwork();
	void sendImageData(unsigned char* image, blockInfo info);

private:
};