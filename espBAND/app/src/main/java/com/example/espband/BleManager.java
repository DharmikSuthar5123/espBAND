package com.example.espband;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Build;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

public class BleManager {
    private static final String TAG = "BleManager";
    
    public static final UUID SERVICE_UUID = UUID.fromString("aae42004-acfa-487a-835c-1e9ce04f0c44");
    public static final UUID TRACK_CHAR_UUID = UUID.fromString("11111111-1111-1111-1111-111111111111");
    public static final UUID CONTROL_CHAR_UUID = UUID.fromString("22222222-2222-2222-2222-222222222222");

    private static BleManager instance;
    private BluetoothGatt bluetoothGatt;
    private BleConnectionListener listener;
    private BluetoothGattCallback externalCallback;

    public interface BleConnectionListener {
        void onConnected();
        void onDisconnected();
    }

    private BleManager() {}

    public static synchronized BleManager getInstance() {
        if (instance == null) {
            instance = new BleManager();
        }
        return instance;
    }

    public void setGattCallback(BluetoothGattCallback callback) {
        this.externalCallback = callback;
    }

    public void connect(Context context, BluetoothDevice device, BleConnectionListener listener) {
        this.listener = listener;
        try {
            bluetoothGatt = device.connectGatt(context.getApplicationContext(), false, gattCallback);
        } catch (SecurityException e) {
            Log.e(TAG, "SecurityException", e);
        }
    }

    public void disconnect() {
        if (bluetoothGatt != null) {
            try {
                bluetoothGatt.disconnect();
                bluetoothGatt.close();
            } catch (SecurityException e) {
                Log.e(TAG, "SecurityException", e);
            }
            bluetoothGatt = null;
        }
    }

    public boolean isConnected() {
        return bluetoothGatt != null;
    }

    public void sendData(String data, UUID serviceUuid, UUID charUuid) {
        if (bluetoothGatt == null) {
            Log.e(TAG, "sendData: bluetoothGatt is null");
            return;
        }

        BluetoothGattService service = bluetoothGatt.getService(serviceUuid);
        if (service == null) {
            Log.e(TAG, "sendData: service is null for " + serviceUuid);
            return;
        }

        BluetoothGattCharacteristic characteristic = service.getCharacteristic(charUuid);
        if (characteristic == null) {
            Log.e(TAG, "sendData: characteristic is null for " + charUuid);
            return;
        }

        Log.d(TAG, "sendData: Writing " + data + " to " + charUuid);

        try {
            // Using WRITE_TYPE_NO_RESPONSE is often required for specific ESP32 firmware configurations
            characteristic.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                bluetoothGatt.writeCharacteristic(characteristic, data.getBytes(), BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
            } else {
                characteristic.setValue(data.getBytes());
                bluetoothGatt.writeCharacteristic(characteristic);
            }
        } catch (SecurityException e) {
            Log.e(TAG, "SecurityException writing characteristic", e);
        }
    }

    public void enableNotifications(UUID serviceUuid, UUID charUuid) {
        if (bluetoothGatt == null) return;
        BluetoothGattService service = bluetoothGatt.getService(serviceUuid);
        if (service == null) return;
        BluetoothGattCharacteristic characteristic = service.getCharacteristic(charUuid);
        if (characteristic == null) return;

        try {
            bluetoothGatt.setCharacteristicNotification(characteristic, true);
            BluetoothGattDescriptor descriptor = characteristic.getDescriptor(UUID.fromString("00002902-0000-1000-8000-00805f9b34fb"));
            if (descriptor != null) {
                descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                bluetoothGatt.writeDescriptor(descriptor);
            }
        } catch (SecurityException e) {
            Log.e(TAG, "Error enabling notifications", e);
        }
    }

    public List<BluetoothGattService> getDiscoveredServices() {
        if (bluetoothGatt == null) return new ArrayList<>();
        return bluetoothGatt.getServices();
    }

    public String getProperties(BluetoothGattCharacteristic characteristic) {
        int props = characteristic.getProperties();
        StringBuilder sb = new StringBuilder();
        if ((props & BluetoothGattCharacteristic.PROPERTY_READ) != 0) sb.append("Read ");
        if ((props & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0) sb.append("Write ");
        if ((props & BluetoothGattCharacteristic.PROPERTY_NOTIFY) != 0) sb.append("Notify ");
        if ((props & BluetoothGattCharacteristic.PROPERTY_INDICATE) != 0) sb.append("Indicate ");
        return sb.toString().trim().replace(" ", ", ");
    }

    private final BluetoothGattCallback gattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            if (externalCallback != null) externalCallback.onConnectionStateChange(gatt, status, newState);
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                try { gatt.discoverServices(); } catch (SecurityException e) { Log.e(TAG, "Error", e); }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                if (listener != null) listener.onDisconnected();
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            if (externalCallback != null) externalCallback.onServicesDiscovered(gatt, status);
            if (status == BluetoothGatt.GATT_SUCCESS) {
                Log.d(TAG, "Services discovered successfully");
                for (BluetoothGattService service : gatt.getServices()) {
                    Log.d(TAG, "Service: " + service.getUuid());
                }
                if (listener != null) listener.onConnected();
            } else {
                Log.e(TAG, "Service discovery failed with status: " + status);
            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
            if (externalCallback != null) externalCallback.onCharacteristicChanged(gatt, characteristic);
        }
    };
}
