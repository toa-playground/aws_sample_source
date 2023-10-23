/**
 * @file shadow_client.h
 *
 * (C) 2021 - Tatsuhiro Iida 
 * This code is licensed under the MIT License.
 */

int RunDeviceShadowClient(bool awsIotMqttMode,
                        const char *pIdentifier,
                        void *pNetworkServerInfo,
                        void *pNetworkCredentialInfo,
                        const void *pNetworkInterface,
                        TaskHandle_t *pHandle);

void publishCurrentStateTask(void *pArgument);
