package com.example.espband;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Button;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.UUID;

import android.media.MediaMetadata;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import java.util.List;

public class TimeSyncActivity extends AppCompatActivity {

    private TextView tvCurrentTime, tvCounter, tvTrackTitle;
    private Button btnRefresh;
    private Handler handler = new Handler(Looper.getMainLooper());
    private int counter = 0;
    private UUID serviceUuid, charUuid;

    private String lastSentTrack = "";
    private MediaController activeController;

    private final MediaController.Callback mediaCallback = new MediaController.Callback() {
        @Override
        public void onMetadataChanged(MediaMetadata metadata) {
            runOnUiThread(() -> updateTrackInfo(metadata));
        }
    };

    private Runnable syncRunnable = new Runnable() {
        @Override
        public void run() {
            sendCurrentTrackToBle();
            sendTimeToBle();
            
            // Send date every 5 seconds
            if (counter % 5 == 0) {
                sendDateToBle();
            }
            
            handler.postDelayed(this, 1000); // 1 second interval
        }
    };

    private void sendTimeToBle() {
        String currentTime = new SimpleDateFormat("hh:mm a", Locale.getDefault()).format(new Date());
        BleManager.getInstance().sendData(currentTime, BleManager.TIME_SERVICE_UUID, BleManager.TIME_CHAR_UUID);
    }

    private void sendDateToBle() {
        String currentDate = new SimpleDateFormat("MMM dd, yyyy", Locale.getDefault()).format(new Date());
        BleManager.getInstance().sendData(currentDate, BleManager.DATE_SERVICE_UUID, BleManager.DATE_CHAR_UUID);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_time_sync);

        tvCurrentTime = findViewById(R.id.tvCurrentTime);
        tvCounter = findViewById(R.id.tvCounter);
        tvTrackTitle = findViewById(R.id.tvTrackTitle);

        serviceUuid = UUID.fromString(getIntent().getStringExtra("service_uuid"));
        charUuid = UUID.fromString(getIntent().getStringExtra("char_uuid"));

        // Use TRACK_CHAR_UUID to send data to ESP32
        BleManager.getInstance().setGattCallback(new BluetoothGattCallback() {
            @Override
            public void onCharacteristicChanged(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic) {
                if (characteristic.getUuid().equals(BleManager.CONTROL_CHAR_UUID)) {
                    String cmd = characteristic.getStringValue(0);
                    Log.e("DEBUG_CONTROL", "Received command: " + cmd);
                    if (cmd != null) handleMediaCommand(cmd);
                }
            }
        });
        // Listen for control commands
        BleManager.getInstance().enableNotifications(serviceUuid, BleManager.CONTROL_CHAR_UUID);
        
        // Initial sync
        sendDateToBle();
        
        handler.post(syncRunnable);
        refreshTrack();
    }

    private void handleMediaCommand(String cmd) {
        if (activeController == null) {
            Log.e("DEBUG_CONTROL", "activeController is null, refreshing...");
            refreshTrack(); // Attempt to re-fetch if null
            if (activeController == null) return;
        }

        android.media.AudioManager audioManager = (android.media.AudioManager) getSystemService(Context.AUDIO_SERVICE);

        android.media.session.MediaController.TransportControls transport = activeController.getTransportControls();
        if (cmd.startsWith("PP")) {
            Log.d("DEBUG_CONTROL", "Dispatching Play/Pause");
            if (activeController.getPlaybackState() != null) {
                int state = activeController.getPlaybackState().getState();
                if (state == android.media.session.PlaybackState.STATE_PLAYING) {
                    transport.pause();
                } else {
                    transport.play();
                }
            } else {
                // Fallback if state is unknown
                transport.playFromSearch(null, null);
            }
        } else if (cmd.startsWith("NX")) {
            Log.d("DEBUG_CONTROL", "Dispatching Next");
            transport.skipToNext();
        } else if (cmd.startsWith("PV")) {
            Log.d("DEBUG_CONTROL", "Dispatching Previous");
            transport.skipToPrevious();
        } else if (cmd.startsWith("VU")) {
            Log.d("DEBUG_CONTROL", "Volume Up");
            audioManager.adjustStreamVolume(android.media.AudioManager.STREAM_MUSIC, android.media.AudioManager.ADJUST_RAISE, android.media.AudioManager.FLAG_SHOW_UI);
        } else if (cmd.startsWith("VD")) {
            Log.d("DEBUG_CONTROL", "Volume Down");
            audioManager.adjustStreamVolume(android.media.AudioManager.STREAM_MUSIC, android.media.AudioManager.ADJUST_LOWER, android.media.AudioManager.FLAG_SHOW_UI);
        }
    }

    private void updateTrackInfo(MediaMetadata metadata) {
        if (metadata == null) return;
        String title = metadata.getString(MediaMetadata.METADATA_KEY_TITLE);
        if (title == null) title = "Unknown";
        
        tvTrackTitle.setText("Track: " + title);
        
        String state = "unknown";
        if (activeController != null && activeController.getPlaybackState() != null) {
            int playbackState = activeController.getPlaybackState().getState();
            if (playbackState == android.media.session.PlaybackState.STATE_PLAYING) state = "playing";
            else if (playbackState == android.media.session.PlaybackState.STATE_PAUSED) state = "paused";
        }
        
        sendCurrentTrackToBle(title + "*|*" + state);
    }

    private void sendCurrentTrackToBle() {
        if (activeController != null && activeController.getMetadata() != null) {
            updateTrackInfo(activeController.getMetadata());
        }
    }

    private void sendCurrentTrackToBle(String title) {
        if (!title.equals("Track: Loading...") && !title.startsWith("Track: None")) {
            BleManager.getInstance().sendData(title, serviceUuid, charUuid);
            lastSentTrack = title;
            counter++;
            tvCounter.setText("Syncs sent: " + counter);
            tvCurrentTime.setText("Last sync: " + new SimpleDateFormat("hh:mm:ss a", Locale.getDefault()).format(new Date()));
        }
    }

    private void refreshTrack() {
        Log.e("DEBUG_SYNC", "REFRESH_TRACK_CALLED");
        
        String enabledListeners = Settings.Secure.getString(getContentResolver(), "enabled_notification_listeners");
        boolean isEnabled = enabledListeners != null && enabledListeners.contains(getPackageName());
        Log.e("DEBUG_SYNC", "Notification access enabled: " + isEnabled);
        
        if (!isEnabled) {
            Log.e("DEBUG_SYNC", "Redirecting to settings");
            try {
                Intent intent = new Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS");
                startActivity(intent);
            } catch (Exception e) {
                Log.e("DEBUG_SYNC", "Failed to launch settings", e);
            }
            return;
        }

        MediaSessionManager manager = (MediaSessionManager) getSystemService(Context.MEDIA_SESSION_SERVICE);
        try {
            // Use the NotificationListenerService ComponentName to get active sessions
            android.content.ComponentName componentName = new android.content.ComponentName(this, MediaNotificationService.class);
            List<MediaController> controllers = manager.getActiveSessions(componentName);
            
            Log.e("DEBUG_SYNC", "Controllers found: " + (controllers != null ? controllers.size() : "null"));
            
            if (controllers != null && !controllers.isEmpty()) {
                activeController = controllers.get(0);
                activeController.registerCallback(mediaCallback);
                updateTrackInfo(activeController.getMetadata());
            } else {
                Log.e("DEBUG_SYNC", "No active controllers found");
                tvTrackTitle.setText("Track: None playing");
            }
        } catch (SecurityException e) {
            Log.e("DEBUG_SYNC", "SecurityException encountered!", e);
            tvTrackTitle.setText("Track: Access Denied.");
        } catch (Exception e) {
            Log.e("DEBUG_SYNC", "General Exception encountered!", e);
            tvTrackTitle.setText("Track: Error.");
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacks(syncRunnable);
        BleManager.getInstance().disconnect();
    }
}
