int gatt_svr_init(void);
void reset_rssi_subscription(void);
void gatt_subscribe_cb(struct ble_gap_event *event);
void send_rssi_notification(void);