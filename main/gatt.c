#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define TAG "GATT"

static int rssi_char_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

// Sluzba RSSI
static const ble_uuid128_t rssi_svc_uuid =
    BLE_UUID128_INIT(0x6c, 0x25, 0xe3, 0x1b, 0x1d, 0xd8, 0x4f, 0x01, 0xb1, 0x64, 0xd2, 0xdb, 0x31, 0xc6, 0x9c, 0x83);
static const ble_uuid128_t rssi_char_uuid =
    BLE_UUID128_INIT(0x6c, 0x25, 0xe3, 0x1b, 0x1d, 0xd8, 0x4f, 0x02, 0xb1, 0x64, 0xd2, 0xdb, 0x31, 0xc6, 0x9c, 0x83);
static uint16_t rssi_char_val_handle;
//static int8_t rssi_chr_value =0;                          // Buffer pro uchovani RSSI hodnoty
static uint16_t rssi_chr_conn_handle = BLE_HS_CONN_HANDLE_NONE; // Handle pro pripojeni, ktery bude drzet aktualni spojeni
static bool rssi_chr_notify_enabled = false;                    // Flag pro indikaci, zda je notifikace povolena

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        // Sluzba RSSI
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &rssi_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){{
                                                           // Charakteristika RSSI
                                                           .uuid = &rssi_char_uuid.u,
                                                           .access_cb = rssi_char_access_cb,                     // Callback
                                                           .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, // Cteni a notifikace
                                                           .val_handle = &rssi_char_val_handle,
                                                       },
                                                       {
                                                           0, // No more characteristics in this service
                                                       }},
    },

    {
        0, // No more services
    },
};

static int rssi_char_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    int8_t rssi_value = 0;
    switch (ctxt->op)
    {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        rc = ble_gap_conn_rssi(conn_handle, &rssi_value);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "Failed to read RSSI; rc=%d", rc);
            return BLE_ATT_ERR_UNLIKELY;
        }
        rc = os_mbuf_append(ctxt->om, &rssi_value, sizeof(rssi_value));
        if (rc != 0)
        {
            ESP_LOGE(TAG, "Failed to read RSSI value; rc=%d", rc);
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        ESP_LOGI(TAG, "RSSI value read: %d dBm", rssi_value);
        break;

    default:
        return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}

int gatt_svr_init(void)
{
    int rc;
    ble_svc_gatt_init();
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0)
    {
        return rc;
    }
    return 0;
}

void reset_rssi_subscription(void)
{
    rssi_chr_notify_enabled = false;
    rssi_chr_conn_handle = BLE_HS_CONN_HANDLE_NONE;
}

void gatt_subscribe_cb(struct ble_gap_event *event)
{
    if (event->subscribe.attr_handle == rssi_char_val_handle)
    {
        rssi_chr_notify_enabled = event->subscribe.cur_notify;
        rssi_chr_conn_handle = event->subscribe.conn_handle;
        ESP_LOGI(TAG, "RSSI notification %s, conn_handle: %d", rssi_chr_notify_enabled ? "enabled" : "disabled", rssi_chr_conn_handle);
    }
}

void send_rssi_notification(void)
{
    if (rssi_chr_notify_enabled)
    {
        if (rssi_chr_conn_handle == BLE_HS_CONN_HANDLE_NONE)
        {
            ESP_LOGW(TAG, "No active connection for RSSI notification");
            return;
        }
        int8_t rssi_dbm;
        int rc = ble_gap_conn_rssi(rssi_chr_conn_handle, &rssi_dbm);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "Failed to read RSSI; rc=%d", rc);
            return;
        }
        struct os_mbuf *om = ble_hs_mbuf_from_flat(&rssi_dbm, sizeof(rssi_dbm));
        if (om == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate mbuf for RSSI notification");
            return;
        }

        rc =ble_gatts_notify_custom(rssi_chr_conn_handle, rssi_char_val_handle, om);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "Failed to send RSSI notification; rc=%d", rc);
            return;
        }
        else
        {
            ESP_LOGI(TAG, "RSSI notification sent: %d dBm", rssi_dbm);
        }
    }
}