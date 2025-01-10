#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "softbus_api.h"

int main() {
    // Initialize software bus
    if (softbus_api_init() != SOFTBUS_OK) {
        printf("Software bus initialization failed\n");
        return -1;
    }
    
    // Register devices
    if (softbus_api_register_device(DEVICE_TYPE_LED, "led") != SOFTBUS_OK) {
        printf("LED device registration failed\n");
        return -1;
    }
    
    if (softbus_api_register_device(DEVICE_TYPE_TEMPERATURE_SENSOR, "temp_sensor") != SOFTBUS_OK) {
        printf("Temperature sensor registration failed\n");
        return -1;
    }
    
    // Create and configure sensor group
    if (softbus_api_create_device_group("sensors") != SOFTBUS_OK ||
        softbus_api_add_device_to_group("sensors", "led") != SOFTBUS_OK ||
        softbus_api_add_device_to_group("sensors", "temp_sensor") != SOFTBUS_OK) {
        printf("Sensor group configuration failed\n");
        return -1;
    }
    
    printf("\n=== Unicast Mode Test ===\n");
    
    // Test sending messages with different priorities
    const char* msgs[] = {
        "Turn on LED",
        "Adjust LED brightness",
        "LED blink",
        "Turn off LED"
    };
    softbus_priority_t priorities[] = {
        SOFTBUS_PRIO_NORMAL,
        SOFTBUS_PRIO_HIGH,
        SOFTBUS_PRIO_URGENT,
        SOFTBUS_PRIO_LOW
    };
    
    // Send multiple messages with different priorities
    for (int i = 0; i < 4; i++) {
        if (softbus_api_send_message("led", MSG_TYPE_CONTROL, msgs[i], priorities[i]) != SOFTBUS_OK) {
            printf("Failed to send LED message\n");
        }
    }
    
    // Process LED device message queue
    softbus_api_process_device_messages("led");
    
    printf("\n=== Multicast Mode Test ===\n");
    
    // Test multicast messages
    const char* group_msgs[] = {
        "All sensors start working",
        "Emergency status check",
        "Normal status check",
        "Low priority maintenance"
    };
    
    // Send multiple multicast messages with different priorities
    for (int i = 0; i < 4; i++) {
        if (softbus_api_send_group_message("sensors", MSG_TYPE_STATUS, 
                                         group_msgs[i], priorities[i]) != SOFTBUS_OK) {
            printf("Failed to send multicast message\n");
        }
        Sleep(100);  // Small delay to observe message processing order
    }
    
    // Process message queues for each device
    softbus_api_process_device_messages("led");
    softbus_api_process_device_messages("temp_sensor");
    
    // Resource request test
    float* temp = (float*)softbus_api_request_resource("temp_sensor", "temperature");
    if (temp) {
        printf("\nCurrent temperature: %.1f°C\n", *temp);
    } else {
        printf("\nFailed to get temperature resource\n");
    }
    
    // Cleanup
    softbus_api_delete_device_group("sensors");
    softbus_api_unregister_device("led");
    softbus_api_unregister_device("temp_sensor");
    softbus_api_deinit();
    
    return 0;
} 