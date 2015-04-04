package no.kimtore.caracas;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.content.Intent;

public class MainActivity extends Activity {
	
	protected Intent service;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	Log.i("caracas", "Caracas main activity started");
        super.onCreate(savedInstanceState);
    	Log.i("caracas", "Attempting to start PiService....");
    	service = new Intent(this, PiService.class);
        startService(service);
    }
}
