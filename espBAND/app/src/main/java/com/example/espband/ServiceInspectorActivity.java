package com.example.espband;

import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.Intent;
import android.os.Bundle;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

import java.util.List;

public class ServiceInspectorActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(32, 32, 32, 32);
        setContentView(layout);

        List<BluetoothGattService> services = BleManager.getInstance().getDiscoveredServices();
        
        for (BluetoothGattService service : services) {
            TextView tvService = new TextView(this);
            tvService.setText("Service: " + service.getUuid().toString());
            tvService.setTextSize(16);
            tvService.setPadding(0, 16, 0, 0);
            layout.addView(tvService);

            for (BluetoothGattCharacteristic characteristic : service.getCharacteristics()) {
                TextView tvChar = new TextView(this);
                String props = BleManager.getInstance().getProperties(characteristic);
                tvChar.setText("  Char: " + characteristic.getUuid().toString() + "\n  Props: " + props);
                tvChar.setPadding(16, 8, 0, 16);
                
                // Make clickable if it supports write
                if ((characteristic.getProperties() & BluetoothGattCharacteristic.PROPERTY_WRITE) != 0) {
                    tvChar.setBackgroundColor(0xFFEEEEEE);
                    tvChar.setOnClickListener(v -> {
                        Intent intent = new Intent(this, TimeSyncActivity.class);
                        intent.putExtra("service_uuid", service.getUuid().toString());
                        intent.putExtra("char_uuid", characteristic.getUuid().toString());
                        startActivity(intent);
                    });
                }
                
                layout.addView(tvChar);
            }
        }
    }
}
