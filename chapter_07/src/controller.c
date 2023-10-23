#define democonfigNETWORK_BUFFER_SIZE (1024U)

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* Set up logging for this demo. */
#include "iot_demo_logging.h"

/* Platform layer includes. */
#include "platform/iot_clock.h"
#include "platform/iot_threads.h"

#include "semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_event.h"

#include "device.h"
#include "controller.h"

#include "core_mqtt.h"
#include "shadow_client.h"
#include "app_network.h"




static const char *TAG = "project";

void _mainButtonEventHandler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base == BUTTON_MAIN_EVENT_BASE)
    {
        if (id == BUTTON_CLICK)
        {
            ESP_LOGI(TAG, "Main Button Pressed");
        }
        if (id == BUTTON_HOLD)
        {
            ESP_LOGI(TAG, "Main Button Held");
        }
    }
}

void _resetButtonEventHandler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base == BUTTON_RESET_EVENT_BASE)
    {

        if (id == BUTTON_HOLD)
        {
            ESP_LOGI(TAG, "Reset Button Held");
            ESP_LOGI(TAG, "Reseting Wifi Networks");
            //vLabConnectionResetWifiNetworks();
        }
        if (id == BUTTON_CLICK)
        {
            ESP_LOGI(TAG, "Reset Button Clicked");
            ESP_LOGI(TAG, "Restarting in 2secs");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
        }
    }
}

void runCoreMqttTask(void *pArgument)
{
    appMqttContext_t *pContext = (appMqttContext_t *)pArgument;
    int status;

    status = network_initialize(pContext);

    if (status != EXIT_SUCCESS)
    {
        IotLogInfo("_initialize failed");
        return;
    }

    appNetworkSetting_t setting = getNetworkSetting();
    // receive command from server.
    RunDeviceShadowClient(true,
                        clientcredentialIOT_THING_NAME,
                        setting.pConnectionParams,
                        setting.pCredentials,
                        setting.pNetworkInterface,
                        pContext->pHandle);
}

void runActuatorTask(void *pArgument)
{
    TaskHandle_t *pHandle = (TaskHandle_t *)pArgument;
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        eChangeLockState(1U);
        STATUS_LED_ON();
        vTaskDelay(pdMS_TO_TICKS(5000));
        eChangeLockState(0U);
        STATUS_LED_OFF();

        if (*pHandle)
        {
            xTaskNotifyGive(*pHandle);
        }
    }
}
/*-----------------------------------------------------------*/

esp_err_t eControllerRun(void)
{
    esp_err_t res = ESP_FAIL;

    IotLogInfo("eControllerRun: ======================================================");

    res = eDeviceInit();

    if (res == ESP_OK)
    {

        res = eDeviceRegisterButtonCallback(BUTTON_MAIN_EVENT_BASE, _mainButtonEventHandler);
        if (res != ESP_OK)
        {
            IotLogError("eControllerRun: Register main button ... failed");
        }

        res = eDeviceRegisterButtonCallback(BUTTON_RESET_EVENT_BASE, _resetButtonEventHandler);
        if (res != ESP_OK)
        {
            IotLogError("eControllerRun: Register reset button ... failed");
        }
    }
    else
    {
        ESP_LOGE(TAG, "eControllerRun: eControllerRun ... failed");
    }

    static TaskHandle_t xCoreMqttTask = NULL, xActuatorTask = NULL, xPublishTask = NULL;
    xTaskCreate(publishCurrentStateTask, "publish", configMINIMAL_STACK_SIZE * 8, (void *)NULL, tskIDLE_PRIORITY + 4, &xPublishTask);

    static appMqttContext_t appMqttContext =
        {
            .networkTypes = AWSIOT_NETWORK_TYPE_WIFI,
            .networkConnectedCallback = NULL,
            .networkDisconnectedCallback = NULL,
            .pHandle = NULL};

    xTaskCreate(runActuatorTask, "actuator", configMINIMAL_STACK_SIZE * 8, &xPublishTask, tskIDLE_PRIORITY + 4, &xActuatorTask);
    appMqttContext.pHandle = &xActuatorTask;

    xTaskCreate(runCoreMqttTask, "mqtt", configMINIMAL_STACK_SIZE * 8, &appMqttContext, tskIDLE_PRIORITY + 5, &xCoreMqttTask);

    ESP_LOGI(TAG, "======================================================");

    return res;
}

/*-----------------------------------------------------------*/
