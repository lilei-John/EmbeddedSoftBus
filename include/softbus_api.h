#ifndef SOFTBUS_API_H
#define SOFTBUS_API_H

#include <softbus.h>

/* Device type definitions */
typedef enum {
    DEVICE_TYPE_LED,
    DEVICE_TYPE_TEMPERATURE_SENSOR,
    DEVICE_TYPE_MAX
} device_type_t;

/* Message type definitions */
typedef enum {
    MSG_TYPE_CONTROL,     // Control messages (e.g., turn on/off)
    MSG_TYPE_STATUS,      // Status messages (e.g., status check)
    MSG_TYPE_DATA,        // Data messages (e.g., sensor readings)
    MSG_TYPE_MAX
} message_type_t;

/* API initialization and cleanup */
int softbus_api_init(void);
void softbus_api_deinit(void);

/* Device management */
int softbus_api_register_device(device_type_t type, const char* device_name);
int softbus_api_unregister_device(const char* device_name);

/* Message sending */
int softbus_api_send_message(const char* target, message_type_t type, 
                           const char* message, softbus_priority_t priority);
int softbus_api_send_group_message(const char* group_name, message_type_t type,
                                 const char* message, softbus_priority_t priority);

/* Group management */
int softbus_api_create_device_group(const char* group_name);
int softbus_api_add_device_to_group(const char* group_name, const char* device_name);
int softbus_api_remove_device_from_group(const char* group_name, const char* device_name);
int softbus_api_delete_device_group(const char* group_name);

/* Resource management */
void* softbus_api_request_resource(const char* device_name, const char* resource_name);
int softbus_api_release_resource(const char* device_name, const char* resource_name);

/* Message processing */
int softbus_api_process_device_messages(const char* device_name);

#endif /* SOFTBUS_API_H */ 