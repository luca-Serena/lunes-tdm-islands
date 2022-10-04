# LUNES

### Large Unstructured NEtwork Simulator (LUNES) for InDaMule protocol
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
-  DATA_MULES 				number of both local mule and radial mule in the simulation. It is suggested to put a squared number for this parameter
- GRID_LENGTH				dimension (in terms of cells) of a side of a grid
- COMMUNICATION_DISTANCE		distance (in terms of cells) within which wireless communication is ensured
- MULE_RADIUS				to set the width of the zigzags in the path of local mules
- COURIERS				number of couriers
- ISLAND_SIZE				dimension (in terms of cells) of the islands in the grid


## Compilation

Run `make` inside this folder to compile the binary: `blockchain`.

## Usage

To start the run use `run` Bash script: this will take care of sourcing and setting all global variables.

This script is used to start the SImulator MAnager (_SIMA_) and the binary `blockchain` with all necessary arguments.

```
USAGE: ./run --nodes|-n #SMH [--debug|-d DEBUGCMD]
	#SMH	   total number of nodes to simulate
	[DEBUGCMD] used for injecting *trace commands (optional)
```

For example to run a simple simulation execute `./run -n 10000`. This will prints all logs in the file`testmob.txt`. To change location where logs are printed one can change the output path in the file `run`



## Contacts

Luca Serena <luca.serena2@unibo.it>

Gabriele D'Angelo: <g.dangelo@unibo.it>

Mirko Zichichi: <mirko.zichichi@upm.es>

Stefano Ferretti: <stefano.ferretti@uniurb.it>
