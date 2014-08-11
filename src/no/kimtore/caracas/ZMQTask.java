package no.kimtore.caracas;

import android.os.AsyncTask;
import android.util.Log;
import org.zeromq.ZMQ;

public class ZMQTask extends AsyncTask<Void, Void, Void> {
	
	protected ZMQ.Context context;
	protected ZMQ.Socket socket;
	
	public final String DSN = "tcp://10.0.0.10:5555";
	
	private void setup() {
		this.context = ZMQ.context(1);
		Log.i("caracas", String.format("Attempting to connect ZMQ REQ socket %s...", this.DSN));
		this.socket = this.context.socket(ZMQ.REQ);
		this.socket.connect(this.DSN);
		Log.i("caracas", "Connected.");
	}
	
	private void teardown() {
		Log.i("caracas", "Destroying ZMQ REQ socket");
		this.socket.close();
		this.context.term();
		this.socket = null;
		this.context = null;
	}
	
	private boolean handshake() {
		String hs = "EHLO CARACAS";
		Log.d("caracas", String.format("Sending handshake: %s", hs));
		this.socket.send(hs);
		String s = this.socket.recvStr();
		Log.d("caracas", String.format("ZMQ said: %s", s));
		return (s == "World");
	}

	@Override
	protected Void doInBackground(Void... voids) {
		Log.d("caracas", "Background task started.");

		this.setup();
		
		if (this.handshake()) {
            while(!this.isCancelled()) {
                this.socket.send("Hello".getBytes(ZMQ.CHARSET), 0);
                Log.d("caracas", "Sent Hello to server");
                String reply = this.socket.recvStr();
                Log.d("caracas", String.format("Got reply from server: %s", reply));
            }
		} else {
			Log.i("caracas", "ZMQ handshake failed, epic fail");
		}

        this.teardown();
		Log.d("caracas", "Background task shut down.");
		return null;
	}

}
