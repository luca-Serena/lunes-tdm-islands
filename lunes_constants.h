/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              Model level parameters for LUNES
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __LUNES_CONSTANTS_H
#define __LUNES_CONSTANTS_H

//  General parameters
#define TOPOLOGY_GRAPH_FILE        "test-graph-cleaned.dot" // Graph definition to be used for network construction
#define MAX_TTL                    10                       // TTL of new messages, standard value
#define MEAN_NEW_MESSAGE           2000                     // Generation of new transactions and checks: exponential distribution, mean value
#define PERC_GENERATORS            100.00                   // Percentage of nodes that generate new messages

//	Dissemination protocols
#define BROADCAST                  0    // Probabilistic broadcast
#define DIRECTIONED		    1   

#endif /* __LUNES_CONSTANTS_H */
