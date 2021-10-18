# LUNES-Blockchain

### Large Unstructured NEtwork Simulator (LUNES) for Blockchain

This _LUNES_ implementation is used to simulate the Bitcoin's protocol with options to run some known attacks.


## Compilation

Run `make` inside this folder to compile the binary: `blockchain`.

## Usage

To start the run use `run` Bash script: this will take care of sourcing and setting all global variables.

```
USAGE: ./run --nodes|-n #SMH [--test|-t TESTNAME] [--debug|-d DEBUGCMD]
	#SMH	   total number of nodes to simulate
	[DEBUGCMD] used for injecting *trace commands (optional)
```

For example to run a simple simulation execute `./run -n 1000`. 


# LUNES-edge
# lunes-tdm-islands
