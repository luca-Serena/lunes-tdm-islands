/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file is defined the state of the simulated entitiesFORKI
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __ENTITY_DEFINITION_H
#define __ENTITY_DEFINITION_H
//#define HIERARCHY

#include "lunes_constants.h"


/*---- E N T I T I E S    D E F I N I T I O N ---------------------------------*/

/*! \brief Structure of "value" in the hash table of each node
 *         in LUNES used to implement neighbors and its properties
 */
typedef struct v_e {
    unsigned int value;                     // Value
} value_element;

/*! \brief Records composing the local state (dynamic part) of each SE
 *         NOTE: no duplicated keys are allowed
 */
struct state_element {
    unsigned int  key;      // Key
    value_element elements; // Value
};

typedef struct data_message {
	int index;
	int emitted;
	int originX;
	int originY;
	int toLocalMule;
	int toRadialMule;
	int delivered;
	char state;           //N --> not delivered      I --> Immediately           P  --> Proxy-mule           B  --> Bus-mule
} data_message;

/*! \brief SE state definition */
typedef struct hash_data_t {
    int           key;                    // SE identifier
    int           lp;                     // Logical Process ID (that is the SE container)
    int	   x;
    int	   y;
    int	   last_y;
    int	   status;		    // 0 --> node	 1 --> node with message to send	 2 --> bus	 3 --> mule that bring messages to proxy     4 --> proxy
    int           reachable;
    int 	   Xspace;
    int	   Yspace;
    int	   baseX;		    //X of data mule base point
    int	   baseY;	            //Y of data mule base point
    int 	   direction;
    int	   neighbors [300];         
    int	   num_neighbors;
    int	   messages_carried;			//
    data_message* messages[5000];
} hash_data_t;


#endif /* __ENTITY_DEFINITION_H */
