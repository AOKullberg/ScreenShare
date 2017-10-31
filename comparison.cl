__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | 
      CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST; 

__kernel void compare_images(read_only image2d_t imageOne,
							read_only image2d_t imageTwo,
							__local int* item_cmp,
							__global ushort* out) {
	int2 coord = (int2)(get_global_id(0), get_global_id(1));

	float4 pixelOne = read_imagef(imageOne, sampler, coord);
	float4 pixelTwo = read_imagef(imageTwo, sampler, coord);

	int4 comparison = isequal(pixelOne,pixelTwo);

	if(all(comparison)) {
		item_cmp[get_local_id(1) * get_local_size(0) + get_local_id(0)] = 0;
	}
	else {
		item_cmp[get_local_id(1)*get_local_size(0) + get_local_id(0)] = 1;
	}
	barrier(CLK_LOCAL_MEM_FENCE);

	//Just make the first work item do this job
	if(get_local_id(0) == 0 && get_local_id(1) == 0) {
		int check = 1;
		// If all the pixels in this group are the same, check remains 1.
		for(int i = 0; i < get_local_size(0)*get_local_size(1); i++) {
			if(item_cmp[i] == 1) {
				check = 0;
				break;
			}
		}
		
		if(check) {
			out[get_group_id(1) * (get_global_size(0)/get_local_size(0)) + get_group_id(0)] = 1;
		}
		else {
			out[get_group_id(1) * (get_global_size(0)/get_local_size(0)) + get_group_id(0)] = 0;
		}
	}
}