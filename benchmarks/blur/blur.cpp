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
#define RADIUS 11

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
  in_img.resize(width*height);
  out_img.resize(width*height);

  load_bitmap_mirrored(filePath, size, in_img);

  // Base pointer to input and output image
  float4 * inputImageData = in_img.data();
  float4 * outputImageData = out_img.data();

  
  

  auto start = std::chrono::steady_clock::now();
   #pragma omp target data map (to: inputImageData[0:(width*height)]) \
                          map(tofrom: outputImageData[0:(width*height)])
  {
    int i;
    //@APPROX LABEL("blur_all_perf") APPROX_TECH(lPerfo | sPerfo)
    #pragma omp target teams distribute parallel for thread_limit(256)
    for (i = 0; i < (height * width); i++){
        int x = i / height;
        int y = i % width;
        int gid = x*width+y;
        if(x < height && y < width) {
          // Perforate the filter computation o
          #ifdef PERFO
          float4 sum_neigh{0.0f,0.0f,0.0f,0.0f};
          int hits = 0;
          //@APPROX LABEL("blur_filter_perf") APPROX_TECH(lPerfo | sPerfo)
          for(int ox = -RADIUS; ox < RADIUS + 1; ++ox) {
            for(int oy = -RADIUS; oy < RADIUS + 1; ++oy) {
              // image boundary check
              if((x + ox) > -1 && (x + ox) < height && (y + oy) > -1 && (y + oy) < width) {
                sum_neigh.x += inputImageData[(x + ox)*width + y + oy].x;
                sum_neigh.y += inputImageData[(x + ox)*width + y + oy].y;
                sum_neigh.z += inputImageData[(x + ox)*width + y + oy].z;
                hits++;
              }
            }
          }
          float4 mean_neigh{sum_neigh.x / hits, sum_neigh.y / hits, sum_neigh.z / hits, 0};
          outputImageData[gid] = mean_neigh;
          #else
          //@APPROX LABEL("blur_in") APPROX_TECH(MEMO_IN) IN(inputImageData[gid]) OUT(outputImageData[gid])
          //@APPROX LABEL("blur_out") APPROX_TECH(MEMO_OUT) IN(inputImageData[gid]) OUT(outputImageData[gid])          
          outputImageData[gid] =filter(inputImageData, inputImageData[gid], x,y, height);
          #endif
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
