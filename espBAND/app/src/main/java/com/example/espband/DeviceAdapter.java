package com.example.espband;

import android.bluetooth.BluetoothDevice;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.List;

public class DeviceAdapter extends RecyclerView.Adapter<DeviceAdapter.ViewHolder> {

    private final List<BluetoothDevice> devices = new ArrayList<>();
    private final List<Integer> rssis = new ArrayList<>();
    private final OnDeviceClickListener listener;

    public interface OnDeviceClickListener {
        void onDeviceClick(BluetoothDevice device);
    }

    public DeviceAdapter(OnDeviceClickListener listener) {
        this.listener = listener;
    }

    public void addDevice(BluetoothDevice device, int rssi) {
        int index = devices.indexOf(device);
        if (index == -1) {
            devices.add(device);
            rssis.add(rssi);
            notifyItemInserted(devices.size() - 1);
        } else {
            rssis.set(index, rssi);
            notifyItemChanged(index);
        }
    }

    public void clear() {
        int size = devices.size();
        devices.clear();
        rssis.clear();
        notifyItemRangeRemoved(0, size);
    }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_device, parent, false);
        return new ViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull ViewHolder holder, int position) {
        BluetoothDevice device = devices.get(position);
        int rssi = rssis.get(position);
        try {
            String name = device.getName();
            holder.tvName.setText((name != null && !name.isEmpty()) ? name : "[No Name]");
            holder.tvAddress.setText(device.getAddress() + " | RSSI: " + rssi + "dBm");
            holder.itemView.setOnClickListener(v -> listener.onDeviceClick(device));
        } catch (SecurityException e) {
            holder.tvName.setText("Security Exception");
            holder.tvAddress.setText(device.getAddress());
        }
    }

    @Override
    public int getItemCount() {
        return devices.size();
    }

    static class ViewHolder extends RecyclerView.ViewHolder {
        TextView tvName, tvAddress;

        ViewHolder(View itemView) {
            super(itemView);
            tvName = itemView.findViewById(R.id.tvDeviceName);
            tvAddress = itemView.findViewById(R.id.tvDeviceAddress);
        }
    }
}
