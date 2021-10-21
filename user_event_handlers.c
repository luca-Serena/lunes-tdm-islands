/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file you find all the user event handlers to be used to implement a
 *                      discrete event simulation. Only the modelling part is to be inserted in this
 *                      file, other tasks such as GAIA-related data structure management are
 *                      implemented in other parts of the code.
 *              -	Some "support" functions are also present.
 *              -	This file is part of the MIGRATION-AGENTS template provided in the
 *                      ARTÌS/GAIA software distribution but some modifications have been done to
 *                      include the LUNES features.
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
#include "utils.h"
#include "msg_definition.h"
#include "lunes.h"
#include "lunes_constants.h"
#include "user_event_handlers.h"


/* ************************************************************************ */
/*          E X T E R N A L     V A R I A B L E S                           */
/* ************************************************************************ */

extern hash_t hash_table, *table;                   /* Global hash table of simulated entities */
extern hash_t sim_table, *stable;                   /* Hash table of locally simulated entities */
extern double simclock;                             /* Time management, simulated time */
extern TSeed  Seed, *S;                             /* Seed used for the random generator */
extern FILE * fp_print_trace;                       /* File descriptor for simulation trace file */
extern char * TESTNAME;                             /* Test name */
extern int    LPID;                                 /* Identification number of the local Logical Process */
extern int    local_pid;                            /* Process identifier */
extern int    NSIMULATE;                            /* Number of Interacting Agents (Simulated Entities) per LP */
extern int    NLP;                                  /* Number of Logical Processes */
// Simulation control
extern unsigned int   env_migration;                /* Migration state */
extern float          env_migration_factor;         /* Migration factor */
extern unsigned int   env_load;                     /* Load balancing */
extern float          env_end_clock;                /* End clock (simulated time) */
extern int            env_directions_allowed;       /* Dissemination: directioned algorithm */
extern int 			  env_grid_length;				/* Length of sides of the square grid*/
extern int 			  env_commmunication_distance;  /* Distance within which is possible to communicate*/
extern int 			  env_data_mules;				/* number of data mules*/
extern int 			  env_mobility_type; 			/* Mobility algorithm to follow */
extern int            env_mule_radius;              /* Number of cells a mule move up and down during its path*/
extern int            env_island_size;              /* Dimension of a side of a squared island*/
extern int            env_couriers;                 /* Number of couriers */







/* ************************************************************************ */
/*       S U P P O R T     F U N C T I O N S			                    */
/* ************************************************************************ */

/* ***************************** D E B U G **********************************/

/*! \brief Prints out the whole content of a glib hashtable data structure
 */
void UNUSED hash_table_print(GHashTable *ht) {
    // Iterator to scan the whole state hashtable of entities
    GHashTableIter iter;
    gpointer       key, value;

    g_hash_table_iter_init(&iter, ht);

    while (g_hash_table_iter_next(&iter, &key, &value)) {
        fprintf(stdout, "DEBUG: %d:%d\n", *(unsigned int *)key, *(unsigned int *)value);
        fflush(stdout);
    }
}

/*! \brief Returns a random key from a hash table
 */
gpointer  hash_table_random_key(GHashTable *ht) {
    // Iterator to scan the (whole) state hashtable of entities
    GHashTableIter iter;
    gpointer       key, value;
    guint          size;
    unsigned int   position;

    size     = g_hash_table_size(ht);
    position = RND_Integer(S, (double)1, (double)size);

    g_hash_table_iter_init(&iter, ht);

    while (position) {
        g_hash_table_iter_next(&iter, &key, &value);
        position--;
    }
    return(key);
}


/*! \brief Utility to check environment variables, if the variable is not defined then the run is aborted
 */
char *check_and_getenv(char *variable) {
    char *value;

    value = getenv(variable);
    if (value == NULL) {
        fprintf(stdout, "The environment variable %s is not defined\n", variable);
        fflush(stdout);
        exit(1);
    }else {
        return(value);
    }
}


/* *********** E N T I T Y    S T A T E    M A N A G E M E N T **************/

/*! \brief Adds a new entry in the hash table that implements the SE's local state
 *         Note: it is used both from the register and the migration handles
 */


void execute_discovery (double ts, hash_node_t *src, hash_node_t *dest, int muleId) {
    DiscoveryMsg     msg;
    unsigned int message_size;

    // Defining the message type
    msg.discovery_static.type = 'D';
    msg.discovery_static.gatewayId  = src->data->key;
    msg.discovery_static.muleId  = muleId;
    message_size = sizeof(struct _discovery_static_part);

    // Buffer check
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }

    GAIA_Send(src->data->key, dest->data->key, ts, (void *)&msg, message_size);
    
    // Real send

}

/* ************************************************************************ */
/*      U S E R   E V E N T   H A N D L E R S			                    */
/*									                                        */
/*	NOTE: when a handler required extensive modifications for LUNES	        */
/*		then it calls another user level handerl called		                */
/*		lunes_<handler_name> and placed in the file lunes.c	                */

/*****************************************************************************
 *      NOTIFY MIGRATION: a local SE will be migrated in another LP.
 *      This notification is reported to the user level but usually nothing
 *      has to be done
 */
void user_notify_migration_event_handler() {
    // Nothing to do
}

/*****************************************************************************
 *! \brief NOTIFY EXTERNAL MIGRATION: SEs that are allocated in other LPs are going
 *      to be migrated, this LP is notified of this update but the user level
 *      usually does not care of it
 */
void  user_notify_ext_migration_event_handler() {
    // Nothing to do
}


void  user_discovery_event_handler(hash_node_t *node, int forwarder, Msg *msg){
    lunes_user_discovery_event_handler(node, forwarder, msg);
}
/*****************************************************************************
 *! \brief MIGRATION: migration-event manager (the real migration handler)
 *         A new migration message for this LP has been received, the trasported SE has
 *         been created and inserted in the data structures. Now it is necessary to
 *         perform some user level tasks such as taking care of de-serializing the
 *         SE's local state
 */

/*****************************************************************************
 *! \brief CONTROL: at each timestep, the LP calls this handler to permit the execution
 *      of model level interactions, for performance reasons the handler is called once
 *      for all the SE that allocated in the LP
 */
void user_control_handler() {
    int          h;
    hash_node_t *node;

    // Only if in the aggregation phase is finished &&
    // if it is possible to send messages up to the last simulated timestep then the statistics will be
    // affected by some messages that have been sent but with no time to be received
    if ((simclock >= (float)BUILDING_STEP) && (simclock < (env_end_clock))) {
        // For each local SE
        for (h = 0; h < stable->size; h++) {
            for (node = stable->bucket[h]; node; node = node->next) {
               // if (node->data->key <= env_data_mules)
                // Calling the appropriate LUNES user level handler
                lunes_user_control_handler(node);
            } 
        }
    }
}

/*****************************************************************************
 *! \brief USER MODEL: when it is received a model level interaction, after some
 *         validation this generic handler is called. The specific user level
 *         handler will complete its processing
 */
void user_model_events_handler(int to, int from, Msg *msg, hash_node_t *node) {

    // A model event has been received, now calling appropriate user level handler

    // If the node should perform a DOS attack: not a miner and is an attacker
    switch (msg->type) {
    // A transaction message
    case 'D':
        user_discovery_event_handler(node, from, msg);
        break;

    default:
        fprintf(stdout, "FATAL ERROR, received an unknown user model event type: %d\n", msg->type);
        fflush(stdout);
        exit(-1);
    }
}

void user_environment_handler() {
    // ######################## RUNTIME CONFIGURATION SECTION ####################################
    //	Runtime configuration:	migration type configuration
    env_migration = atoi(check_and_getenv("MIGRATION"));
    fprintf(stdout, "LUNES____[%10d]: MIGRATION, migration variable set to %d\n", local_pid, env_migration);
    if ((env_migration > 0) && (env_migration < 4)) {
        fprintf(stdout, "LUNES____[%10d]: MIGRATION is ON, migration type is set to %d\n", local_pid, env_migration);
        GAIA_SetMigration(env_migration);
    }else {
        fprintf(stdout, "LUNES____[%10d]: MIGRATION is OFF\n", local_pid);
        GAIA_SetMigration(MIGR_OFF);
    }

    //	Runtime configuration:	migration factor (GAIA)
    env_migration_factor = atof(check_and_getenv("MFACTOR"));
    fprintf(stdout, "LUNES____[%10d]: MFACTOR, migration factor: %f\n", local_pid, env_migration_factor);
    GAIA_SetMF(env_migration_factor);

    //	Runtime configuration:	turning on/off the load balancing (GAIA)
    env_load = atoi(check_and_getenv("LOAD"));
    fprintf(stdout, "LUNES____[%10d]: LOAD, load balancing: %d\n", local_pid, env_load);
    if (env_load == 1) {
        fprintf(stdout, "LUNES____[%10d]: LOAD, load balancing is ON\n", local_pid);
        GAIA_SetLoadBalancing(LOAD_ON);
    }else {
        fprintf(stdout, "LUNES____[%10d]: LOAD, load balancing is OFF\n", local_pid);
        GAIA_SetLoadBalancing(LOAD_OFF);
    }

    //	Runtime configuration:	number of steps in the simulation run
    env_end_clock = atof(check_and_getenv("END_CLOCK"));
    fprintf(stdout, "LUNES____[%10d]: END_CLOCK, number of steps in the simulation run -> %f\n", local_pid, env_end_clock);
    if (env_end_clock == 0) {
        fprintf(stdout, "LUNES____[%10d]:  END_CLOCK is 0, no timesteps are defined for this run!!!\n", local_pid);
    }

    env_grid_length = atof(check_and_getenv("GRID_LENGTH"));
    fprintf(stdout, "LUNES____[%10d]: GRID SIDES LENGTH -> %d\n", local_pid, env_grid_length);
    if (env_grid_length <= 0) {
        fprintf(stdout, "LUNES____[%10d]: GRID_LENGTH <= 0, error \n", local_pid);
    }

    env_mobility_type = atof(check_and_getenv("MOBILITY_TYPE"));
    fprintf(stdout, "LUNES____[%10d]: MOBILITY_TYPE-> %d\n", local_pid, env_mobility_type);

    env_commmunication_distance = atof(check_and_getenv("COMMUNICATION_DISTANCE"));
    fprintf(stdout, "LUNES____[%10d]: COMMUNICATION_DISTANCE -> %d\n", local_pid, env_commmunication_distance);
    if (env_commmunication_distance <= 0) {
        fprintf(stdout, "LUNES____[%10d]: COMMUNICATION_DISTANCE <= 0, error \n", local_pid);
    }

    env_mule_radius = atof(check_and_getenv("MULE_RADIUS"));
    fprintf(stdout, "LUNES____[%10d]: MULE_RADIUS -> %d\n", local_pid, env_mule_radius);
    if (env_mule_radius <= 0) {
        fprintf(stdout, "LUNES____[%10d]: MULE_RADIUS <= 0, error \n", local_pid);
    }

    env_data_mules =atof(check_and_getenv("DATA_MULES"));
    fprintf(stdout, "LUNES____[%10d]: DATA_MULES -> %d\n", local_pid, env_data_mules);
    if (env_data_mules <= 0) {
        fprintf(stdout, "LUNES____[%10d]: DATA_MULES <= 0, error \n", local_pid);
    }

    env_island_size =atof(check_and_getenv("ISLAND_SIZE"));
    fprintf(stdout, "LUNES____[%10d]: ISLAND_SIZE -> %d\n", local_pid, env_island_size);
    if (env_island_size <= 0) {
        fprintf(stdout, "LUNES____[%10d]: ISLAND_SIZE <= 0, error \n", local_pid);
    }

    env_couriers =atof(check_and_getenv("COURIERS"));
    fprintf(stdout, "LUNES____[%10d]: COURIERS -> %d\n", local_pid, env_couriers);
    if (env_couriers <= 0) {
        fprintf(stdout, "LUNES____[%10d]: COURIERS <= 0, error \n", local_pid);
    }
} 

/*****************************************************************************
 *! \brief BOOTSTRAP: before starting the real simulation tasks, the model level
 *         can initialize some data structures and set parameters
 */
void user_bootstrap_handler() {
    #ifdef TRACE_DISSEMINATION
    char buffer[1024];

    // Preparing the simulation trace file
    sprintf(buffer, "%sSIM_TRACE_%03d.log", TESTNAME, LPID);

    fp_print_trace = fopen(buffer, "w");
    #endif
}

/*****************************************************************************
 *! \brief SHUTDOWN: Before shutting down, the model layer is able to
 *         deallocate some data structures
 */
void user_shutdown_handler() {
    #ifdef TRACE_DISSEMINATION
    char  buffer[1024];
    FILE *fp_print_messages_trace;


    sprintf(buffer, "%stracefile-messages-%d.trace", TESTNAME, LPID);

    fp_print_messages_trace = fopen(buffer, "w");

    //	statistics
    //	total number of trans on the network
    fprintf(fp_print_messages_trace, "M %010lu\n", get_total_sent_trans());
    //	total number of block broadcasted on the network
    fprintf(fp_print_messages_trace, "M %010lu\n", get_total_sent_blocks());

    fclose(fp_print_messages_trace);

    fclose(fp_print_trace);
    #endif
}