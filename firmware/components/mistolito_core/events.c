#include "events.h"
#include "esp_log.h"

static const char *TAG = "EVENTS";

QueueHandle_t g_event_queue = NULL;

void events_init(void)
{
    if (g_event_queue == NULL) {
        g_event_queue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(game_event_t));
        ESP_LOGI(TAG, "Event queue created (size=%d)", EVENT_QUEUE_SIZE);
    }
}

bool events_send(game_event_t *event)
{
    if (g_event_queue == NULL || event == NULL) {
        return false;
    }
    BaseType_t ret = xQueueSend(g_event_queue, event, 0);
    return ret == pdTRUE;
}

bool events_receive(game_event_t *event, uint32_t timeout_ms)
{
    if (g_event_queue == NULL || event == NULL) {
        return false;
    }
    BaseType_t ret = xQueueReceive(g_event_queue, event, pdMS_TO_TICKS(timeout_ms));
    return ret == pdTRUE;
}
