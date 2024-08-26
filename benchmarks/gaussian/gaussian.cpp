/**********************************************************************
  Copyright �2013 Advanced Micro Devices, Inc. All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  �   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  �   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/


#include <omp.h>
#include "bitmap.h"
#include <chrono>
#include <math.h>
#define RADIUS 11
#define KERNEL_SIZE 7


#pragma omp declare target
float4 filter(float4* in, float4 center, int x, int y, int size) {
  float4 sum_neigh{0.0f,0.0f,0.0f,0.0f};
  int hits = 0;
  for(int ox = -RADIUS; ox < RADIUS + 1; ++ox) {
    for(int oy = -RADIUS; oy < RADIUS + 1; ++oy) {
      // image boundary check
      if((x + ox) > -1 && (x + ox) < size && (y + oy) > -1 && (y + oy) < size) {
        sum_neigh.x += in[(x + ox)*size + y + oy].x;
        sum_neigh.y += in[(x + ox)*size + y + oy].y;
        sum_neigh.z += in[(x + ox)*size + y + oy].z;
        hits++;
      }
    }
  }
  float4 mean_neigh{sum_neigh.x / hits, sum_neigh.y / hits, sum_neigh.z / hits, 0};
  return mean_neigh;
}
#pragma omp end declare target

std::vector<float> populate_blur_kernel(std::vector<float> out_kernel){
  float scaleVal = 1;
  float stDev = (float)KERNEL_SIZE/3;

  for (int i = 0; i < KERNEL_SIZE; ++i) {
    for (int j = 0; j < KERNEL_SIZE; ++j) {
      float xComp = pow((i - KERNEL_SIZE/2), 2);
      float yComp = pow((j - KERNEL_SIZE/2), 2);

      float stDevSq = pow(stDev, 2);
      float pi = M_PI;

      //calculate the value at each index of the Kernel
      float kernelVal = exp(-(((xComp) + (yComp)) / (2 * stDevSq)));
      kernelVal = (1 / (sqrt(2 * pi)*stDev)) * kernelVal;

      //populate Kernel
      out_kernel[i*KERNEL_SIZE+j] = kernelVal;

      if (i==0 && j==0) 
      {
              scaleVal = out_kernel[0];
      }

      //normalize Kernel
      out_kernel[i*KERNEL_SIZE+j] = out_kernel[i*KERNEL_SIZE+j] / scaleVal;
    }
  }
  return out_kernel;
}

int main(int argc, char * argv[])
{
  if (argc != 3) {
    printf("Usage: %s <path to input bmp img> <out img>\n", argv[0]);
    return 1;
  }
  const char* filePath = argv[1];
  const char* fileOutPath = argv[2];
  // squared image
  int width = 3072;
  int height = width;
  int size = width;

  // load image
  std::vector<float4> in_img, out_img;
  std::vector<float> kernel;
  kernel.resize(KERNEL_SIZE*KERNEL_SIZE);
  
  in_img.resize(width*height);
  out_img.resize(width*height);
  
  kernel = populate_blur_kernel(kernel);

  load_bitmap_mirrored(filePath, size, in_img);

  // Base pointer to input and output image
  float4 * inputImageData = in_img.data();
  float4 * outputImageData = out_img.data();
  float * kernel_ptr = kernel.data();

  
  

  auto start = std::chrono::steady_clock::now();
   #pragma omp target data map (to: inputImageData[0:(width*height)]) \
                          map (to: kernel_ptr[0:(KERNEL_SIZE*KERNEL_SIZE)])\
                          map(tofrom: outputImageData[0:(width*height)])
  {
    int i;
    //@APPROX LABEL("gaussian_all_perf") APPROX_TECH(lPerfo | sPerfo)
    #pragma omp target teams distribute parallel for thread_limit(256)
    for (i = 0; i < (height * width); i++){
        int x = i / height;
        int y = i % width;  
        int gid = x*width+y;
        
				if (x < height  && y < width) {
					float kernelSum=0;
					float redPixelVal=0;
					float greenPixelVal=0;
					float bluePixelVal=0;
          int i = 0, j = 0;
          
          //@APPROX LABEL("gaussian_in") APPROX_TECH(MEMO_IN) IN(inputImageData[gid], kernel_ptr[i*KERNEL_SIZE+j]) OUT(outputImageData[gid])
          //@APPROX LABEL("gaussian_out") APPROX_TECH(MEMO_OUT) IN(inputImageData[gid], kernel_ptr[i*KERNEL_SIZE+j]) OUT(outputImageData[gid])             
					{
            //Apply Kernel to each pixel of image
            //@APPROX LABEL("gaussian_filter_perf") APPROX_TECH(lPerfo | sPerfo)
            for (i = 0; i < KERNEL_SIZE; ++i) {
              for (j = 0; j < KERNEL_SIZE; ++j) {    
                //check edge cases, if within bounds, apply filter
                if ( ((x + (i - ((KERNEL_SIZE - 1) / 2))) >= 0)
                    && (x + (i - ((KERNEL_SIZE - 1) / 2)) < height)
                    && ((y + j - ((KERNEL_SIZE - 1) / 2)) >= 0)
                    && ((y + j - ((KERNEL_SIZE - 1) / 2)) < width)) {

                    redPixelVal += kernel_ptr[i*KERNEL_SIZE+j] * inputImageData[(x + (i - ((KERNEL_SIZE - 1) / 2)))*width + y + j - ((KERNEL_SIZE - 1) / 2)].x;
                    greenPixelVal += kernel_ptr[i*KERNEL_SIZE+j] * inputImageData[(x + (i - ((KERNEL_SIZE - 1) / 2)))*width + y + j - ((KERNEL_SIZE - 1) / 2)].y;
                    bluePixelVal += kernel_ptr[i*KERNEL_SIZE+j] * inputImageData [(x + (i - ((KERNEL_SIZE - 1) / 2)))*width + y + j - ((KERNEL_SIZE - 1) / 2)].z;
                    kernelSum += kernel_ptr[i*KERNEL_SIZE+j];
                }
              }
            }
          
					//update output image
            
            float4 computed_pixel = {redPixelVal / kernelSum, greenPixelVal / kernelSum, bluePixelVal / kernelSum, 0};
            outputImageData[gid]=computed_pixel;
          }
    }
    }
  }

  auto end = std::chrono::steady_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  printf("Average kernel execution time: %f (s)\n", (time * 1e-9f));




  // copy uchar data to float array
  // for(int i = 0; i < height*width;i++){
  //   std::cout<< static_cast<int>(outputImageData[i].x) << " "<<  static_cast<int>(outputImageData[i].y)<< " "<< static_cast<int>(outputImageData[i].z) << std::endl;
  // }
  
  save_bitmap(fileOutPath, size, out_img);
  // compare the results and see if they match

  return 0;
}
