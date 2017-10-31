#include "imgCompare.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

void imgCompare::setImage(int selector, unsigned char*& image)
{
	if (selector == 0) {
		imageOne = image;
	}
	else {
		imageTwo = image;
	}
}

int imgCompare::initCL()
{
	device = create_device();
	
	err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
		sizeof(size_t), &local_size[0], NULL);
	err |= clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,
		sizeof(cl_ulong), &mem_size, NULL);
	if (err < 0) {
		perror("Couldn't obtain device information");
		exit(1);
	}

	global_size[0] = width; global_size[1] = height;
	local_size[1] = (size_t)sqrt(local_size[0]); local_size[0] = local_size[1];

	/* Create context */
	context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if (err < 0) {
		std::cerr << "Couldn't create a context";
		exit(1);
	}

	/* Build the program from PROGRAMFILE */
	program = build_program(context, device, programfile);

	/* Create kernel */
	err = clCreateKernelsInProgram(program, 0, NULL, &num_kernels);
	kernels = (cl_kernel*)malloc(num_kernels * sizeof(cl_kernel));
	clCreateKernelsInProgram(program, num_kernels, kernels, NULL);
	if (err < 0) {
		std::cerr << "Couldn't create kernels";
		exit(1);
	}

	/* Create a command queue */
	queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
	if (err < 0) {
		cerr << "Couldn't create a command queue";
		exit(1);
	}

	/* Create image object */
	img_format.image_channel_order = CL_RGBA;
	img_format.image_channel_data_type = CL_UNORM_INT8;

	return err;
}

void imgCompare::releaseCL() {
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	clReleaseDevice(device);
	for (int i = 0; i < (int)num_kernels; i++) {
		clReleaseKernel(kernels[i]);
	}
	clReleaseProgram(program);
}

void imgCompare::setProgramFile(const char * file)
{
	programfile = file;
}

void imgCompare::setDimensions(int new_width, int new_height)
{
	width = new_width;
	height = new_height;
}

blockInfo imgCompare::compare()
{
	if (imageOne == NULL || imageTwo == NULL) {
		throw (runtime_error("Images not set"));
	}

	blockInfo result;

	image_one = clCreateImage2D(context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&img_format, width, height, 0, (void*)imageOne, &err);
	image_two = clCreateImage2D(context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&img_format, width, height, 0, (void*)imageTwo, &err);
	if (err < 0) {
		cerr << "Couldn't create the image object";
		exit(1);
	};

	int num_groups = width*height / (local_size[0]*local_size[1]);
	unsigned short *block_ids = (unsigned short*)malloc(sizeof(cl_ushort)*num_groups);
	for (int i = 0; i < num_groups; i++) {
		block_ids[i] = 0;
	}

	cl_mem out;
	out = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR,
		sizeof(cl_ushort)*num_groups, block_ids, &err);
	if (err < 0) {
		cerr << "Couldn't create buffer";
		exit(1);
	}

	err = clSetKernelArg(kernels[0], 0, sizeof(cl_mem), &image_one);
	err |= clSetKernelArg(kernels[0], 1, sizeof(cl_mem), &image_two);
	err |= clSetKernelArg(kernels[0], 2, local_size[0]*local_size[1] * sizeof(int), NULL);
	err |= clSetKernelArg(kernels[0], 3, sizeof(cl_mem), &out);
	if (err < 0) {
		cerr << "Couldn't set kernel argument";
		exit(1);
	}

	err = clEnqueueNDRangeKernel(queue, kernels[0], 2, NULL, global_size,
		local_size, 0, NULL, NULL);
	if (err < 0) {
		cerr << "Couldn't enqueue kernel";
		exit(1);
	}
	clFinish(queue);
	err = clEnqueueReadBuffer(queue, out, CL_TRUE, 0,
		sizeof(cl_ushort)*num_groups, block_ids, 0, NULL, NULL);
	if (err < 0) {
		perror("Couldn't read buffer");
		exit(1);
	}

	unsigned short* non_equal_block_id = (unsigned short*)malloc(sizeof(unsigned short)*num_groups);

	int groups_per_row = global_size[0] / 16;
	int groups_per_column = global_size[1] / 16;
	int offset = 0;

	int count = 0;
	int j = 0;
	for (int i = 0; i < num_groups; i++) {
		if (block_ids[i] == 1) {
			count++;
		}
		else {
			non_equal_block_id[j++] = i;
			offset = (int)floor(i / groups_per_row);
			cout << "Coordinates: (" << offset << "," << i - (offset*groups_per_row) << ")" << endl;
		}
	}

	unsigned char** blocks = (unsigned char**)malloc(sizeof(unsigned char*) * (num_groups - count));
	blocks = getNonEqualBlocks(imageTwo, non_equal_block_id, num_groups - count, local_size);

	result.image_width = width;
	result.image_height = height;
	result.block_size = local_size[0] * local_size[1] * 4;
	result.block_ids = non_equal_block_id;
	result.num_blocks_nonequal = num_groups - count;
	result.blockData = blocks;
	
	clReleaseMemObject(out);
	clReleaseMemObject(image_one);
	clReleaseMemObject(image_two);
	return result;
}

cl_device_id imgCompare::create_device()
{
	cl_platform_id* platforms;
	cl_uint num_platforms;
	cl_device_id dev{};
	int err;

	/* Identify the number of platforms */
	err = clGetPlatformIDs(3, NULL, &num_platforms);
	if (err < 0) {
		cerr << "Couldn't identify a platform";
		exit(1);
	}

	/* Retrieve all available platforms */
	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*num_platforms);
	err = clGetPlatformIDs(num_platforms, platforms, NULL);
	if (err < 0) {
		cerr << "Couldn't access any platforms";
		exit(1);
	}
	string input{};
	cout << "Please specify what type of device you wish to use (CPU OR GPU): ";
	cin >> input;

	if (input == "GPU") {
		/* Looks through all platforms for a GPU */
		for (size_t i = 0; i < num_platforms; i++) {
			err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
			if (err == CL_SUCCESS) {
				break;
			}
		}
		/* If a GPU is not found in any platform, it looks through them again.
		* This time in search of a CPU. */
		if (err == CL_DEVICE_NOT_FOUND) {
			cout << "Could not find a GPU, using CPU instead\n";
			for (size_t i = 0; i < num_platforms; i++) {
				err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
				if (err == CL_SUCCESS) {
					break;
				}
			}
		}
	}
	else {
		/* Looks through all platforms for a CPU */
		for (size_t i = 0; i < num_platforms; i++) {
			err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 1, &dev, NULL);
			if (err == CL_SUCCESS) {
				break;
			}
		}
		/* If a CPU is not found in any platform, it looks through them again.
		* This time in search of a GPU. */
		if (err == CL_DEVICE_NOT_FOUND) {
			cout << "Could not find a CPU, using GPU instead\n";
			for (size_t i = 0; i < num_platforms; i++) {
				err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 1, &dev, NULL);
				if (err == CL_SUCCESS) {
					break;
				}
			}
		}
	}

	/* If no devices are found/accessible it exits the application. */
	if (err == CL_DEVICE_NOT_FOUND) {
		cerr << "Couldn't access any devices";
		exit(1);
	}
	char device_name[80];
	err = clGetDeviceInfo(dev, CL_DEVICE_NAME, sizeof(device_name), &device_name, 0);
	cout << "Using " << device_name << ".\n";
	return dev;
}

cl_program imgCompare::build_program(cl_context ctx, cl_device_id dev, const char* filename)
{
	ifstream textfile;
	cl_program program;
	char *program_log;
	size_t program_size, log_size;
	int err;

	/* Read program file and place content into buffer */
	std::fstream kernelFile(filename);
	std::string content(
		(std::istreambuf_iterator<char>(kernelFile)),
		std::istreambuf_iterator<char>()
	);

	program_size = content.size();
	char* program_buffer = new char[program_size + 1];
	strcpy(program_buffer, content.c_str());
	program_buffer[program_size] = '\0';

	/* Create program from file */
	program = clCreateProgramWithSource(ctx, 1,
		(const char**)&program_buffer, &program_size, &err);
	if (err < 0) {
		cerr << "Couldn't create the program";
		exit(1);
	}
	free(program_buffer);

	/* Build program */
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (err < 0) {

		/* Find size of log and print to std output */
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
			0, NULL, &log_size);
		program_log = (char*)malloc(log_size + 1);
		program_log[log_size] = '\0';
		clGetProgramBuildInfo(program, dev, CL_PROGRAM_BUILD_LOG,
			log_size + 1, program_log, NULL);
		cout << program_log << endl;
		free(program_log);
		exit(1);
	}
	return program;
}

unsigned char** imgCompare::getNonEqualBlocks(unsigned char*& image, unsigned short* block_ids, int num_blocks_nonequal, size_t* local_size) {
	size_t block_size = local_size[0] * local_size[1] * 4;
	unsigned char** tmp = (unsigned char**)malloc(sizeof(unsigned char*) * num_blocks_nonequal);
	int offset = 0;
	for (int i = 0; i < num_blocks_nonequal; i++) {
		offset = block_size * i;
		tmp[i] = getBlockData(image, block_ids[i], local_size);
	}
	return tmp;
}

unsigned char * imgCompare::getBlockData(unsigned char *& image, int block_num, size_t * local_size)
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
