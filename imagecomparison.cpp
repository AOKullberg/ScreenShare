/* HEADER
 * Functionality: Can compare two images and returns the group coordinates
 * for every non-equal group. Can also retrieve a single part of the image given by
 * group id. 
 * TODO: Place ALL non-equal parts in a char*. Place all group_ids in a char*. 
 * Place group-size and image-size in a char*, along with number of non-equal groups.*/

#define _CRT_SECURE_NO_WARNINGS
#define PROGRAMFILE "comparison.cl"
#define INPUT_FILE "test.bmp"
#define OUTPUT_FILE "output.bmp"

extern "C" {
#include "bmpfuncs.h"
}

#include "screenshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <iostream>
#include <fstream>
#include <string>

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

using namespace std;

/* Create a device. Lets the user choose GPU or CPU */
cl_device_id create_device() {

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

/* Create program from a file and compile it */
cl_program build_program(cl_context ctx, cl_device_id dev, const char* filename) {
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
	std::free(program_buffer);

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
		std::free(program_log);
		exit(1);
	}
	return program;
}

unsigned char* getBlockData(unsigned char*& image, int block_num , size_t* local_size) {
	unsigned char* tmp = (unsigned char*)malloc(sizeof(unsigned char) * local_size[0] * local_size[1]*4);

	for (int i = 0; i < local_size[0]*4; i+=4) {
		for (int j = 0; j < local_size[1]*4; j +=4) {
			tmp[i*local_size[0] + j] = image[i*local_size[0]*block_num + j];
			tmp[i*local_size[0] + j + 1] = image[i*local_size[0] * block_num + j + 1];
			tmp[i*local_size[0] + j + 2] = image[i*local_size[0] * block_num + j + 2];
			tmp[i*local_size[0] + j + 3] = image[i*local_size[0] * block_num + j + 3];
		}
	}
	return tmp;
}

int main() {
	/* Basic CL structures */
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel *kernels;
	cl_uint num_kernels;
	cl_event prof_event;
	int err;
	size_t global_size[2], local_size[2];

	/* Image data */
	unsigned char* inputImage;
	unsigned char* otherImage;

	cl_image_format img_format;
	cl_mem input_image;
	size_t origin[3], region[3];
	size_t width, height;
	int w, h;
	/*initGDI();
	inputImage = GetScreenshot();
	cout << "Move something now pls" << endl;
	Sleep(3000);
	otherImage = GetScreenshot();
	cout << "Done with screenshot" << endl;*/
	
	inputImage = readRGBImage(INPUT_FILE, &w, &h);
	otherImage = readRGBImage("test2.bmp", &w, &h);
	width = w;
	height = h;
	
	device = create_device();
	cl_ulong mem_size;
	err = clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE,
		sizeof(size_t), &local_size[0], NULL);
	err |= clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE,
		sizeof(cl_ulong), &mem_size, NULL);
	if (err < 0) {
		perror("Couldn't obtain device information");
		exit(1);
	}

	context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
	if (err < 0) {
		cerr << "Couldn't create a context";
		exit(1);
	}

	/* Build the program from PROGRAMFILE */
	program = build_program(context, device, PROGRAMFILE);

	/* Create kernel */
	err = clCreateKernelsInProgram(program, 0, NULL, &num_kernels);
	kernels = (cl_kernel*)malloc(num_kernels * sizeof(cl_kernel));
	clCreateKernelsInProgram(program, num_kernels, kernels, NULL);
	if (err < 0) {
		cerr << "Couldn't create kernels";
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
	cl_mem oth_image;
	input_image = clCreateImage2D(context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&img_format, width, height, 0, (void*)inputImage, &err);
	oth_image = clCreateImage2D(context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&img_format, width, height, 0, (void*)otherImage, &err);
	if (err < 0) {
		perror("Couldn't create the image object");
		exit(1);
	};

	int num_groups = w*h / local_size[0];
	int *fax = (int*)malloc(sizeof(int)*num_groups);
	for (int i = 0; i < num_groups; i++) {
		fax[i] = 0;
	}
	
	cl_mem out;
	out = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 
		sizeof(int)*num_groups, fax, &err);
	if (err < 0) {
		cerr << "Couldn't create buffer";
		exit(1);
	}

	err = clSetKernelArg(kernels[0], 0, sizeof(cl_mem), &input_image);
	err |= clSetKernelArg(kernels[0], 1, sizeof(cl_mem), &oth_image);
	err |= clSetKernelArg(kernels[0], 2, local_size[0] * sizeof(int), NULL);
	err |= clSetKernelArg(kernels[0], 3, sizeof(cl_mem), &out);
	if (err < 0) {
		cerr << "Couldn't set kernel argument";
		exit(1);
	}

	global_size[0] = width; global_size[1] = height;
	local_size[1] = sqrt(local_size[0]); local_size[0] = local_size[1];
	err = clEnqueueNDRangeKernel(queue, kernels[0], 2, NULL, global_size, 
		local_size, 0, NULL, NULL);
	if (err < 0) {
		cerr << "Couldn't enqueue kernel";
		exit(1);
	}
	clFinish(queue);
	err = clEnqueueReadBuffer(queue, out, CL_TRUE, 0,
		sizeof(int)*num_groups, fax, 0, NULL, NULL);
	if (err < 0) {
		cerr << "Couldn't read buffer";
		exit(1);
	}

	unsigned char* block = (unsigned char*)malloc(sizeof(unsigned char) * local_size[0]*local_size[1]*4);

	int groups_per_row = global_size[0] / 16;
	int groups_per_column = global_size[1] / 16;
	int offset = 0;
	
	int count = 0;
	for (int i = 0; i < num_groups; i++) {
		if (fax[i] == 1) {
			count++;
		}
		else {
			offset = floor(i / groups_per_row);
			cout << "Coordinates: (" << offset << "," << i - (offset*groups_per_row) << ")" << endl;
			if (offset == 34 && (i - (offset*groups_per_row)) == 13)
			{
				block = getBlockData(otherImage, i, local_size);
			}
		}
	}
	/*for (int i = 0; i < local_size[0] * local_size[1] * 4; i += 4) {
		cout << "Red: " << (int)block[i] << endl;
		cout << "Green: " << (int)block[i+1] << endl;
		cout << "Blue: " << (int)block[i+2] << endl;
	}*/
	cout << count << " groups are equal" << endl;
	cout << num_groups - count << " groups are not equal" << endl;
	printf("Done.");

	/* Flush input */
	fseek(stdin, 0, SEEK_END);
	getchar();

	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	clReleaseDevice(device);
	for (int i = 0; i < (int)num_kernels; i++) {
		clReleaseKernel(kernels[i]);
	}
	clReleaseProgram(program);
	clReleaseMemObject(input_image);
	clReleaseMemObject(out);
	//clReleaseEvent(prof_event);
}