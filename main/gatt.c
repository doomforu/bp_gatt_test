#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/ans/ble_svc_ans.h"

//Sluzba RSSI
static const ble_uuid128_t rssi_svc_uuid =
    BLE_UUID128_INIT(0x6c, 0x25, 0xe3, 0x1b, 0x1d, 0xd8, 0x4f, 0x01, 0xb1, 0x64, 0xd2, 0xdb, 0x31, 0xc6, 0x9c, 0x83);
static const ble_uuid128_t rssi_char_uuid =
    BLE_UUID128_INIT(0x6c, 0x25, 0xe3, 0x1b, 0x1d, 0xd8, 0x4f, 0x02, 0xb1, 0x64, 0xd2, 0xdb, 0x31, 0xc6, 0x9c, 0x83);
static uint16_t rssi_char_val_handle;

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        //Sluzba RSSI
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &rssi_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
                //Charakteristika RSSI
                .uuid = &rssi_char_uuid.u,
                .access_cb = rssi_char_access_cb, // Callback
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY, // Cteni a notifikace
                .val_handle = &rssi_char_val_handle,
            }, {
                0, // No more characteristics in this service
            } },
    },

    {
        0, // No more services
    },
};

int gatt_svr_init(void){
    int rc;
    ble_svc_gatt_init();
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }
    return 0;
}