package no.kimtore.caracas;

import android.os.AsyncTask;
import android.util.Log;
import org.zeromq.ZMQ;

public class ZMQTask extends AsyncTask<Void, Void, Void> {
	
	protected ZMQ.Context context;
	protected ZMQ.Socket req;
	
	public final String DSN = "tcp://10.0.0.10:5555";

	private void setup() {
		context = ZMQ.context(1);
		Log.i("caracas", String.format("Attempting to connect ZMQ REQ socket %s...", DSN));
		req = context.socket(ZMQ.REQ);
		req.connect(DSN);
		Log.i("caracas", "Connected.");
	}
	
	private void teardown() {
		Log.i("caracas", "Destroying ZMQ REQ socket");
		req.close();
		context.term();
		req = null;
		context = null;
	}
	
	private void send_req(String msg) {
		Log.d("caracas", String.format("REQ<send>: %s", msg));
		req.send(msg);
	}
	
	private String recv_req() {
		String msg = req.recvStr();
		Log.d("caracas", String.format("REQ<recv>: %s", msg));
		return msg;
	}
	
	private int get_power_status() {
		send_req("get_power_status");
		String msg = recv_req();
		return Integer.parseInt(msg);
	}

	@Override
	protected Void doInBackground(Void... voids) {
		Log.d("caracas", "Background task started.");

		setup();
		get_power_status();
		teardown();

		Log.d("caracas", "Background task shut down.");
		return null;
	}

}
