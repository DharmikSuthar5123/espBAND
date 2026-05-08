package com.example.espband;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

import android.bluetooth.le.ScanSettings;
import android.location.LocationManager;
import android.provider.Settings;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "BLE_SCANNER";
    private static final int PERMISSION_REQUEST_CODE = 101;
    private static final String PREFS_NAME = "BlePrefs";
    private static final String KEY_DEVICE_ADDRESS = "last_connected_device";

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothLeScanner bluetoothLeScanner;
    private boolean scanning;
    private Handler handler = new Handler(Looper.getMainLooper());

    private DeviceAdapter deviceAdapter;
    private BluetoothGatt bluetoothGatt;

    private static final long SCAN_PERIOD = 20000; // Increased to 20s

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button btnRefresh = findViewById(R.id.btnRefresh);
        RecyclerView rvDevices = findViewById(R.id.rvDevices);

        deviceAdapter = new DeviceAdapter(this::connectToDevice);
        rvDevices.setLayoutManager(new LinearLayoutManager(this));
        rvDevices.setAdapter(deviceAdapter);

        BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        bluetoothAdapter = bluetoothManager.getAdapter();

        if (bluetoothAdapter == null) {
            Toast.makeText(this, "Bluetooth not supported", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }

        // Auto-connect logic
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        String savedAddress = prefs.getString(KEY_DEVICE_ADDRESS, null);
        
        if (savedAddress != null && bluetoothAdapter.isEnabled()) {
            BluetoothDevice device = bluetoothAdapter.getRemoteDevice(savedAddress);
            if (device != null) {
                Toast.makeText(this, "Auto-connecting to " + savedAddress, Toast.LENGTH_SHORT).show();
                connectToDevice(device);
            }
        }

        btnRefresh.setOnClickListener(v -> {
            if (!bluetoothAdapter.isEnabled()) {
                Toast.makeText(this, "Please enable Bluetooth", Toast.LENGTH_SHORT).show();
                return;
            }
            if (checkPermissions()) {
                if (isLocationEnabled()) {
                    startScan();
                } else {
                    Toast.makeText(this, "Please enable Location services", Toast.LENGTH_LONG).show();
                    startActivity(new Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS));
                }
            } else {
                requestPermissions();
            }
        });
    }

    private boolean isLocationEnabled() {
        LocationManager locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        return locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER) ||
               locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER);
    }

    private boolean checkPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            boolean base = ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED &&
                   ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED &&
                   ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED;
            
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                return base && ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED;
            }
            return base;
        } else {
            return ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED;
        }
    }

    private void requestPermissions() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            List<String> perms = new ArrayList<>();
            perms.add(Manifest.permission.BLUETOOTH_SCAN);
            perms.add(Manifest.permission.BLUETOOTH_CONNECT);
            perms.add(Manifest.permission.ACCESS_FINE_LOCATION);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                perms.add(Manifest.permission.POST_NOTIFICATIONS);
            }
            ActivityCompat.requestPermissions(this, perms.toArray(new String[0]), PERMISSION_REQUEST_CODE);
        } else {
            ActivityCompat.requestPermissions(this, new String[]{
                    Manifest.permission.ACCESS_FINE_LOCATION
            }, PERMISSION_REQUEST_CODE);
        }
    }

    private void startScan() {
        if (scanning) return;

        bluetoothLeScanner = bluetoothAdapter.getBluetoothLeScanner();
        if (bluetoothLeScanner == null) {
            Toast.makeText(this, "Cannot get BLE Scanner - check if Bluetooth is ON", Toast.LENGTH_SHORT).show();
            return;
        }

        deviceAdapter.clear();
        scanning = true;
        
        handler.postDelayed(() -> {
            if (scanning) {
                scanning = false;
                try {
                    bluetoothLeScanner.stopScan(scanCallback);
                    Toast.makeText(this, "Scan stopped", Toast.LENGTH_SHORT).show();
                } catch (SecurityException e) {
                    Log.e(TAG, "SecurityException while stopping scan", e);
                }
            }
        }, SCAN_PERIOD);

        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();

        try {
            bluetoothLeScanner.startScan(null, settings, scanCallback);
            Toast.makeText(this, "Scanning for ESP32...", Toast.LENGTH_SHORT).show();
            Log.d(TAG, "Scan started");
        } catch (SecurityException e) {
            Log.e(TAG, "SecurityException while starting scan", e);
        }
    }

    private final ScanCallback scanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result.getDevice();
            int rssi = result.getRssi();
            Log.d(TAG, "Found device: " + device.getAddress() + " RSSI: " + rssi);
            runOnUiThread(() -> deviceAdapter.addDevice(device, rssi));
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            for (ScanResult result : results) {
                onScanResult(ScanSettings.CALLBACK_TYPE_ALL_MATCHES, result);
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            Log.e(TAG, "Scan failed with error: " + errorCode);
            runOnUiThread(() -> Toast.makeText(MainActivity.this, "Scan failed: " + errorCode, Toast.LENGTH_SHORT).show());
        }
    };

    private void connectToDevice(BluetoothDevice device) {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                requestPermissions();
                return;
            }
        }

        // Save address
        getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
                .edit()
                .putString(KEY_DEVICE_ADDRESS, device.getAddress())
                .apply();

        Toast.makeText(this, "Connecting to " + device.getAddress(), Toast.LENGTH_SHORT).show();
        
        BleManager.getInstance().connect(this, device, new BleManager.BleConnectionListener() {
            @Override
            public void onConnected() {
                runOnUiThread(() -> {
                    Toast.makeText(MainActivity.this, "Connected!", Toast.LENGTH_SHORT).show();
                    Intent intent = new Intent(MainActivity.this, TimeSyncActivity.class);
                    // Use hardcoded UUIDs as per firmware requirements
                    intent.putExtra("service_uuid", "aae42004-acfa-487a-835c-1e9ce04f0c44");
                    intent.putExtra("char_uuid", "11111111-1111-1111-1111-111111111111");
                    startActivity(intent);
                });
            }

            @Override
            public void onDisconnected() {
                runOnUiThread(() -> Toast.makeText(MainActivity.this, "Disconnected", Toast.LENGTH_SHORT).show());
            }
        });
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                startScan();
            } else {
                Toast.makeText(this, "Permissions required for scanning", Toast.LENGTH_SHORT).show();
            }
        }
    }
}
