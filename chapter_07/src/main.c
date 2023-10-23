#include "iot_config.h"

/* FreeRTOS includes. */

#include "FreeRTOS.h"
#include "task.h"

// /* Demo includes */
#include "aws_demo.h"
#include "aws_dev_mode_key_provisioning.h"
#include "aws_clientcredential.h"

/* AWS System includes. */
#include "iot_system_init.h"
#include "iot_logging_task.h"

#include "nvs_flash.h"
#if !AFR_ESP_LWIP
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#endif

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_interface.h"
#include "esp_bt.h"
#if CONFIG_NIMBLE_ENABLED == 1
    #include "esp_nimble_hci.h"
#else
    #include "esp_gap_ble_api.h"
    #include "esp_bt_main.h"
#endif

#include "driver/uart.h"
#include "aws_application_version.h"
#include "tcpip_adapter.h"

#include "iot_network_manager_private.h"

#include "controller.h"

/* Logging Task Defines. */
#define mainLOGGING_MESSAGE_QUEUE_LENGTH    ( 32 )
#define mainLOGGING_TASK_STACK_SIZE         ( configMINIMAL_STACK_SIZE * 4 )
#define mainDEVICE_NICK_NAME                "Espressif_Demo"

/*-----------------------------------------------------------*/

/* Static arrays for FreeRTOS+TCP stack initialization for Ethernet network connections
 * are use are below. If you are using an Ethernet connection on your MCU device it is
 * recommended to use the FreeRTOS+TCP stack. The default values are defined in
 * FreeRTOSConfig.h. */

/**
 * @brief Initializes the board.
 */
static void prvMiscInitialization( void );

/*-----------------------------------------------------------*/

extern void esp_vApplicationTickHook();
void IRAM_ATTR vApplicationTickHook()
{
    esp_vApplicationTickHook();
}

/*-----------------------------------------------------------*/
extern void esp_vApplicationIdleHook();
void vApplicationIdleHook()
{
    esp_vApplicationIdleHook();
}

/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
}

#if !AFR_ESP_LWIP
    extern void vApplicationIPInit( void );

    /*-----------------------------------------------------------*/

    void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
    {
        uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
        system_event_t evt;

        if( eNetworkEvent == eNetworkUp )
        {
            /* Print out the network configuration, which may have come from a DHCP
            * server. */
            FreeRTOS_GetAddressConfiguration(
                &ulIPAddress,
                &ulNetMask,
                &ulGatewayAddress,
                &ulDNSServerAddress );

            evt.event_id = SYSTEM_EVENT_STA_GOT_IP;
            evt.event_info.got_ip.ip_changed = true;
            evt.event_info.got_ip.ip_info.ip.addr = ulIPAddress;
            evt.event_info.got_ip.ip_info.netmask.addr = ulNetMask;
            evt.event_info.got_ip.ip_info.gw.addr = ulGatewayAddress;
            esp_event_send( &evt );
        }
    }
#endif

/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */
int app_main( void )
{
    /* Perform any hardware initialization that does not require the RTOS to be
     * running.  */

    prvMiscInitialization();

    if( SYSTEM_Init() == pdPASS )
    {

        /* A simple example to demonstrate key and certificate provisioning in
         * microcontroller flash using PKCS#11 interface. This should be replaced
         * by production ready key provisioning mechanism. */
        vDevModeKeyProvisioning();

        ESP_ERROR_CHECK( esp_bt_controller_mem_release( ESP_BT_MODE_CLASSIC_BT ) );
        ESP_ERROR_CHECK( esp_bt_controller_mem_release( ESP_BT_MODE_BLE ) );

        eControllerRun();
    }

    /* Start the scheduler.  Initialization that requires the OS to be running,
     * including the WiFi initialization, is performed in the RTOS daemon task
     * startup hook. */
    /* Following is taken care by initialization code in ESP IDF */
    /* vTaskStartScheduler(); */
    return 0;
}

/*-----------------------------------------------------------*/

static void prvMiscInitialization( void )
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();

    if( ( ret == ESP_ERR_NVS_NO_FREE_PAGES ) || ( ret == ESP_ERR_NVS_NEW_VERSION_FOUND ) )
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK( ret );

    /* Create tasks that are not dependent on the WiFi being initialized. */
    xLoggingTaskInitialize( mainLOGGING_TASK_STACK_SIZE,
                            tskIDLE_PRIORITY + 5,
                            mainLOGGING_MESSAGE_QUEUE_LENGTH );

#if AFR_ESP_LWIP
    configPRINTF( ("Initializing lwIP TCP stack\r\n") );
    tcpip_adapter_init();
#else
    configPRINTF( ("Initializing FreeRTOS TCP stack\r\n") );
    vApplicationIPInit();
#endif
}

/*-----------------------------------------------------------*/


