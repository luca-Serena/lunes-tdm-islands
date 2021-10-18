/*  ##############################################################################################
 *      Advanced RTI System, ARTÌS          http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              For a general introduction to LUNES implmentation please see the
 *              file: mig-agents.c
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h> 
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <ini.h>
#include <ts.h>
#include <rnd.h>
#include <gaia.h>
#include <rnd.h>
#include <values.h>
#include "utils.h"
#include "user_event_handlers.h"
#include "lunes.h"
#include "lunes_constants.h"
#include "entity_definition.h"


/* ************************************************************************ */
/*       L O C A L  V A R I A B L E S                                       */
/* ************************************************************************ */

FILE *         fp_print_trace;        // File descriptor for simulation trace file
unsigned short env_max_ttl = MAX_TTL; // TTL of newly created messages


/* ************************************************************************ */
/*          E X T E R N A L     V A R I A B L E S                           */
/* ************************************************************************ */

extern hash_t hash_table, *table;                   /* Global hash table of simulated entities */
extern hash_t sim_table, *stable;                   /* Hash table of locally simulated entities */
extern double simclock;                             /* Time management, simulated time */
extern TSeed  Seed, *S;                             /* Seed used for the random generator */
extern char * TESTNAME;                             /* Test name */
extern int    NSIMULATE;                            /* Number of Interacting Agents (Simulated Entities) per LP */
extern int    NLP;                                  /* Number of Logical Processes */
// Simulation control
extern int            env_data_mules;               /* Percentage of edge nodes*/
extern int            env_grid_length;              /* Length of sides of the square grid*/
extern int            env_commmunication_distance;  /* Distance within which it is possible to communicate*/
extern int            env_mobility_type;
extern float          env_end_clock;
extern int            env_mule_radius;              /* Number of cells a mule move up and down during its path*/
extern int            env_island_size;              /* Dimension of a side of a squared island*/


extern data_message *  messages;                //array with all the originated messages
extern int messages_counter;                    //counter of the messages
extern int total_hops;

int transmits = 8;                              // probability for a node to send a message
int isHomogeneous = 1;                          // distribution of the population
int counterReachable=0;
int counterUnreachable=0;
int counter = 1;

 


float distance(float diff_x, float diff_y) {
    return( sqrtf( ( diff_x * diff_x ) + ( diff_y * diff_y ) ) );
}


void find_close_nodes (hash_node_t *thisNode){ //Find the nodes that during an epoch are potentially contactable during an epoch
    hash_node_t *node;
    for (int h = 0; h < stable->size; h++) { //considering only the client nodes
        for (node = stable->bucket[h]; node; node = node->next) {
            if ((node->data->x / env_island_size) == (thisNode->data->x / env_island_size) && (node->data->y / env_island_size) == (thisNode->data->y / env_island_size)
                && thisNode->data->key != node->data->key && thisNode->data->num_neighbors < 300 && node->data->key > 2 * env_data_mules){
                thisNode->data->neighbors[thisNode->data->num_neighbors] = node->data->key;
                thisNode->data->num_neighbors++;
            }
        }
    }
}



void lunes_send_discovery_to_neighbors (hash_node_t *node, int muleId){
    hash_node_t *  sender, *receiver; // Sender and receiver nodes in the global hashtable
    int neighs = 0;

    // Dissemination mode for the forwarded messages (dissemination algorithm)
    // The message is forwarded to ALL neighbors of this node
    // NOTE: in case of probabilistic broadcast dissemination this function is called
    //       only if the probabilities evaluation was positive
    
    while (neighs < node->data->num_neighbors ) {
        sender   = hash_lookup(stable, node->data->key);              // This node
        receiver = hash_lookup(table, node->data->neighbors[neighs]); // The neighbor
        execute_discovery(simclock + FLIGHT_TIME, sender, receiver, muleId);       
        neighs++;
    }
}

/****************************************************************************
 *! \brief LUNES_CONTROL: node activity for the current timestep
 * @param[in] node: Node that execute actions
 */
void lunes_user_control_handler(hash_node_t *node) {

	int per_row = (int) (sqrtf(env_data_mules));
    int h;
    hash_node_t * receiver;
    hash_node_t * proxy = hash_lookup(table, env_data_mules*2);


    //*************************************************** INITIAL SETUP***********************************************************************************************//
    if (simclock == BUILDING_STEP){                     //positioning the nodes in the graph
        node->data->messages_carried = 0;
        if (node->data->key < env_data_mules){          //BUS-MULES
            node->data->Xspace = node->data->key / per_row;
            node->data->Yspace = node->data->key % per_row;
            node->data->direction =0;
            node->data->baseX = node->data->Xspace * (env_grid_length/ per_row);
            node->data->baseY = node->data->Yspace * (env_grid_length/ per_row);
            node->data->x = node->data->baseX; 
            node->data->y = node->data->baseY; 
            node->data->status = 2;
            node->data->messages_carried=0;     

        } else if (node->data->key < 2 * env_data_mules){     //mules that bring messages to the proxy
            node->data->status = 3;
            hash_node_t * bus_mule = hash_lookup(table, node->data->key - env_data_mules);
            node->data->direction = 0;                          //direction for mules that bring messages to the proxy  0:stationary    1:way there    2:way back
            node->data->baseX = bus_mule->data->baseX;
            node->data->baseY = bus_mule->data->baseY;
            node->data->x = bus_mule->data->baseX;
            node->data->y = bus_mule->data->baseY;

        } else if (node->data->key == 2 * env_data_mules){      //proxy
            node->data->status = 4;
            node->data->x = env_grid_length/2;
            node->data->y = env_grid_length/2;

        } else {                                               //normal nodes

            if (isHomogeneous == 1){
                node->data->x = RND_Interval(S, 0, env_grid_length);
                node->data->y = RND_Interval(S, 0, env_grid_length);
            } else {
                int rndX = (int) cbrtf (RND_Interval(S, 0, pow(env_grid_length/2, 3))); //  X distance from the center of the grid
                int rndY = (int) cbrtf (RND_Interval(S, 0, pow(env_grid_length/2, 3))); //  Y distance from the center of the grid
                if (node->data->key % 2 == 0){                                          //left or right?                                               
                    node->data->x =  env_grid_length / 2 + (env_grid_length/2 - rndX);
                } else {
                    node->data->x =  env_grid_length / 2 - (env_grid_length/2 - rndX);
                }
                if ((node->data->key / 2) % 2 == 0){                                    // north or south?
                    node->data->y =  env_grid_length / 2 + (env_grid_length/2 - rndY);
                } else {
                    node->data->y =  env_grid_length / 2 - (env_grid_length/2 - rndY);
                }                
            }
            if (node->data->x < 666 && node->data->x > 333 && node->data->y < 666 && node->data->y > 333){
                counter++;
            }
            find_close_nodes(node);
            node->data->status = 0;
            node->data->reachable=0;
        }
    }


//********************************************************FIXED NODES MANAGEMENT **************************************************************************

    //chance for a node to activate (have data to be transmitted to the mule)
    if (simclock > BUILDING_STEP &&  node->data->status == 0 && RND_Interval(S, 0, 10000) < transmits && simclock < (env_end_clock - 10000)){  
        node->data->status = 1;
        if (node->data->reachable == 0){   //se non è già stato constatato che il nodo può contattare un mulo
            node->data->reachable = -1;
        }

        data_message new_message = {.index = messages_counter, .emitted = (int)simclock, .originX = node->data->x, .originY = node->data->y, .state = 'N'};
        messages[messages_counter] = new_message;
        node->data->messages[0] = &messages[messages_counter];
        messages_counter++;
    }
  
 //send message to the proxy

    if (simclock > BUILDING_STEP && node->data->status == 1){
        if (distance( abs(node->data->x - proxy->data->x), abs(node->data->y - proxy->data->y)) <= env_commmunication_distance){
            node->data->messages[0]->delivered = (int)simclock;
            node->data->messages[0]->state = 'I';
            node->data->messages[0]->toBusMule = -1;
            node->data->messages[0]->toProxyMule = -1;
            node->data->status = 0;
            total_hops++;
            node->data->reachable = 1;
            lunes_send_discovery_to_neighbors (node, 2 * env_data_mules);

        } else { //try to contact a mule
            for (h=2 * env_data_mules; h >= 0; h--){
                receiver = hash_lookup(table, h);
                if (distance( abs(node->data->x - receiver->data->x), abs(node->data->y - receiver->data->y)) <= env_commmunication_distance){
                    node->data->reachable = 1;
                    lunes_send_discovery_to_neighbors (node, h);
                    if (receiver->data->messages_carried < 5000){
                        node->data->status = 0;
                        total_hops++;
                        int index = receiver->data->messages_carried;
                        receiver->data->messages[index] = node->data->messages[0];
                        receiver->data->messages_carried++;
                        if (h < env_data_mules){
                            node->data->messages[0]->toBusMule = (int)simclock;
                        } else {
                             node->data->messages[0]->toBusMule = -1;                           
                             node->data->messages[0]->toProxyMule = (int)simclock; 
                        }
                    }
                    break;
                }
            }
        }     
    }





//  ********************************************** MULES MOBILITY ******************************************************************************

    //moving bus-mule depending on its position
    if (simclock > BUILDING_STEP && node->data->status == 2){ 
        //move the bus-mule
        switch (node->data->direction){
            case 0: //go east, then north
                if (node->data->x < (node->data->Xspace + 0.5) * (env_grid_length / per_row)){//} - env_commmunication_distance){  
                    node->data->x = node->data->x + 1;
                } else { //stop moving toward east
                    if (node->data->y <= (node->data->Yspace + 1) * (env_grid_length / per_row) - env_mule_radius){  //still go north
                        node->data->direction = 4;  //
                        node->data->last_y = node->data->y;
                        node->data->y = node->data->y + 1;
                    } else {                                        //Move to the other half   
                        node->data->direction = 1;  //
                        node->data->x +=1;
                    }       
                }
            break;
            
            case 1: // go east, then south
                if (node->data->x < (node->data->Xspace  + 1) * (env_grid_length / per_row)){//- env_commmunication_distance){  
                    node->data->x = node->data->x + 1;
                } else { //stop moving toward east
                        node->data->direction = 5;  //
                        node->data->last_y = node->data->y;
                        node->data->y = node->data->y - 1;
                }
            break;

            case 2: //go west, then north
                if (node->data->x > node->data->Xspace   * (env_grid_length / per_row)){//} + env_commmunication_distance){  
                    node->data->x = node->data->x - 1;
                } else { //stop moving toward west
                        node->data->direction = 4;  //
                        node->data->last_y = node->data->y;
                        node->data->y = node->data->y + 1;  
                }
            break;

            case 3: //go west, then south
                if (node->data->x > (node->data->Xspace + 0.5)  * (env_grid_length / per_row) ){//+ env_commmunication_distance){  
                    node->data->x = node->data->x - 1;
                } else { //stop moving toward west
                    if (node->data->y <= node->data->Yspace  * (env_grid_length / per_row) + env_mule_radius){
                        node->data->direction = 2;  //
                        node->data->x -=1;
                    } else {
                        node->data->direction = 5;  //
                        node->data->last_y = node->data->y;
                        node->data->y = node->data->y - 1;
                    }       
                }
            break;

            case 4: //go north
                if ((node->data->y >= (node->data->Yspace + 1) * (env_grid_length / per_row))){ // already too high
                    if (node->data->x < (node->data->Xspace  + 0.25) * (env_grid_length / per_row) ){
                        node->data->x = node->data->x + 1;
                        node->data->direction = 1;
                    } else {
                        node->data->x = node->data->x - 1;
                        node->data->direction = 2;
                    }       
                }
                else if ((node->data->y - node->data->last_y >= env_mule_radius *2 )){ //go east/west and then north again
                    if (node->data->x < (node->data->Xspace  + 0.5) * (env_grid_length / per_row) ){
                        node->data->x = node->data->x + 1;
                        node->data->direction = 0;
                    } else {
                        node->data->x = node->data->x - 1;
                        node->data->direction = 2;
                    }       
                }
                else { //continue going north
                    node->data->y = node->data->y + 1;
                }
            break;

            case 5: // go south
                if ((node->data->y <= node->data->Yspace  * (env_grid_length / per_row))){ // already too low
                    if (node->data->x < (node->data->Xspace  + 0.75) * (env_grid_length / per_row) ){
                        node->data->x = node->data->x + 1;
                        node->data->direction = 1;
                    } else {
                        node->data->x = node->data->x - 1;
                        node->data->direction = 2;
                    }       
                }
                else if ((node->data->last_y - node->data->y >= env_mule_radius * 2 )){ //go east/west and then south again
                    if (node->data->x <= (node->data->Xspace  + 0.5) * (env_grid_length / per_row) ){
                        node->data->x = node->data->x + 1;
                        node->data->direction = 1;
                    } else {
                        node->data->x = node->data->x - 1;
                        node->data->direction = 3;
                    }       
                }
                else { //continue going south
                    node->data->y = node->data->y - 1;
                }
            break;
        }


        //check if the message can be delivered to the proxy
        if (distance( abs(node->data->x - proxy->data->x), abs(node->data->y - proxy->data->y)) <= env_commmunication_distance){
            for (int i=0; i< node->data->messages_carried; i++) {                                     //all messages from the data-mule delivered to the proxy mule
                node->data->messages[i]->delivered = (int)simclock;
                node->data->messages[i]->toProxyMule = -1;
                node->data->messages[i]->state = 'B';
                total_hops++;
            }   
            node->data->messages_carried = 0;
        }
        
        // check if the proxy-mule has to be activated
        hash_node_t * correspondingMule = hash_lookup (table, node->data->key + env_data_mules);
        if (distance (abs(node->data->x - correspondingMule->data->x), abs(node->data->y - correspondingMule->data->y)) <= env_commmunication_distance && correspondingMule->data->direction < 2){  
            for (int i=0; i< node->data->messages_carried; i++) {                                     //all messages from the local bus-mule delivered to the proxy mule
                int proxyMuleIndex = i + correspondingMule->data->messages_carried;
                correspondingMule->data->messages[proxyMuleIndex] = node->data->messages[i];  
                correspondingMule->data->messages[proxyMuleIndex]->toProxyMule = (int)simclock;
                total_hops++;
            }   
            correspondingMule->data->messages_carried += node->data->messages_carried;
            node->data->messages_carried = 0;
            if (correspondingMule->data->direction == 0){
                correspondingMule->data->direction =1;
            }
        } 


//*********************************************PROXY_MULE****************************************************************************************************************

    } else if (simclock > BUILDING_STEP && node->data->status == 3){         //mobility for mules that bring messages to the proxy  (proxy-mule)

        if (node->data->direction == 1){                                     //way there, approach to the center
            if (node->data->x < env_grid_length/2){
                node->data->x = node->data->x + 1;
            } else if (node->data->x > env_grid_length/2){
                node->data->x = node->data->x -1;
            }
            if (node->data->y < env_grid_length/2){
                node->data->y = node->data->y + 1;
            } else if (node->data->y > env_grid_length/2){
                node->data->y = node->data->y -1;
            }
        } else if (node->data->direction == 2) {                           //way back, return to the base point
            if (node->data->x < node->data->baseX){
                node->data->x = node->data->x + 1;
            } else if (node->data->x > node->data->baseX){
                node->data->x = node->data->x -1;
            }
            if (node->data->y < node->data->baseY){
                node->data->y = node->data->y + 1;
            } else if (node->data->y > node->data->baseY){
                node->data->y = node->data->y -1;
            }
        }

        if (node->data->x == node->data->baseX && node->data->y == node->data->baseY && node->data->direction == 2){
            node->data->direction = 0;                                    //stationary until the bus-mule arrives and delivers the messages
        }

        if (node->data->x == env_grid_length/2 && node->data->y == env_grid_length/2 && node->data->direction == 1){
            node->data->direction = 2;                                    //the proxy-mule starts the way back
            for (int i=0; i < node->data->messages_carried; i++){
                node->data->messages[i]->delivered = (int)simclock;
                node->data->messages[i]->state = 'P';
            }  
            node->data->messages_carried = 0;
        }
    }

        /*STATISTIC*/

    if (simclock == (env_end_clock - 50) && node->data->status <= 1 ){
        if (node->data->reachable == -1){
            counterUnreachable++;
        } else if (node->data->reachable == 1){
            counterReachable++;
        }
    }

    else if (node->data->key == 1 && simclock == (env_end_clock - 49)){
        fprintf(stdout, "reachable: %d unreachable: %d\n", counterReachable, counterUnreachable);
    }
}


void lunes_user_discovery_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    hash_node_t * mule = hash_lookup(stable, msg->discovery.discovery_static.muleId);
    if (node->data->status == 1) {      //if the node has messages to send
        node->data->reachable = 1;
        if (mule->data->status == 4){                                       // it's the proxy  
            node->data->messages[0]->delivered = (int)simclock;
            node->data->messages[0]->state = 'I';
            node->data->messages[0]->toBusMule = -1;
            node->data->messages[0]->toProxyMule = -1;
        } else if (mule->data->status == 2){                                // it's a busMule
            node->data->messages[0]->toBusMule = (int)simclock;
        } else {                                                            // it's a proxyMule
            node->data->messages[0]->toProxyMule = (int)simclock;           
            node->data->messages[0]->toBusMule = -1;
        }                                                                    
        total_hops++;
        
        node->data->messages_carried = 0;
        if (mule->data->messages_carried < 5000 && mule->data->status != 4){
            int index = mule->data->messages_carried;
            mule->data->messages[index] = node->data->messages[0];
            mule->data->messages_carried++;
        }
    }
    node->data->messages_carried = 0;
}
