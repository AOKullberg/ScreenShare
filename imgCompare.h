#ifndef IMGCOMPARE_H
#define IMGCOMPARE_H

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <string>
#include <vector>

struct blockInfo {
	int image_width;
	int image_height;
	int num_blocks_nonequal;
	int block_size;
	unsigned short* block_ids;
	unsigned char** blockData;
};

class imgCompare {
public:
	imgCompare() {}
	~imgCompare() {}

	void setImage(int selector, unsigned char*& image);
	int initCL();
	void releaseCL();

	void setProgramFile(const char* file);

	void setDimensions(int new_width, int new_height);

	/* Main comparison function. Returns a vector. First index is general info formatted as:
	 * Width, height, number of non-equal blocks, block_ids, block_size.
	 * Second index is non-equal blocks. */
	blockInfo compare();

private:
	/* Private variables */
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel *kernels;
	cl_uint num_kernels;
	int err;
	size_t global_size[2], local_size[2];
	cl_ulong mem_size;

	/* Image data */
	unsigned char* imageOne = NULL;
	unsigned char* imageTwo = NULL;
	
	cl_image_format img_format;
	cl_mem image_one, image_two;
	//size_t origin[3], region[3];
	size_t width, height;

	const char* programfile;

	/* Private methods */
	cl_device_id create_device();
	cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename);

	unsigned char* getBlockData(unsigned char*& image, int block_num, size_t* local_size);
	unsigned char** getNonEqualBlocks(unsigned char*& image, unsigned short* block_ids, int num_blocks_nonequal, size_t* local_size);
};

#endif