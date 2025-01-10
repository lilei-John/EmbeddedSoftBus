# SoftBus API Reference

Version: 1.0.0
Last Updated: 2024-01-10

## Overview
The SoftBus API provides a comprehensive interface for device communication and management in distributed systems. It supports device registration, message passing, group management, and resource allocation with emphasis on reliability and efficiency.

## Table of Contents
- [Data Types](#data-types)
- [Initialization and Cleanup](#initialization-and-cleanup)
- [Device Management](#device-management)
- [Message Management](#message-management)
- [Group Management](#group-management)
- [Resource Management](#resource-management)
- [Error Codes](#error-codes)

## Data Types

### Device Type (device_type_t)
```c
typedef enum {
    DEVICE_TYPE_LED,              // LED device
    DEVICE_TYPE_TEMPERATURE_SENSOR, // Temperature sensor
    DEVICE_TYPE_MAX               // Number of device types
} device_type_t;
```

### Message Type (message_type_t)
```c
typedef enum {
    MSG_TYPE_CONTROL,     // Control messages (e.g., switch control)
    MSG_TYPE_STATUS,      // Status messages (e.g., status query)
    MSG_TYPE_DATA,        // Data messages (e.g., sensor readings)
    MSG_TYPE_MAX         // Number of message types
} message_type_t;
```

### Message Priority (softbus_priority_t)
```c
typedef enum {
    SOFTBUS_PRIO_LOW,     // Low priority (background tasks)
    SOFTBUS_PRIO_NORMAL,  // Normal priority (regular operations)
    SOFTBUS_PRIO_HIGH,    // High priority (important operations)
    SOFTBUS_PRIO_URGENT   // Urgent priority (critical operations)
} softbus_priority_t;
```

## Initialization and Cleanup

### softbus_api_init
```c
int softbus_api_init(void);
```
**Purpose**: Initialize the software bus system
**Parameters**: None
**Return Value**:
- `SOFTBUS_OK`: Initialization successful
- `SOFTBUS_ERROR`: Initialization failed
**Note**: Must be called before using other APIs

### softbus_api_deinit
```c
void softbus_api_deinit(void);
```
**Purpose**: Clean up the software bus system
**Parameters**: None
**Return Value**: None
**Note**: Call before program termination to release all resources

## Device Management

### softbus_api_register_device
```c
int softbus_api_register_device(device_type_t type, const char* device_name);
```
**Purpose**: Register a device to the software bus system
**Parameters**:
- `type`: Device type, see device_type_t
- `device_name`: Device name, max 32 characters
**Return Value**:
- `SOFTBUS_OK`: Registration successful
- `SOFTBUS_ERROR`: Registration failed
**Note**: Device name must be unique

### softbus_api_unregister_device
```c
int softbus_api_unregister_device(const char* device_name);
```
**Purpose**: Unregister a device from the software bus system
**Parameters**:
- `device_name`: Device name
**Return Value**:
- `SOFTBUS_OK`: Unregistration successful
- `SOFTBUS_ERROR`: Unregistration failed
**Note**: Cleans up all resources related to the device

## Message Management

### softbus_api_send_message
```c
int softbus_api_send_message(const char* target, message_type_t type, 
                           const char* message, softbus_priority_t priority);
```
**Purpose**: Send a message to a specific device
**Parameters**:
- `target`: Target device name
- `type`: Message type
- `message`: Message content
- `priority`: Message priority
**Return Value**:
- `SOFTBUS_OK`: Send successful
- `SOFTBUS_ERROR`: Send failed
**Note**: Maximum message length is 1024 bytes

### softbus_api_send_group_message
```c
int softbus_api_send_group_message(const char* group_name, message_type_t type,
                                 const char* message, softbus_priority_t priority);
```
**Purpose**: Send a message to a device group
**Parameters**:
- `group_name`: Group name
- `type`: Message type
- `message`: Message content
- `priority`: Message priority
**Return Value**:
- `SOFTBUS_OK`: Send successful
- `SOFTBUS_ERROR`: Send failed
**Note**: Message will be sent to all devices in the group

### softbus_api_process_device_messages
```c
int softbus_api_process_device_messages(const char* device_name);
```
**Purpose**: Process device message queue
**Parameters**:
- `device_name`: Device name
**Return Value**:
- `SOFTBUS_OK`: Processing successful
- `SOFTBUS_ERROR`: Processing failed
**Note**: Messages are processed in priority order

## Group Management

### softbus_api_create_device_group
```c
int softbus_api_create_device_group(const char* group_name);
```
**Purpose**: Create a device group
**Parameters**:
- `group_name`: Group name, max 32 characters
**Return Value**:
- `SOFTBUS_OK`: Creation successful
- `SOFTBUS_ERROR`: Creation failed
**Note**: Group name must be unique

### softbus_api_add_device_to_group
```c
int softbus_api_add_device_to_group(const char* group_name, const char* device_name);
```
**Purpose**: Add a device to a group
**Parameters**:
- `group_name`: Group name
- `device_name`: Device name
**Return Value**:
- `SOFTBUS_OK`: Addition successful
- `SOFTBUS_ERROR`: Addition failed
**Note**: A device can belong to multiple groups

### softbus_api_remove_device_from_group
```c
int softbus_api_remove_device_from_group(const char* group_name, const char* device_name);
```
**Purpose**: Remove a device from a group
**Parameters**:
- `group_name`: Group name
- `device_name`: Device name
**Return Value**:
- `SOFTBUS_OK`: Removal successful
- `SOFTBUS_ERROR`: Removal failed

### softbus_api_delete_device_group
```c
int softbus_api_delete_device_group(const char* group_name);
```
**Purpose**: Delete a device group
**Parameters**:
- `group_name`: Group name
**Return Value**:
- `SOFTBUS_OK`: Deletion successful
- `SOFTBUS_ERROR`: Deletion failed
**Note**: Automatically cleans up group relationships for all devices in the group

## Resource Management

### softbus_api_request_resource
```c
void* softbus_api_request_resource(const char* device_name, const char* resource_name);
```
**Purpose**: Request device resource
**Parameters**:
- `device_name`: Device name
- `resource_name`: Resource name
**Return Value**:
- Non-NULL: Resource pointer
- NULL: Request failed
**Note**: Returned resource pointer must be cast to appropriate type

### softbus_api_release_resource
```c
int softbus_api_release_resource(const char* device_name, const char* resource_name);
```
**Purpose**: Release device resource
**Parameters**:
- `device_name`: Device name
- `resource_name`: Resource name
**Return Value**:
- `SOFTBUS_OK`: Release successful
- `SOFTBUS_ERROR`: Release failed

## Error Codes

### SOFTBUS_OK
```c
#define SOFTBUS_OK    0
```
**Note**: Operation successful

### SOFTBUS_ERROR
```c
#define SOFTBUS_ERROR (-1)
```
**Note**: Operation failed

## Usage Limitations

1. Name Length Limitations
   - Device name: Max 32 characters
   - Group name: Max 32 characters
   - Resource name: Max 32 characters

2. Capacity Limitations
   - Maximum 32 devices per group
   - Maximum message size 1024 bytes
   - Number of device types cannot exceed DEVICE_TYPE_MAX

3. Thread Safety
   - All APIs are thread-safe
   - Resource access is mutually exclusive

4. Memory Usage
   - Message structures use fixed-size memory
   - No dynamic memory allocation
   - Avoids memory fragmentation 