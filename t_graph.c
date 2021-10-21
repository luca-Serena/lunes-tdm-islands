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
#include "utils.h"
#include "entity_definition.h"
#include "user_event_handlers.h"

/*-------- G L O B A L     V A R I A B L E S --------------------------------*/

int NSIMULATE,                // Number of Interacting Agents (Simulated Entities) per LP
    NLP,                      // Number of Logical Processes
    LPID,                     // Identification number of the local Logical Process
    local_pid;                // Process Identifier (PID)

double *rates;
// < 0: disabled

// SImulation MAnager (SIMA) information and localhost identifier
static char LP_HOST[64];      // Local hostname
static char SIMA_HOST[64];    // SIMA execution host (fully qualified domain)
static int  SIMA_PORT;        // SIMA execution port number

// Time management variables
double step,                  // Size of each timestep (expressed in time-units)
       simclock        = 0.0; // Simulated time
static int end_reached = 0;   // Control variable, false if the run is not finished

// A single LP is responsible to show the runtime statistics
//  by default the first started LP is responsible for this task
static int LP_STAT = 0;

// Seed used for the random generator
TSeed Seed, *S = &Seed;
/*---------------------------------------------------------------------------*/

// File descriptors:
//  lcr_fp: (output) -> local communication ratio evaluation
//  finished: (output) -> it is created when the run is finished (used for scripts management)
//
FILE *lcr_fp, *finished_fp;

// Output directory (for the trace files)
char *TESTNAME;

// Simulation control (from environment variables, used by batch scripts)
unsigned int   env_migration;                 // Migration state
float          env_migration_factor;          // Migration factor
unsigned int   env_load;                      // Load balancing
float          env_end_clock;                 // End clock (simulated time)
int            applicant;
int            env_data_mules;                /* number of data mules*/
int            env_grid_length;               /* Length of sides of the square grid*/
int            env_commmunication_distance;   /* Distance within which it is possible to communicate*/
int            env_mobility_type;
int            env_mule_radius;               /* Number of cells a mule move up and down during its path*/
int            env_island_size;               /* Dimension of a side of a squared island*/
int            env_couriers;                  /* Number of couriers */

/* ************************************************************************ */
/*                      Hash Tables                                         */
/* ************************************************************************ */

hash_t hash_table, *table = &hash_table; /* Global hash table, contains ALL the simulated entities */
hash_t sim_table, *stable = &sim_table;  /* Local hash table, contains only the locally managed entities */


data_message * messages;
int messages_counter = 0;
int total_hops=1;
/*---------------------------------------------------------------------------*/

/* ************************************************************************ */
/*             Migrating Objects List                                       */
/* ************************************************************************ */

// List containing the objects (SE) that have to migrate at the end of the
//  current timestep
static se_list migr_list,
               *mlist = &migr_list;
/*---------------------------------------------------------------------------*/


/* *************************************************************************
 *                    M O D E L    D E F I N I T I O N
 *      NOTE: in the following there is only a part of the model definition,
 *      the most part of it is implemented in the user level,
 *      see: user_event_handlers.c and lunes.c
 **************************************************************************** */

/*! \brief Computation and Interactions generation: called at each timestep
 *         it will provide the model behavior.
 */
static void Generate_Computation_and_Interactions(int total_SE) {
    // Call the appropriate user event handler
    user_control_handler();
}

/*! \breif SEs initial generation: called once when global variables have been
 *         initialized.
 */
static void Generate(int count) {
    int i;

    // The local Simulated Entities are registered using the appropriate GAIA API
    for (i = 0; i < count; i++) {
        // In this case every entity can be migrated
        GAIA_Register(MIGRABLE);

        // NOTE: the internal state of entities is initialized in the
        //      register_event_handler()
        //      see in the following of this source file
    }
}


/*---------------------------------------------------------------------------*/

/* ************************************************************************ */
/*           E V E N T   H A N D L E R S                                    */
/* ************************************************************************ */

/*! \brief Upon arrival of a model level event, firstly we have to validate it
 *         and only in the following the appropriate handler will be called
 */
struct hash_node_t *validation_model_events(int id, int to, Msg *msg) {
    struct hash_node_t *node;

    // The receiver has to be a locally manager Simulated Entity, let's check!
    if (!(node = hash_lookup(stable, to)))  {
        // The receiver is not managed by this LP, it is really a fatal error
        fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] is NOT in this LP!\n", simclock, to);
        fflush(stdout);
        exit(-1);
    }else {
        return(node);
    }
}

/*! \brief A new SE has been created, we have to insert it into the global
 *      and local hashtables, the correct key to use is the sender's ID
 */
static void register_event_handler(int id, int lp) {
    hash_node_t *node;

    // In every case the new node has to be inserted in the global hash table
    //  containing all the Simulated Entities
    node = hash_insert(GSE, table, NULL, id, lp);
    if (node) {
        // If the SMH is local then it has to be inserted also in the local
        //  hashtable and some extra management is required
        if (lp == LPID) {
            // Call the appropriate user event handler
           // user_register_event_handler(node, id);

            // Inserting it in the table of local SEs
            if (!hash_insert(LSE, stable, node->data, node->data->key, LPID)) {
                // Unable to allocate memory for local SEs
                fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] impossible to add new elements to the hash table of local entities\n", simclock, id);
                fflush(stdout);
                exit(-1);
            }
        }
    }else {
        // The model is unable to add the new SE in the global hash table
        fprintf(stdout, "%12.2f node: FATAL ERROR, [%5d] impossible to add new elements to the global hash table\n", simclock, id);
        fflush(stdout);
        exit(-1);
    }

    fflush(stdout);
}

/*! \brief Manages the "migration notification" of local SEs (i.e. allocated in this LP)
 */
static void notify_migration_event_handler(int id, int to) {
    hash_node_t *node;

    #ifdef DEBUG
    fprintf(stdout, "%12.2f agent: [%5d] is going to be migrated to LP [%5d]\n", simclock, id, to);
    #endif

    // The GAIA framework has decided that a local SE has to be migrated,
    //  the migration can NOT be executed immediately because the SE
    //  could be the destination of some "in flight" messages
    if ((node = hash_lookup(table, id)))  {
        /* Now it is updated the list of SEs that are enabled to migrate (flagged) */
        list_add(mlist, node);

        node->data->lp = to;
        // Call the appropriate user event handler
        user_notify_migration_event_handler();
    }
    // Just before the end of the current timestep, the migration list will be emptied
    //  and the pending migrations will be executed
}

/*! \brief Manages the "migration notification" of external SEs
 *         (that is, NOT allocated in the local LP).
 */
static void notify_ext_migration_event_handler(int id, int to) {
    hash_node_t *node;

    // A migration that does not directly involve the local LP is going to happen in
    //  the simulation. In some special cases the local LP has to take care of
    //  this information
    if ((node = hash_lookup(table, id)))  {
        node->data->lp = to;                // Destination LP of the migration
        // Call the appropriate user event handler
        user_notify_ext_migration_event_handler();
    }
}

/*\brief Migration-event manager (the real migration handler)
 *       This handler is executed when a migration message is received and
 *       therefore a new SE has to be accomodated in the local LP.
 */
static void  migration_event_handler(int id, Msg *msg) {
    hash_node_t *node;

    #ifdef DEBUG
    fprintf(stdout, "%12.2f agent: [%5d] has been migrated in this LP\n", simclock, id);
    #endif

    if ((node = hash_lookup(table, id))) {
        // Inserting the new SE in the local table
        hash_insert(LSE, stable, node->data, node->data->key, LPID);

        // Call the appropriate user event handler
        //user_migration_event_handler(node, id, msg);
    }
}

/*---------------------------------------------------------------------------*/


/* ************************************************************************ */
/*                  U T I L S                                               */
/* ************************************************************************ */

/*! \brief Loading the configuration file of the simulator
 */
static void UNUSED LoadINI(char *ini_name) {
    int  ret;
    char data[64];


    ret = INI_Load(ini_name);
    ASSERT(ret == INI_OK, ("Error loading ini file \"%s\"", ini_name));

    /* SIMA */
    ret = INI_Read("SIMA", "HOST", data);
    if (ret == INI_OK && strlen(data)) {
        strcpy(SIMA_HOST, data);
    }

    ret = INI_Read("SIMA", "PORT", data);
    if (ret == INI_OK && strlen(data)) {
        SIMA_PORT = atoi(data);
    }

    INI_Free();
}

/*---------------------------------------------------------------------------*/


/* ************************************************************************ */
/*                  M A I N                                                 */
/* ************************************************************************ */

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Not enough arguments:\n");
        printf("\tUSAGE: ./t_graph #NLP #IA #TRACEPATH\n");
    }
    char msg_type,                      // Type of message
         *data,                         // Buffer for incoming messages, dynamic allocation
         *rnd_file = "Rand2.seed";      // File containing seeds for the random numbers generator

    int count,                          // Number of SEs to simulate in the local LP
        start,                          // First identifier (ID) to be used to tag the locally managed SEs
        max_data;                       // Maximum size of incoming messages

    int from,                           // ID of the message sender
        to,                             // ID of the message receiver
        tot = 0;                        // Total number of executed migrations

    int loc,                            // Number of messages with local destination (intra-LP)
        rem,                            // Number of messages with remote destination (extra-LP)
        migr;                           // Number of executed migrations
    //int t;                              // Total number of messages (local + remote)

    double Ts;                          // Current timestep
    Msg *  msg;                         // Generic message

    //int migrated_in_this_step;          // Number of entities migrated in this step, in the local LP

    struct hash_node_t *tmp_node;       // Tmp variable, a node in the hash table
    char *dat_filename, *tmp_filename;  // File descriptors for simulation traces

    // Time measurement
    struct timeval t1, t2;

    // Local PID
    local_pid = getpid();

    // Loading the input parameters from the configuration file
    LoadINI("graph.ini");

    // Returns the standard host name for the execution host
    gethostname(LP_HOST, 64);


    // Command-line input parameters
    NLP       = atoi(argv[1]);  // Number of LPs
    NSIMULATE = atoi(argv[2]);  // Number of SEs to simulate
    TESTNAME  = argv[3];        // Output directory for simulation traces

        messages = (data_message *) malloc(sizeof(data_message) * (300000));



    // Initialization of the random numbers generator
    RND_Init(S, rnd_file, LPID);

    // User level handler to get some configuration parameters from the runtime environment
    // (e.g. the GAIA parameters and many others)
    user_environment_handler();

    /*
     *      Set-up of the GAIA framework
     *
     *      Parameters:
     *      1. (SIMULATE*NLP)   Total number of simulated entities
     *      2. (NLP)        Number of LPs in the simulation
     *      3. (rnd_file)       Seeds file for the random numbers generator
     *      4. (NULL)           LP canonical name
     *      5. (SIMA_HOST)      Hostname where the SImulation MAnager is running
     *      6. (SIMA_PORT)      SIMA TCP port number
     */
    LPID = GAIA_Initialize(NSIMULATE * NLP, NLP, rnd_file, NULL, SIMA_HOST, SIMA_PORT);

    // Returns the length of the timestep
    // this value is defined in the "CHANNELS.TXT" configuration file
    // given that GAIA is based on the time-stepped synchronization algorithm
    // it retuns the size of a step
    step = GAIA_GetStep();

    // Due to synchronization constraints The FLIGHT_TIME has to be bigger than the timestep size
    if (FLIGHT_TIME < step) {
        fprintf(stdout, "FATAL ERROR, the FLIGHT_TIME (%8.2f) is less than the timestep size (%8.2f)\n", FLIGHT_TIME, step);
        fflush(stdout);
        exit(-1);
    }

    // First identifier (ID) of SEs allocated in the local LP
    start = NSIMULATE * LPID;

    // Number of SEs to allocate in the local LP
    count = NSIMULATE;

    //  Used to set the ID of the first simulated entity (SE) in the local LPnsimulnsimulnsimul
    GAIA_SetFstID(start);

    // Output file for statistics (communication ratio data)
    dat_filename = malloc(1024);
    snprintf(dat_filename, 1024, "%stmp-evaluation-lcr.dat", TESTNAME);
    lcr_fp = fopen(dat_filename, "w");

    // Data structures initialization (hash tables and migration list)
    hash_init(table, NSIMULATE * NLP);                  // Global hashtable: all the SEs
    hash_init(stable, NSIMULATE);                       // Local hastable: local SEs
    list_init(mlist);                                   // Migration list (pending migrations in the local LP)

    // Starting the execution timer
    TIMER_NOW(t1);

    fprintf(stdout, "#LP [%d] HOSTNAME [%s]\n", LPID, LP_HOST);
    fprintf(stdout, "#                      LP[%d] STARTED\n#\n", LPID);
    fprintf(stdout, "#          Generating Simulated Entities from %d To %d ... ", (LPID * NSIMULATE), ((LPID * NSIMULATE) + NSIMULATE) - 1);
    fflush(stdout);

    // Generate all the SEs managed in this LP
    Generate(count);
    fprintf(stdout, " OK\n#\n");

    fprintf(stdout, "# Data format:\n");
    fprintf(stdout, "#\tcolumn 1:   elapsed time (seconds)\n");
    fprintf(stdout, "#\tcolumn 2:   timestep\n");
    fprintf(stdout, "#\tcolumn 3:   number of entities in this LP\n");
    fprintf(stdout, "#\tcolumn 4:   number of migrating entities (from this LP)\n");

    // It is the LP that manages statistics
    if (LPID == LP_STAT) {                      // Verbose output
        fprintf(stdout, "#\tcolumn 5:   local communication ratio (percentage)\n");
        fprintf(stdout, "#\tcolumn 6:   remote communication ratio (percentage)\n");
        fprintf(stdout, "#\tcolumn 7:   total number of migrations in this timestep\n");
    }
    fprintf(stdout, "#\n");

    // Dynamically allocating some space to receive messages
    data = malloc(BUFFER_SIZE);
    ASSERT((data != NULL), ("simulation main: malloc error, receiving buffer NOT allocated!"));

    // Before starting the real simulation tasks, the model level can initialize some
    //  data structures and set parameters
    user_bootstrap_handler();

    /* Main simulation loop, receives messages and calls the handler associated with them */
    while (!end_reached) {
        // Max size of the next message.
        //  after the receive the variable will contain the real size of the message
        max_data = BUFFER_SIZE;

        // Looking for a new incoming message
        msg_type = GAIA_Receive(&from, &to, &Ts, (void *)data, &max_data);
        msg      = (Msg *)data;

        // A message has been received, process it (calling appropriate handler)
        //  message handlers
        switch (msg_type) {
        // The migration of a locally managed SE has to be done,
        //  calling the appropriate handler to insert the SE identifier
        //  in the list of pending migrations
        case NOTIF_MIGR:
            notify_migration_event_handler(from, to);
            break;

        // A migration has been executed in the simulation but the local
        //  LP is not directly involved in the migration execution
        case NOTIF_MIGR_EXT:
            notify_ext_migration_event_handler(from, to);
            break;

        // Registration of a new SE that is managed by another LP
        case REGISTER:
            register_event_handler(from, to);
            break;

        // The local LP is the receiver of a migration and therefore a new
        //  SE has to be managed in this LP. The handler is responsible
        //  to allocate the necessary space in the LP data structures
        //  and in the following to copy the SE state that is contained
        //  in the migration message
        case EXEC_MIGR:
            migration_event_handler(from, msg);
            break;

        // End Of Step:
        //  the current simulation step is finished, some pending operations
        //  have to be performed
        case EOS:
            // Stopping the execution timer
            //  (to record the execution time of each timestep)
            TIMER_NOW(t2);

            /*  Actions to be done at the end of each simulated timestep  */
            if (simclock < env_end_clock) { // The simulation is not finished
                // Simulating the interactions among SEs
                //
                //  in the last (env_end_clock - FLIGHT_TIME) timesteps
                //  no msgs will be sent because we wanna check if all
                //  sent msgs are correctly received
                if (simclock < (env_end_clock - FLIGHT_TIME)) {
                    Generate_Computation_and_Interactions(NSIMULATE * NLP);
                }

                // The pending migration of "flagged" SEs has to be executed,
                //  the SE to be migrated were previously inserted in the migration
                //  list due to the receiving of a "NOTIF_MIGR" message sent by
                //  the GAIA framework
                //migrated_in_this_step = ScanMigrating();

                // The LP that manages statistics prints out them
                if (LPID == LP_STAT) {                                  // Verbose output
                    // Some of them are provided by the GAIA framework
                    GAIA_GetStatistics(&loc, &rem, &migr);

                    // Total number of migrations (in the simulation run)
                    tot += migr;

                    // Printed fields:
                    //  elapsed Wall-Clock-Time up to this step
                    //  timestep number
                    //  number of entities in this LP
                    //  percentage of local communications (intra-LP)
                    //  percentage of remote communications (inter-LP)
                    //  number of migrations in this timestep

                    // Total number of interactions (in the timestep)
                    #ifdef DEBUG
                    float t = loc + rem;
                    fprintf(stdout, "- [%11.2f]\t[%6.5f]\t%4.0f\t%2.2f\t%2.2f\t%d\n", TIMER_DIFF(t2, t1), simclock, (float)stable->count, (float)loc / (float)t * 100.0, (float)rem / (float)t * 100.0, migr);
                    if (simclock >= 7) { fprintf(lcr_fp, "%f\n", (float)loc / (float)t * 100.0); }
                    #endif
                }else {
                    // Reduced output
                    #ifdef DEBUG
                    fprintf(stdout, "[%11.2fs]   %12.2f [%d]\n", TIMER_DIFF(t2, t1), simclock, stable->count);
                    #endif
                }

                // Now it is possible to advance to the next timestep
                simclock = GAIA_TimeAdvance();
            }else {
                /* End of simulation */
                TIMER_NOW(t2);

                fprintf(stdout, "\n\n");
                fprintf(stdout, "### Termination condition reached (%d)\n", tot);
                fprintf(stdout, "### Clock           %12.2f\n", simclock);



                int detail = 20;                // there will be detail + detail cells in the heatmap
                int heatValues [detail][detail];
                int heatPop    [detail][detail];

                for (int i =0; i< detail; i++){         //initialize matrices for heat map
                    for (int j =0; j < detail; j++){
                        heatPop[i][j] = 0;
                        heatValues[i][j] = 0;
                    }
                }


                int total_delay = 0, border_nodes_messages = 0, border_nodes_delay=0, notImmidiateMessages=0, deliveredMessages=0;
                double average_delay=0, variance_delay, average_delayNI, variance_delayNI, border_average_delay, not_border_average_delay;
                for (int i =0; i< messages_counter; i++){
                    if (messages[i].delivered >0){
                        deliveredMessages++;
                        if (messages[i].state != 'I'){
                            notImmidiateMessages++;
                        }
                    }

                    if (messages[i].originX < 250 || messages[i].originY < 250 || messages[i].originX > 750 || messages[i].originY > 750){
                        if (messages[i].delivered > 0){
                            border_nodes_messages++;
                            border_nodes_delay += (messages[i].delivered - messages[i].emitted);
                        }
                    }
                    
                    if (messages[i].delivered > 0){                 //consider only received messages
                        total_delay += (messages[i].delivered - messages[i].emitted);
                        int thisX, thisY;
                        thisX = messages[i].originX / (env_grid_length/detail);
                        thisY = messages[i].originY / (env_grid_length/detail);
                        heatValues[thisX][thisY] += (messages[i].delivered - messages[i].emitted);
                        heatPop[thisX][thisY] ++;
                    }
                }

                average_delay = (double)total_delay/deliveredMessages;
                average_delayNI = (double)total_delay/notImmidiateMessages;                                            //average delay for not immediate messages
                border_average_delay = (double)border_nodes_delay/border_nodes_messages;
                not_border_average_delay = (double)(total_delay - border_nodes_delay)/(deliveredMessages - border_nodes_messages);
                double temp_variance =0, temp_varianceNI=0;
                for (int i = 0; i < deliveredMessages; i++){                                                            //calculate variance
                    if (messages[i].delivered > 0){
                        temp_variance += ((messages[i].delivered - messages[i].emitted) * 2 - average_delay * 2) * ((messages[i].delivered - messages[i].emitted) * 2 - average_delay * 2);
                        if (messages[i].state!= 'I'){
                            temp_varianceNI += ((messages[i].delivered - messages[i].emitted) * 2 - average_delay * 2) * ((messages[i].delivered - messages[i].emitted) * 2 - average_delay * 2);
                        }
                    }
                }
                variance_delay = temp_variance / deliveredMessages;
                variance_delayNI = temp_varianceNI/notImmidiateMessages;  //variance delay for not immediate messages
                fprintf(stdout, "%d out of %d messages delivered. \n", deliveredMessages, messages_counter);
                fprintf(stdout, "Average delay: %.3f with variance %.2f. Messages which employ mules have an average delay of %.3f with variance %.3f \n", average_delay, variance_delay, average_delayNI, variance_delayNI);
                fprintf(stdout, "%d out of %d messages come from border nodes. For these messages the average delay is %.3f, against %.3f of the non-border nodes\n", border_nodes_messages, messages_counter, border_average_delay, not_border_average_delay);
                fprintf(stdout, "Average number of hops = %lf ", (double) total_hops/deliveredMessages);

                int total_delay1=0, total_delay2=0, total_delay3 = 0, proxyMuleMessages=0; //, busMuleMessages=0
                for (int i =0; i<messages_counter; i++){
                    if (messages[i].delivered > 0){
                        if (messages[i].toBusMule >= 0){
                            total_delay1 += (messages[i].toBusMule - messages[i].emitted);
                        }
                        if (messages[i].toProxyMule >= 0 ){
                            if (messages[i].toBusMule >= 0){
                                total_delay2 += (messages[i].toProxyMule - messages[i].toBusMule);
                            } else {
                                total_delay2 += (messages[i].toProxyMule - messages[i].emitted);
                            }
                        }
                        if (messages[i].toBusMule != -1 && messages[i].toProxyMule != -1){
                            proxyMuleMessages++;
                        }

                        if (messages[i].toProxyMule != -1){
                            total_delay3 += (messages[i].delivered - messages[i].toProxyMule);
                        } else {
                            if (messages[i].toBusMule != -1){
                                total_delay3 += (messages[i].delivered - messages[i].toBusMule);
                            } else {
                                total_delay3 += (messages[i].delivered - messages[i].emitted);
                            }
                        }
                    }
                }

                fprintf(stdout, "On average delay for not immediate messages (%d out of %d messages) is composed of %lf as a bus-mule delay, %lf as a proxy-mule delay, %lf as delay3 \n", notImmidiateMessages ,messages_counter, (double)total_delay1/notImmidiateMessages, (double)total_delay2/notImmidiateMessages, (double)total_delay3/notImmidiateMessages );


                //coverage region by regions for the heatmap
                fprintf(stdout, "\n [");
                for (int i = 0; i < detail;  i++){
                    fprintf(stdout, "[");
                    for (int j = 0; j < detail; j++){
                        double res = 0.0;
                        if (heatPop[i][j]>0){
                            res =  (double)heatValues[i][j]/heatPop[i][j];
                        }
                        if (j == detail - 1 ){
                            if (i == detail - 1){
                                fprintf(stdout, "%.2f]",res);
                            } else {
                                fprintf(stdout, "%.2f], \n",res);
                            }
                        } else {
                            fprintf(stdout, "%.2f ,", res);
                        }
                    }                
                }
                fprintf(stdout, "]\n");

                fflush(stdout); 
                end_reached = 1;
            }
            break;

        // Simulated model events (user level events)
        case UNSET:
            // First some checks for validation
            tmp_node = validation_model_events(from, to, msg);

            // The appropriate handler is defined at model level
            user_model_events_handler(to, from, msg, tmp_node);
            break;

        default:
            fprintf(stdout, "FATAL ERROR, received an unknown event type: %d\n", msg_type);
            fflush(stdout);
            exit(-1);
        }
    }


    // Finalize the GAIA framework
    GAIA_Finalize();

    // Before shutting down, the model layer is able to deallocate some data structures
    user_shutdown_handler();

    // Closing output file for performance evaluation
    fclose(lcr_fp);

    // Freeing of the receiving buffer
    free(data);

    // Creating the "finished file" that is used by some scripts
    tmp_filename = malloc(256);
    snprintf(tmp_filename, 256, "%d.finished", LPID);
    finished_fp = fopen(tmp_filename, "w");
    fclose(finished_fp);

    // That's all folks.
    return(0);
}