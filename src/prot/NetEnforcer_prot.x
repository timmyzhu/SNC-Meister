/* NetEnforcer_prot.x - NetEnforcer RPC interface.
 * Interface for sending network configuration parameters to NetEnforcer.
 */

#ifdef IGNORE_WARNINGS
%#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

/* Identifies a client by a dst/src IP address */
struct NetClient {
    unsigned long s_dstAddr;
    unsigned long s_srcAddr;
};

/* Network QoS configuration parameters for a client */
struct NetClientUpdate {
    NetClient client;
    unsigned int priority;
};

typedef NetClientUpdate NetUpdateClientsArgs<>;
typedef NetClient NetRemoveClientsArgs<>;

/* NetEnforcer RPC interface */
program NET_ENFORCER_PROGRAM {
    version NET_ENFORCER_V1 {
        void
        NET_ENFORCER_NULL(void) = 0;

        /* Update network QoS parameters for a set of clients */
        void
        NET_ENFORCER_UPDATE_CLIENTS(NetUpdateClientsArgs) = 1;

        /* Remove a set of clients and revert their network QoS settings to defaults */
        void
        NET_ENFORCER_REMOVE_CLIENTS(NetRemoveClientsArgs) = 2;
    } = 1;
} = 8001;
