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
#include "chrono"
#define FILTER_WIDTH 5
#define FILTER_HEIGHT 5

#pragma omp declare target
float filter(float* in, float center, int x, int y, int width) {
    float sod = 0;
    if((x>=FILTER_WIDTH/2) && (x<(width-FILTER_WIDTH/2)) && (y>=FILTER_HEIGHT/2) && (y<(width-FILTER_HEIGHT/2))){

      for (int ky = -FILTER_HEIGHT / 2; ky <= FILTER_HEIGHT / 2; ky++) {
          for (int kx = -FILTER_WIDTH / 2; kx <= FILTER_WIDTH / 2; kx++) {
              float fl = in[((y + ky) * width) + (x + kx)];
              sod += fl - center;
          }
      }
    }
    return sod;
}
float sum(float data){
  return data + 1;
}
#pragma omp end declare target


int main(int argc, char * argv[])
{
  if (argc != 3) {
    printf("Usage: %s <path to input bmp img> <out img>\n", argv[0]);
    return 1;
  }
  const char* filePath = argv[1];
  const char* fileOutPath = argv[2];
  // squared image
  const int width = 3072;
  const int height = width;
  int size = width;
  

  // load image
  std::vector<float4> in_img;
  std::vector<float4> out_img;
  out_img.resize(width*height);
  load_bitmap_mirrored(filePath, size, in_img);

  std::vector<float> in_img_grayscale;
  in_img_grayscale.resize(width*height);
  
	// Create pointer to data and converts image to grayscale
  in_img_grayscale = rgbToGrayScale<float4, float>(in_img, size);
  // for(int i = 0; i < size*size; i++)
  //   std::cout<< "r: " << in_img_grayscale[i] << " g: " << in_img_grayscale[i]<< " b: " << in_img_grayscale[i]  << std::endl;

  float *inputImageData = in_img_grayscale.data();
  
  std::vector<float> out_img_grayscale;
  out_img_grayscale.resize(width*height);
  float * outputImageData = out_img_grayscale.data();

  auto start = std::chrono::steady_clock::now();
  const int img_size = (width*height);
  #pragma omp target data map (to: inputImageData[0:img_size]) \
                          map(tofrom: outputImageData[0:(img_size)]) 
  {
    int  i=0;
    //@APPROX LABEL("tv_perf") APPROX_TECH(lPerfo | sPerfo)
    #pragma omp target teams distribute parallel for thread_limit(256)
    for (i = 0; i < (height * width); i++){
				//TODO: implement tv filter with openmp GPU offloading
        const int y = i / height;
        const int x = i % width;
        const int gid = y*width+x;

      
        float center = 0;
        float sod = 0;
        center = inputImageData[gid];
        // Does not work with big image due to illigal memory access
        //@APPROX LABEL("entire_memo_in") APPROX_TECH(MEMO_IN) IN(inputImageData[gid]) OUT(outputImageData[gid])
        //@APPROX LABEL("entire_memo_out") APPROX_TECH(MEMO_OUT) IN(inputImageData[gid]) OUT(outputImageData[gid])
        // When there are if statement we can have an infinite loop with memoization
        outputImageData[gid] = filter(inputImageData, inputImageData[gid], x, y, width);
      }
  
    }

  auto end = std::chrono::steady_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  printf("Average kernel execution time: %f (s)\n", (time * 1e-9f));

  
  // for(int i = 0; i < height*width;i++){
  //   std::cout<< static_cast<int>(outputImageData[i].x) << " "<<  static_cast<int>(outputImageData[i].y)<< " "<< static_cast<int>(outputImageData[i].z) << std::endl;
  // }
  
  // Save the output image to a file
  // the save bitmap work with float4 so I have to convert the data
  out_img = grayScaleToRgb<float4, float>(out_img_grayscale, size);
  // for(int i = 0; i < size*size; i++)
  //   std::cout<< "r: " << out_img[i].x << " g: " << out_img[i].y << " b: " << out_img[i].z  << std::endl;
  save_bitmap(fileOutPath, size, out_img);

  // compare the results and see if they match
  // if(compare(outputReference, outputDevice, imageSize))
  //   printf("PASS\n");
  // else
  //   printf("FAIL\n");

  return 0;
}
