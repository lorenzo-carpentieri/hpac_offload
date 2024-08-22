## Introduction

This directory contains a set of Python scripts to run the accuracy/performance trade-off experiments for the benchmarks and approximation techniques presented in the paper. The goal is to provide an easy and flexible way to run experiments and store the results in a database for further analysis. This document will guide you through the process of generating and running experiments.

## Directory Structure

The main directory contains the following files and folders:

- `config_all.yaml`: Configuration file containing information about each benchmark and the experimental space for each technique.
- `gen_experiments.py`: Python script to generate experimental configuration files.
- `run_experiment.py`: Python script to run a single experiment based on the provided configuration.
- `create_db.py`: Python script to create the SQLite database for storing experiment results.
- `print_db.py`: Python script to display the contents of the SQLite database.
- `results_all.sqlite3`: SQLite database to store the results of the experiments.
- `experiment_config/`: Directory containing CSV files with experimental configurations for each approximation technique.

## Approximation Techniques

Three different approximation techniques are used in this repository:

1. iACT (iact)
2. taf (TAF)
3. perforation (perfo)

## Generating Experiments

First, you need to generate the experiment configuration files for the approximation techniques using the `gen_experiments.py` script. The script requires the following options:

- `--input`: Input file containing configuration.
- `--output`: Output file.
- `--technique`: Approximate technique to generate experiments for. Can be one of: perfo, iact, taf.

Example usage:

```
python3 gen_experiments.py --input input_config.yaml --output experiment_config/experiments_iact_all.csv --technique iact
```

This command will generate a CSV file containing the experiments for the iACT technique.

## Running Experiments

To run an experiment, use the `run_experiment.py` script with the following options:

- `--config_file`: Path to the configuration file (e.g., `config_all.yaml`).
- `--number`: Experiment number from the corresponding configuration file.

Example usage:

The config.yaml file contains the path to the generated file with all configuration for the specified techinique
```
python3 gen_experiments.py --input config_leukocyte.yaml --output experiment_config/experiments_iact_leukocyte.csv --technique iact
python3 run_experiment.py --config_file config_leukocyte.yaml  --number 2  --technique iact
```
s
This command will run the experiment with the specified number from the corresponding configuration file. The results of the experiment will be written to a table in the `results_all.sqlite3` database.

## Creating the Database

If you need to create a new SQLite database to store the experiment results, use the `create_db.py` script:

```
python3 create_db.py --name results.sqlite3
```

This command will create a new SQLite database named `results_all.sqlite3`.

## Viewing the Database Contents

To view the contents of the SQLite database, use the `print_db.py` script. The script provides several options for customizing the output:

- `--input`: Input database filename.
- `--technique`: Approximation technique to print the output for. Must be one of: iact, taf, or perfo.
- `--output`: Output CSV filename.
- `--print_errors`: If specified, prints the table of errors.

Example usage:

```
python3 print_db.py --input results_all.sqlite3 --technique iact --output iact_results.csv 
```

This command will print the contents of the `results_all.sqlite3` database for the iACT technique, and save the output to the `iact_results.csv` file.

## Conclusion

This README should provide you with all the necessary information to generate and run experiments using the provided scripts. By following the instructions, you can explore various approximation techniques and store the results in an organized manner.