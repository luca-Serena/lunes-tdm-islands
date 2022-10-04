# LUNES

### Large Unstructured NEtwork Simulator (LUNES) for InDaMule protocol
This _LUNES_ implementation is used to simulate InDaMule


LUNES depends on some external libraries that have to be installed for
a correct compilation and execution:
1)	GLib		(libglib2.0-dev)
2)	igraph		(libigraph0-dev)
3)	GNU awk		(gawk)

## Compilation

Run `make` inside this folder to compile the binary

## Usage

To start the run use `run` Bash script: this will take care of sourcing and setting all global variables.

This script is used to start the SImulator MAnager (_SIMA_) and the binary `blockchain` with all necessary arguments.

```
USAGE: ./run --nodes|-n #SMH [--debug|-d DEBUGCMD]
	#SMH	   total number of nodes to simulate
	[DEBUGCMD] used for injecting *trace commands (optional)
```

For example to run a simple simulation execute `./run -n 10000`. This will prints all logs in the file`testmob.txt`. To change location where logs are printed one can change the output path in the file `run`

## Important paramters to set
Some important paramters the users con edit to configure the simulation are located in the file `run`

- END_CLOCK 			 	number of discrete time-steps of the simulation
-  DATA_MULES 				number of both local mule and radial mule in the simulation. It is suggested to put a squared number for this parameter
- GRID_LENGTH				dimension (in terms of cells) of a side of a grid
- COMMUNICATION_DISTANCE		distance (in terms of cells) within which wireless communication is ensured
- MULE_RADIUS				to set the width of the zigzags in the path of local mules
- COURIERS				number of couriers
- ISLAND_SIZE				dimension (in terms of cells) of the islands in the grid


##License

This software can be used ONLY for EDUCATIONAL or RESEARCH purposes. In 
both cases the results (i.e. reports or research papers) should clearly 
report the credits to this software. Any commercial use is expressly 
forbidden without prior written consent from the authors.

You may NOT modify, adapt, translate, reverse engineer, decompile, 
disassemble this software to create derivative works based on this 
software package. The software may not be transferred or commercialized 
in any way to anyone without the prior written consent of the authors.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

## Contacts

Luca Serena <luca.serena2@unibo.it>

Gabriele D'Angelo: <g.dangelo@unibo.it>

Mirko Zichichi: <mirko.zichichi@upm.es>

Stefano Ferretti: <stefano.ferretti@uniurb.it>
