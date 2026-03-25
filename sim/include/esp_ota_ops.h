#pragma once
// OTA not available in sim
typedef void* esp_app_desc_t;
inline const esp_app_desc_t* esp_ota_get_running_partition() { return nullptr; }
inline const esp_app_desc_t* esp_app_get_description() { return nullptr; }
