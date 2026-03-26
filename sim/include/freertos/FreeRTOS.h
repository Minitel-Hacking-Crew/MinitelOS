#pragma once
#include <pthread.h>
#include <unistd.h>

typedef int      BaseType_t;
typedef void*    TaskHandle_t;
typedef uint32_t TickType_t;

#define pdPASS  1
#define pdFAIL  0
#define pdTRUE  1
#define pdFALSE 0

#define portTICK_PERIOD_MS      1
#define configMAX_PRIORITIES   25
#define ARDUINO_RUNNING_CORE    1

#define portMAX_DELAY           ((TickType_t)0xFFFFFFFF)

inline void vTaskDelay(TickType_t ticks) {
    usleep((useconds_t)ticks * 1000);
}

inline BaseType_t xTaskCreatePinnedToCore(
    void (*func)(void*),
    const char* name,
    uint32_t stackSize,
    void* params,
    int priority,
    TaskHandle_t* handle,
    int core)
{
    pthread_t tid;
    struct PthreadArgs { void (*fn)(void*); void* p; };
    auto* a = new PthreadArgs{func, params};
    pthread_create(&tid, nullptr, [](void* arg) -> void* {
        auto* a = (PthreadArgs*)arg;
        a->fn(a->p);
        delete a;
        return nullptr;
    }, a);
    pthread_detach(tid);
    if (handle) *handle = (TaskHandle_t)(uintptr_t)tid;
    return pdPASS;
}

inline void vTaskDelete(TaskHandle_t) {
    pthread_exit(nullptr);
}
