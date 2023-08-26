
package com.lvonasek.preyvr;


import static android.system.Os.setenv;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.RemoteException;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.util.Pair;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

import com.drbeef.externalhapticsservice.HapticServiceClient;
import com.drbeef.externalhapticsservice.HapticsConstants;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;
import java.util.Vector;

@SuppressLint("SdCardPath") public class GLES3JNIActivity extends Activity implements SurfaceHolder.Callback
{
	private Vector<HapticServiceClient> externalHapticsServiceClients = new Vector<>();

	//Use a vector of pairs, it is possible a given package _could_ in the future support more than one haptic service
	//so a map here of Package -> Action would not work.
	private static Vector<Pair<String, String>> externalHapticsServiceDetails = new Vector<>();

	static
	{
		System.loadLibrary( "doom3" );

		//Add possible external haptic service details here
		externalHapticsServiceDetails.add(Pair.create(HapticsConstants.BHAPTICS_PACKAGE, HapticsConstants.BHAPTICS_ACTION_FILTER));
		externalHapticsServiceDetails.add(Pair.create(HapticsConstants.FORCETUBE_PACKAGE, HapticsConstants.FORCETUBE_ACTION_FILTER));
	}

	private static final int READ_EXTERNAL_STORAGE_PERMISSION_ID = 1;
	private static final int WRITE_EXTERNAL_STORAGE_PERMISSION_ID = 2;

	private static final String APPLICATION = "Doom3Quest";

	private static final String[] DATA_DEMO = {"demo00.pk4", "demo01.pk4", "demo02.pk4", "demo03.pk4", "demo04.pk4", "demo05.pk4", "demo06.pk4", "demo07.pk4"};
	private static final String[] DATA_FULL = {"pak000.pk4", "pak001.pk4", "pak002.pk4", "pak003.pk4", "pak004.pk4"};
	private static final String DATA_URL = "https://github.com/lvonasek/PreyVR/raw/master/data/";

	private File root = new File("/sdcard/PreyVR");
	private File base = new File(root, "preybase");

	private String commandLineParams;

	private SurfaceView mView;

	private boolean started;


	public void shutdown() {
		finish();
		System.exit(0);
	}


	public void haptic_event(String event, int position, int flags, int intensity, float angle, float yHeight)  {

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {

			if (externalHapticsServiceClient.hasService()) {
				try {
					externalHapticsServiceClient.getHapticsService().hapticEvent(APPLICATION, event, position, flags, intensity, angle, yHeight);
				}
				catch (RemoteException r)
				{
					Log.v(APPLICATION, r.toString());
				}
			}
		}
	}

	public void haptic_updateevent(String event, int intensity, float angle) {

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {

			if (externalHapticsServiceClient.hasService()) {
				try {
					externalHapticsServiceClient.getHapticsService().hapticUpdateEvent(APPLICATION, event, intensity, angle);
				} catch (RemoteException r) {
					Log.v(APPLICATION, r.toString());
				}
			}
		}
	}

	public void haptic_stopevent(String event) {

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {

			if (externalHapticsServiceClient.hasService()) {
				try {
					externalHapticsServiceClient.getHapticsService().hapticStopEvent(APPLICATION, event);
				} catch (RemoteException r) {
					Log.v(APPLICATION, r.toString());
				}
			}
		}
	}

	public void haptic_endframe() {

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {

			if (externalHapticsServiceClient.hasService()) {
				try {
					externalHapticsServiceClient.getHapticsService().hapticFrameTick();
				} catch (RemoteException r) {
					Log.v(APPLICATION, r.toString());
				}
			}
		}
	}

	public void haptic_enable() {

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {

			if (externalHapticsServiceClient.hasService()) {
				try {
					externalHapticsServiceClient.getHapticsService().hapticEnable();
				} catch (RemoteException r) {
					Log.v(APPLICATION, r.toString());
				}
			}
		}
	}

	public void haptic_disable() {

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {

			if (externalHapticsServiceClient.hasService()) {
				try {
					externalHapticsServiceClient.getHapticsService().hapticDisable();
				} catch (RemoteException r) {
					Log.v(APPLICATION, r.toString());
				}
			}
		}
	}

	@Override protected void onCreate( Bundle icicle )
	{
		super.onCreate( icicle );

		mView = new SurfaceView( this );
		setContentView( mView );
		mView.getHolder().addCallback( this );

		// Force the screen to stay on, rather than letting it dim and shut off
		// while the user is watching a movie.
		getWindow().addFlags( WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );

		// Force screen brightness to stay at maximum
		WindowManager.LayoutParams params = getWindow().getAttributes();
		params.screenBrightness = 1.0f;
		getWindow().setAttributes( params );

		if (!started) {
			commandLineParams = "doom3quest";
			Intent intent = getIntent();
			if (intent != null) {
				String map = intent.getDataString();
				if (map != null) {
					commandLineParams += " +map " + map;
				}
			}

			checkPermissionsAndInitialize();
			started = true;
		}
	}

	/** Initializes the Activity only if the permission has been granted. */
	private void checkPermissionsAndInitialize() {
		// Boilerplate for checking runtime permissions in Android.
		if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
				!= PackageManager.PERMISSION_GRANTED){
			ActivityCompat.requestPermissions(this,
					new String[]{Manifest.permission.READ_EXTERNAL_STORAGE,
							Manifest.permission.WRITE_EXTERNAL_STORAGE},
					WRITE_EXTERNAL_STORAGE_PERMISSION_ID);
		}
		else
		{
			// Permissions have already been granted.
			create();
		}
	}

	/** Handles the user accepting the permission. */
	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
		if (requestCode == WRITE_EXTERNAL_STORAGE_PERMISSION_ID) {
			checkPermissionsAndInitialize();
		}
	}

	public void create() {

		//Base game
		base.mkdirs();
		copy_asset(base.getAbsolutePath(), "quest1_default.cfg", true);
		copy_asset(base.getAbsolutePath(), "quest2_default.cfg", true);
		copy_asset(base.getAbsolutePath(), "vr_support.pk4", true);

		//Demo or full version menu layout
		String demoLayout = "vr_support_demo.pk4";
		if (has_files(base, DATA_FULL)) {
			new File(base, demoLayout).delete();
		} else {
			copy_asset(base.getAbsolutePath(), demoLayout, true);
		}

		//Download data
		String updateLayout = "vr_support_update.pk4";
		if (has_files(base, DATA_DEMO)) {
			new File(base, updateLayout).delete();
		} else if (has_files(base, DATA_FULL)) {
			new File(base, updateLayout).delete();
		} else {
			copy_asset(base.getAbsolutePath(), updateLayout, true);
			download_data(DATA_DEMO);
		}

		try {
			ApplicationInfo ai =  getApplicationInfo();

			setenv("USER_FILES", root.getAbsolutePath(), true);
			setenv("GAMELIBDIR", getApplicationInfo().nativeLibraryDir, true);
			setenv("GAMETYPE", "16", true); // hard coded for now
		} catch (Exception e) {
			e.printStackTrace();
		}

		//Parse the config file for these values
		long refresh = 60; // Default to 60
		float ss = -1.0F;
		long msaa = 1; // default for both HMDs
		File config = new File(base, "preyconfig.cfg");
		if(config.exists())
		{
			BufferedReader br;
			try {
				br = new BufferedReader(new FileReader(config));
				String s;
				while ((s=br.readLine())!=null) {
					int i1 = s.indexOf("\"");
					int i2 = s.lastIndexOf("\"");
					if (i1 != -1 && i2 != -1) {
						String value = s.substring(i1+1, i2);
						if (s.contains("vr_refresh")) {
							refresh = Long.parseLong(value);
						} else if (s.contains("vr_msaa")) {
							msaa = Long.parseLong(value);
						} else if (s.contains("vr_supersampling")) {
							ss = Float.parseFloat(value);
						}
					}
				}
				br.close();
			} catch (IOException | NumberFormatException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

		for (Pair<String, String> serviceDetail : externalHapticsServiceDetails) {
			HapticServiceClient client = new HapticServiceClient(this, (state, desc) -> {
				Log.v(APPLICATION, "ExternalHapticsService " + serviceDetail.second + ": " + desc);
			}, new Intent(serviceDetail.second)
					.setPackage(serviceDetail.first));

			client.bindService();
			externalHapticsServiceClients.add(client);
		}

		GLES3JNILib.onCreate( this, commandLineParams, refresh, ss, msaa );
	}

	private void download_data(String[] data) {
		new Thread(() -> {
			int index = 0;
			int count = data.length;
			for (String file : data) {
				index++;
				String url = DATA_URL + file;
				String progress = index + "/" + count;
				if (!download_file(url, new File(base, file), false, progress)) {
					GLES3JNILib.setText("#str_lubos_title", "Downloading data failed!");
					GLES3JNILib.setText("#str_lubos_text", "Restart the game to try it again.");
					GLES3JNILib.setText("#str_lubos_button", "Continue");
					return;
				}
			}
			GLES3JNILib.setText("#str_lubos_title", "Downloading data finished!");
			GLES3JNILib.setText("#str_lubos_text", "Restart the game to continue.");
			GLES3JNILib.setText("#str_lubos_button", "Continue");
		}).start();
	}

	public boolean download_file(String url, File target, boolean forced, String progress) {
		try {
			if (target.exists() && !forced) {
				return true;
			}
			File temp = new File(root, "temp");
			URL u = new URL(url);
			InputStream is = u.openStream();
			DataInputStream dis = new DataInputStream(is);

			int length;
			byte[] buffer = new byte[1024];
			FileOutputStream fos = new FileOutputStream(temp);
			while ((length = dis.read(buffer))>0) {
				GLES3JNILib.setText("#str_lubos_title", "Downloading data, please wait...");
				GLES3JNILib.setText("#str_lubos_text", "Downloading " + progress);
				GLES3JNILib.setText("#str_lubos_button", "Cancel");
				fos.write(buffer, 0, length);
			}
			dis.close();
			is.close();
			if (temp.renameTo(target)) {
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return false;
	}

	public boolean has_files(File base, String[] files) {
		for (String file : files) {
			if (!new File(base, file).exists()) {
				return false;
			}
		}
		return true;
	}
	
	public void copy_asset(String path, String name, boolean force) {
		File f = new File(path + "/" + name);
		if (!f.exists() || force) {
			
			//Ensure we have an appropriate folder
			String fullname = path + "/" + name;
			String directory = fullname.substring(0, fullname.lastIndexOf("/"));
			new File(directory).mkdirs();
			_copy_asset(name, path + "/" + name);
		}
	}

	public void _copy_asset(String name_in, String name_out) {
		AssetManager assets = this.getAssets();

		try {
			InputStream in = assets.open(name_in);
			OutputStream out = new FileOutputStream(name_out);

			copy_stream(in, out);

			out.close();
			in.close();

		} catch (Exception e) {

			e.printStackTrace();
		}

	}

	public static void copy_stream(InputStream in, OutputStream out)
			throws IOException {
		byte[] buf = new byte[1024];
		while (true) {
			int count = in.read(buf);
			if (count <= 0)
				break;
			out.write(buf, 0, count);
		}
	}

	@Override protected void onStart()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onStart()" );
		super.onStart();
		GLES3JNILib.onStart(this);
	}

	@Override protected void onResume()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onResume()" );
		super.onResume();
		GLES3JNILib.onResume();
	}

	@Override protected void onPause()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onPause()" );
		GLES3JNILib.onPause();
		super.onPause();
	}

	@Override protected void onDestroy()
	{
		Log.v(APPLICATION, "GLES3JNIActivity::onDestroy()" );
		GLES3JNILib.onDestroy();

		for (HapticServiceClient externalHapticsServiceClient : externalHapticsServiceClients) {
			externalHapticsServiceClient.stopBinding();
		}

		super.onDestroy();
	}

	@Override public void surfaceCreated( SurfaceHolder holder )
	{
		Log.v(APPLICATION, "GLES3JNIActivity::surfaceCreated()" );
		GLES3JNILib.onSurfaceCreated( holder.getSurface() );
	}

	@Override public void surfaceChanged( SurfaceHolder holder, int format, int width, int height )
	{
		Log.v(APPLICATION, "GLES3JNIActivity::surfaceChanged()" );
		GLES3JNILib.onSurfaceChanged( holder.getSurface() );
	}
	
	@Override public void surfaceDestroyed( SurfaceHolder holder )
	{
		Log.v(APPLICATION, "GLES3JNIActivity::surfaceDestroyed()" );
	}
}
