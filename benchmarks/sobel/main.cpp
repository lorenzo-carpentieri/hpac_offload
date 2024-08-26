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
#include "sobel.h"
#include "kernels.cpp"


static bool compare(const float *refData, const float *data,
                    const int length, const float epsilon = 1e-6f)
{
  float error = 0.0f;
  float ref = 0.0f;
  for(int i = 1; i < length; ++i)
  {
    float diff = refData[i] - data[i];
    //if (diff != 0) printf("mismatch @%d: %f %f\n", i, refData[i] , data[i]);
    error += diff * diff;
    ref += refData[i] * refData[i];
  }
  float normRef = sqrtf((float) ref);
  if (fabs((float) ref) < 1e-7f)
  {
    return false;
  }
  float normError = sqrtf((float) error);
  error = normError / normRef;
  return error < epsilon;
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
  
   int pixelSize = sizeof(uchar4);
   int imageSize = width * height * pixelSize;

  // load image
  std::vector<float4> img;
  load_bitmap_mirrored(filePath, size, img);

  // vector 
  std::vector<uchar4> in_img_vec;
  std::vector<uchar4> out_img_vec;

  out_img_vec.resize(width*height);
  in_img_vec.resize(width*height);
  uchar4 * outputImageData = out_img_vec.data();

  uchar4 * inputImageData = in_img_vec.data();
  
  // convert float4 data to uchar4
  for (int i = 0; i < size*size; i++){
      in_img_vec[i]= convert_uchar4(img[i]);
  }
  
  // create verfication output data
  std::vector<uchar4> ver_output_img;
  ver_output_img.resize(width*height);
  uchar4 *verificationOutput = ver_output_img.data();
  
  

  auto start = std::chrono::steady_clock::now();
   #pragma omp target data map (to: inputImageData[0:width*height]) \
                          map(tofrom: outputImageData[0:width*height])
  {
    int y, x, i ;
    //@APPROX LABEL("sobel_perf") APPROX_TECH(lPerfo | sPerfo)
    #pragma omp target teams distribute parallel for thread_limit(256)
    for (i = 0; i < (height * width); i++){
        // Calculate y and x from the single index i
        y = i / width; // row
        x = i % width; // col
        y+=1;
        x+=1;
        // Skip the boundary pixels
        if (x-1 < 0 || x+1 >= width || y - 1 < 0 || y+1 >= height) continue;

        int c = x + y * width;
        float4 i00 = convert_float4(inputImageData[c - 1 - width]);
        float4 i01 = convert_float4(inputImageData[c - width]);
        float4 i02 = convert_float4(inputImageData[c + 1 - width]);

        float4 i10 = convert_float4(inputImageData[c - 1]);
        float4 i12 = convert_float4(inputImageData[c + 1]);

        float4 i20 = convert_float4(inputImageData[c - 1 + width]);
        float4 i21 = convert_float4(inputImageData[c + width]);
        float4 i22 = convert_float4(inputImageData[c + 1 + width]);

        const float4 two = {2.f, 2.f, 2.f, 2.f};

        float4 Gx = i00 + two * i10 + i20 - i02  - two * i12 - i22;

        float4 Gy = i00 - i20  + two * i01 - two * i21 + i02  -  i22;

        /* taking root of sums of squares of Gx and Gy */
        outputImageData[c] = convert_uchar4({sqrtf(Gx.x*Gx.x + Gy.x*Gy.x)/2.f,
                                             sqrtf(Gx.y*Gx.y + Gy.y*Gy.y)/2.f,
                                             sqrtf(Gx.z*Gx.z + Gy.z*Gy.z)/2.f,
                                             sqrtf(Gx.w*Gx.w + Gy.w*Gy.w)/2.f});
      }
    }
  

  auto end = std::chrono::steady_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  printf("Average kernel execution time: %f (s)\n", (time * 1e-9f));

  // reference implementation
  reference(verificationOutput, inputImageData, width, height, pixelSize);

  float *outputDevice = (float*) malloc (imageSize * sizeof(float));
  if (outputDevice == NULL)
    printf("Failed to allocate host memory! (outputDevice)");

  float *outputReference = (float*) malloc (imageSize * sizeof(float));

  if (outputReference == NULL)
    printf("Failed to allocate host memory!" "(outputReference)");

  // copy uchar data to float array
  for(int i = 0; i < (int)(width * height); i++)
  {
    outputDevice[i * 4 + 0] = outputImageData[i].x;
    outputDevice[i * 4 + 1] = outputImageData[i].y;
    outputDevice[i * 4 + 2] = outputImageData[i].z;
    outputDevice[i * 4 + 3] = outputImageData[i].w;

    outputReference[i * 4 + 0] = verificationOutput[i].x;
    outputReference[i * 4 + 1] = verificationOutput[i].y;
    outputReference[i * 4 + 2] = verificationOutput[i].z;
    outputReference[i * 4 + 3] = verificationOutput[i].w;
  }
  // for(int i = 0; i < height*width;i++){
  //   std::cout<< static_cast<int>(outputImageData[i].x) << " "<<  static_cast<int>(outputImageData[i].y)<< " "<< static_cast<int>(outputImageData[i].z) << std::endl;
  // }
  
  // Save the output image to a file
  // the save bitmap work with float4 so I have to convert the data
  std::vector<float4> out_img;
  out_img.resize(width*height);
  // convert float data to uchar
  for (int i = 0; i < size*size; i++){
      out_img[i]= convert_float4(outputImageData[i]);
  }
  save_bitmap(fileOutPath, size, out_img);
  // compare the results and see if they match
  if(compare(outputReference, outputDevice, imageSize))
    printf("PASS\n");
  else
    printf("FAIL\n");

  return 0;
}
