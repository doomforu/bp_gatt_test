#include <stdio.h>
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"


#define TAG "BP_GATT_Server"

static void start_advertising(void);
static void on_stack_sync(void);
static int gap_event(struct ble_gap_event *event, void *arg);

void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}
/*
static void on_stack_reset(int reason) {
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}
*/
static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    start_advertising();
}
static void start_advertising(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));

    /* Nastaveni priznaku pro advertising:
        BLE obecny discoverable rezim
        BLE only
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    // Nastaveni jmena pro advertising a uvedeni tx power level do advertising dat.
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)"BP_GATT_Server";
    fields.name_len = strlen((char *)fields.name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement data; rc=%d", rc);
        return;
    }

    //Zapnuti advertisingu
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error enabling advertisement; rc=%d", rc);
        return;
    }
}



static int gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connection established; handle=%d", event->connect.conn_handle);
            } else {
                ESP_LOGE(TAG, "Connection failed; status=%d", event->connect.status);
                start_advertising(); // Obnovit advertising po neuspesnem pripojeni
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected; reason=%d", event->disconnect.reason);
            start_advertising(); // Obnovit advertising po odpojeni
            break;

        default:
            ESP_LOGI(TAG, "Unhandled GAP event: %d", event->type);
            break;
    }
    return 0;
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret= nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE("NimBLE", "Failed to initialize NimBLE stack: %d", ret);
        return;
    }

    ble_hs_cfg.sync_cb = on_stack_sync;

    nimble_port_freertos_init(ble_host_task);

    return;
}
