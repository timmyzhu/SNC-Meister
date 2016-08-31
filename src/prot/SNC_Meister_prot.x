/* SNC_Meister_prot.x - SNC-Meister RPC interface.
 * Interface for communicating with SNC-Meister and performing admission control.
 */

#ifdef IGNORE_WARNINGS
%#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

typedef string str<>;

enum SNCStatus {
    SNC_SUCCESS,
    SNC_ERR_MISSING_ARGUMENT,
    SNC_ERR_INVALID_ARGUMENT,
    SNC_ERR_FLOW_NAME_IN_USE,
    SNC_ERR_CLIENT_NAME_IN_USE,
    SNC_ERR_QUEUE_NAME_IN_USE,
    SNC_ERR_FLOW_NAME_NONEXISTENT,
    SNC_ERR_CLIENT_NAME_NONEXISTENT,
    SNC_ERR_QUEUE_NAME_NONEXISTENT,
    SNC_ERR_QUEUE_HAS_ACTIVE_FLOWS
};

/* Arguments for AddClients RPC */
struct SNCAddClientsArgs {
    /* string encoded JSON of list of clients (see SNC-Library/NC.hpp) */
    str clientInfos;
};

/* Results for AddClients RPC */
struct SNCAddClientsRes {
    SNCStatus status;
    bool admitted;
};

/* Arguments for DelClient RPC */
struct SNCDelClientArgs {
    /* name of client to delete */
    str name;
};

/* Results for DelClient RPC */
struct SNCDelClientRes {
    SNCStatus status;
};

/* Arguments for AddQueue RPC */
struct SNCAddQueueArgs {
    /* string encoded JSON of a queue (see SNC-Library/NC.hpp) */
    str queueInfo;
};

/* Results for AddQueue RPC */
struct SNCAddQueueRes {
    SNCStatus status;
};

/* Arguments for DelQueue RPC */
struct SNCDelQueueArgs {
    /* name of queue to delete */
    str name;
};

/* Results for DelQueue RPC */
struct SNCDelQueueRes {
    SNCStatus status;
};

/* SNC-Meister RPC interface */
program SNC_MEISTER_PROGRAM {
    version SNC_MEISTER_V1 {
        void
        SNC_MEISTER_NULL(void) = 0;

        /* Determine admission control for a set of clients */
        SNCAddClientsRes
        SNC_MEISTER_ADD_CLIENTS(SNCAddClientsArgs) = 1;

        /* Delete a client */
        SNCDelClientRes
        SNC_MEISTER_DEL_CLIENT(SNCDelClientArgs) = 2;

        /* Add a queue */
        SNCAddQueueRes
        SNC_MEISTER_ADD_QUEUE(SNCAddQueueArgs) = 3;

        /* Delete a queue */
        SNCDelQueueRes
        SNC_MEISTER_DEL_QUEUE(SNCDelQueueArgs) = 4;
    } = 1;
} = 8000;
