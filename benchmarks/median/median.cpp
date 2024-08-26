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
#include <algorithm>
#define RADIUS 11

#pragma omp declare target
void swap(float4 A[], int i, int j) {
  float val_i = (A[i].x + A[i].y + A[i].z) / 3;
  float val_j = (A[j].x + A[j].y + A[j].z) / 3;

  if(val_i > val_j) {
    float4 temp = A[i];
    A[i] = A[j];
    A[j] = temp;
  }
}
#pragma omp end declare target
#pragma omp declare target

int max(int a , int b){return (a>b ? a : b);};
int min(int a, int b){return (a<b? a : b);};

 float4 filter( const float4 *in, float4 center, int x, int y, int size){
  int k = 0;
  float4 window[9];
  for(int i = -1; i<2; i++)
      for(int j = -1; j<2; j++) {
          uint xs = min(max(x+j, 0), static_cast<int>(size-1)); // borders are handled here with extended values
          uint ys = min(max(y+i, 0), static_cast<int>(size-1));
          window[k] =in[xs*size+ys];
          k++;
      }
  

  swap(window, 0, 1);
  swap(window, 2, 3);
  swap(window, 0, 2);
  swap(window, 1, 3);
  swap(window, 1, 2);
  swap(window, 4, 5);
  swap(window, 7, 8);
  swap(window, 6, 8);
  swap(window, 6, 7);
  swap(window, 4, 7);
  swap(window, 4, 6);
  swap(window, 5, 8);
  swap(window, 5, 7);
  swap(window, 5, 6);
  swap(window, 0, 5);
  swap(window, 0, 4);
  swap(window, 1, 6);
  swap(window, 1, 5);
  swap(window, 1, 4);
  swap(window, 2, 7);
  swap(window, 3, 8);
  swap(window, 3, 7);
  swap(window, 2, 5);
  swap(window, 2, 4);
  swap(window, 3, 6);
  swap(window, 3, 5);
  swap(window, 3, 4);
  float4 val = window[4];
  return val;
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
    //@APPROX LABEL("median_all_perf") APPROX_TECH(lPerfo | sPerfo)
    #pragma omp target teams distribute parallel for thread_limit(256)
    for (i = 0; i < (height * width); i++){
        int x = i / height;
        int y = i % width;
        int gid = x*width+y;
        
        float4 window[9];
        int k = 0;
        // outputImageData[gid] = filter(inputImageData, inputImageData[gid], x, y, height);

        // Optimization note: this array can be prefetched in local memory, TODO
        //@APPROX LABEL("median_in") APPROX_TECH(MEMO_IN) IN(inputImageData[gid], window[k]) OUT(outputImageData[gid])
        //@APPROX LABEL("median_out") APPROX_TECH(MEMO_OUT) IN(inputImageData[gid], window[k]) OUT(outputImageData[gid])             
        {
        for(int i = -1; i<2; i++)
            for(int j = -1; j<2; j++) {
                uint xs = std::min(std::max(x+j, 0), static_cast<int>(height-1)); // borders are handled here with extended values
                uint ys = std::min(std::max(y+i, 0), static_cast<int>(width-1));
                window[k] =inputImageData[xs*width+ys];
                k++;
            }
        
    
	        swap(window, 0, 1);
          swap(window, 2, 3);
          swap(window, 0, 2);
          swap(window, 1, 3);
          swap(window, 1, 2);
          swap(window, 4, 5);
          swap(window, 7, 8);
          swap(window, 6, 8);
          swap(window, 6, 7);
          swap(window, 4, 7);
          swap(window, 4, 6);
          swap(window, 5, 8);
          swap(window, 5, 7);
          swap(window, 5, 6);
          swap(window, 0, 5);
          swap(window, 0, 4);
          swap(window, 1, 6);
          swap(window, 1, 5);
          swap(window, 1, 4);
          swap(window, 2, 7);
          swap(window, 3, 8);
          swap(window, 3, 7);
          swap(window, 2, 5);
          swap(window, 2, 4);
          swap(window, 3, 6);
          swap(window, 3, 5);
          swap(window, 3, 4);
          float4 val = window[4];
       
          outputImageData[gid] = val;
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
