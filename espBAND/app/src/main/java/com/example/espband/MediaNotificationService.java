package com.example.espband;

import android.service.notification.NotificationListenerService;
import android.util.Log;

public class MediaNotificationService extends NotificationListenerService {
    @Override
    public void onListenerConnected() {
        super.onListenerConnected();
        Log.d("MediaNotification", "Service connected and ready!");
    }
}
