# hpac_offload_artifact
Evaluation Artifact for HPAC-Offload (https://github.com/LLNL/HPAC/tree/hpac_offload)

## Building the HPAC-Offload Compiler and Runtime System, Downloading Input Data
Before running any experiments, the HPAC-Offload compiler and runtime system must be built, and the input data used for the analsysis downloaded. We have provided the script `setup.sh` to perform both actions. In the root direcotry of this repository, simply run `./setup.sh $NTHREADS`, where NTHREADS will be used to build the compiler. On an AMD Epyc system using 64 threads, the build takes about 1 hour. Build times will vary with the sytem architecture and the number of threads used.

**All following instructions assume that the build is complete, and that the directory `HPAC` exists in the root directory of the repository.

## Runtime Expectations
The following lists the expected time needed to perform the accuracy/performance trade-off experiments for two different configurations: small and large. The small configuration runs only those data points shown in the paper, where we take the fastest and slowest 10% of all experiments for 10 different x-axis intervals. The large configuration runs experiments for the entire Cartesian space.


| Benchmark       | Time to Run (NVIDIA Large, hours) | Time to Run (AMD Large, hours) | Time to Run (NVIDIA Small, hours) | Time to Run (AMD Small, hours) |
| --------------- | ---------------------------------- | ------------------------------ | ---------------------------------- | ------------------------------ |
| blackscholes    | 0.12                               | 0.09                           | 0.03                               | 0.03                           |
| binomialoptions | 0.45                               | 0.19                           | 0.13                               | 0.05                           |
| kmeans          | 987.61                             | 493.02                         | 218.87                             | 121.19                         |
| lavaMD          | 6.58                               | 4.64                           | 1.29                               | 0.96                           |
| leukocyte       | 1.35                               | 1.31                           | 0.26                               | 0.25                           |
| lulesh          | 55.83                              | 59.13                          | 11.63                              | 13.13                          |
| miniFE          | 53.91                              | 0.00                           | 14.38                              | 0.00                           |
| **Total**       | **1105.86**                        | **558.37**                     | **246.59**                          | **135.61**                     |

## NVIDIA 
### Performance/Accuracy Trade-Off
To run the NVIDIA performance/accuracy trade-off experiments, first build the HPAC-Offload compiler using the above steps.
Then, enter the directory `experiments/nvidia/tradeoff_space`. In this directory, you'll see two YAML configuration files: `config_all.yaml` and `config_small.yaml`. These files correspond to the large and small configurations, respectively. Files containing the individual experimental configuration for all benchmarks and each approximation technique are in the `experiment_config` directory.
For more information on running these experiments, see the [README](/experiments/nvidia/tradeoff_space/README.md) in the NVIDIA directory.