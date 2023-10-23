#include "FreeRTOS.h"

#include "platform/iot_network.h"
#include "platform/iot_threads.h"

#include "iot_network_manager_private.h"


/**
 * @brief All C SDK demo functions have this signature.
 */
typedef int (* demoFunction_t)( bool awsIotMqttMode,
                                const char * pIdentifier,
                                void * pNetworkServerInfo,
                                void * pNetworkCredentialInfo,
                                const IotNetworkInterface_t * pNetworkInterface );


typedef void (* networkConnectedCallback_t)( bool awsIotMqttMode,
                                             const char * pIdentifier,
                                             void * pNetworkServerInfo,
                                             void * pNetworkCredentialInfo,
                                             const IotNetworkInterface_t * pNetworkInterface );

typedef void (* networkDisconnectedCallback_t)( const IotNetworkInterface_t * pNetworkInteface );

typedef struct appMqttContext
{
    /* Network types for the demo */
    uint32_t networkTypes;
    /* Function pointers to be set by the implementations for the demo */
    demoFunction_t demoFunction;
    networkConnectedCallback_t networkConnectedCallback;
    networkDisconnectedCallback_t networkDisconnectedCallback;
    TaskHandle_t * pHandle;
} appMqttContext_t;

typedef struct appNetworkSetting
{
    const IotNetworkInterface_t *pNetworkInterface;
    void *pConnectionParams;
    void *pCredentials;
} appNetworkSetting_t;

int network_initialize(appMqttContext_t *pContext);
appNetworkSetting_t getNetworkSetting();

