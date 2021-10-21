#ifndef __MESSAGE_DEFINITION_H
#define __MESSAGE_DEFINITION_H

#include "entity_definition.h"

/*---- M E S S A G E S    D E F I N I T I O N ---------------------------------*/


typedef struct _migr_msg       MigrMsg;  // Migration message
typedef union   msg            Msg;
typedef struct _discovery_msg  DiscoveryMsg;   

// General note:
//	each type of message is composed of a static and a dynamic part
//	-	the static part contains a pre-defined set of variables, and the size
//		of the dynamic part (as number of records)
//	-	a dynamic part that is composed of a sequence of records


struct _discovery_static_part {
    char		type;		//Message type
    int		gatewayId;	//ID of the client node that can act as gateway
    int		muleId;	//ID of the mule in contact with the client
};
//

struct _discovery_msg {
    struct  _discovery_static_part discovery_static;
};




// **********************************************
// MIGRATION MESSAGES
// **********************************************
//
/*! \brief Static part of migration messages */
struct _migration_static_part {
    char          type;        // Message type
    unsigned int  dyn_records; // Number of records in the dynamic part of the message
};
//
/*! \brief Dynamic part of migration messages */
struct _migration_dynamic_part {
    struct state_element records[MAX_MIGRATION_DYNAMIC_RECORDS]; // It is an array of records
};
//
/*! \brief Migration message */
struct _migr_msg {
    struct  _migration_static_part  migration_static;  // Static part
    struct  _migration_dynamic_part migration_dynamic; // Dynamic part
};



/*! \brief Union structure for all types of messages */
union msg {
    char           type;
    MigrMsg        migr;
    DiscoveryMsg   discovery;
};
/*---------------------------------------------------------------------------*/

#endif /* __MESSAGE_DEFINITION_H */
