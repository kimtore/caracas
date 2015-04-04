package no.kimtore.caracas;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.BatteryManager;
import android.os.IBinder;
import android.provider.Settings;
import android.util.Log;

public class PiService extends Service {
	
	protected BroadcastReceiver receiver;
	
	protected void set_airplane_mode(int state) {
		// toggle airplane mode
		Settings.Global.putInt(getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, state);

		// Post an intent to reload
		Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
		//intent.putExtra("state", state != 1);
		sendBroadcast(intent);
	}

	protected void create_power_receiver() {
	    receiver = new BroadcastReceiver() {
	        public void onReceive(Context context, Intent intent) {
	            int plugged = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
	            if (plugged == BatteryManager.BATTERY_PLUGGED_AC) {
                    Log.d("caracas", String.format("On AC power"));
                    set_airplane_mode(0);
	            } else if (plugged == BatteryManager.BATTERY_PLUGGED_USB) {
                    Log.d("caracas", String.format("On USB power"));
                    set_airplane_mode(0);
	            } else if (plugged == 0) {
                    Log.d("caracas", String.format("On battery power"));
                    set_airplane_mode(1);
	            } else {
                    Log.d("caracas", String.format("Power source unknown!"));
                    set_airplane_mode(1);
	            }
	        }
	    };
	    IntentFilter filter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
	    registerReceiver(receiver, filter);
	}	
	
	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
	
	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.i("caracas", "Caracas PiService started.");
		create_power_receiver();
		//new ZMQTask().execute();
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
