# LUNES-Blockchain

### Large Unstructured NEtwork Simulator (LUNES) for TruDaMule protocol
This _LUNES_ implementation is used to simulate TruDaMule

## Compilation

Run `make` inside this folder to compile the binary

## Usage

To start the run use `run` Bash script: this will take care of sourcing and setting all global variables.

```
USAGE: ./run --nodes|-n #SMH  [--debug|-d DEBUGCMD]
	#SMH	   total number of nodes to simulate
	[DEBUGCMD] used for injecting *trace commands (optional)
```

For example to run a simple simulation execute `./run -n 10000`. 

## Important paramters to set
Some important paramters the users con edit to configure the simulation are located in the file `run`

- END_CLOCK 			 	number of discrete time-steps of the simulation
export DATA_MULES 			number of both local mule and radial mule in the simulation. It is suggested to put a squared number for this parameter
export GRID_LENGTH			dimension (in terms of cells) of a side of a grid
export COMMUNICATION_DISTANCE		distance (in terms of cells) within which wireless communication is ensured
export MULE_RADIUS			to set the width of the zigzags in the path of local mules
export COURIERS				number of couriers
export ISLAND_SIZE			dimension (in terms of cells) of the islands in the grid
