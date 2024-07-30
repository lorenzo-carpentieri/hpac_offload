#include <stdio.h>
#include <iostream>
#include <stdlib.h>

#define N 128
#define DATA_TYPE int
#include "omp.h"
int foo (DATA_TYPE val){
    return val * 2;
}
int main (int argc, char * argv[]){
    DATA_TYPE *a = (DATA_TYPE *) malloc(sizeof(DATA_TYPE)*N);
    DATA_TYPE *b = (DATA_TYPE *) malloc(sizeof(DATA_TYPE)*N);
    DATA_TYPE alpha=2;
    
    #pragma omp target teams 
    // #pragma omp parallel
    {
        if (omp_is_initial_device()) {
            printf("Running on host\n");    
        } else {
            int nteams= omp_get_num_teams(); 
            int nthreads= omp_get_num_threads();
            printf("Running on device with %d teams in total and %d threads in each team\n",nteams,nthreads);
        }
    }

    
    #pragma omp target teams distribute parallel for map(from:a[:N]) map(from:b[:N])
    for(int i = 0; i < N; i++){
        a[i]=i+1;
        b[i]=i+1;
    }

    #pragma omp target data map(from:a[:N]) map(from:b[:N])
    {
        int i = 0;
        // #pragma approx perfo(small:4)
        #pragma omp target teams distribute parallel for
        for(i = 0; i < N; i++){
            a[i]= a[i] * alpha + b[i];
        }
    }
    
    std::cout<< a[0] <<std::endl;
    
   
 
}