package com.nenoff.connecttoarduinoble;

public interface BLEControllerListener {
    void BLEControllerConnected();
    void BLEControllerDisconnected();
    void BLEDeviceFound(String name, String address);
}
