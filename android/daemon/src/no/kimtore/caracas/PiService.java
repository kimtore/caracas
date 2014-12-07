package no.kimtore.caracas;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class PiService extends Service {
	
	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.i("caracas", "Caracas PiService started.");
		new ZMQTask().execute();
		return START_STICKY;
	}
	
	@Override
	public void onDestroy() {
		Log.i("caracas", "Destroying PiService.");
	}
	
	public void onCreate() {
		Log.i("caracas", "Creating PiService.");
	}
}
