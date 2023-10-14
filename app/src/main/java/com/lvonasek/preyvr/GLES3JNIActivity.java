
package com.lvonasek.preyvr;


import static android.system.Os.setenv;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.util.Log;
import android.util.Pair;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowManager;

import com.drbeef.externalhapticsservice.HapticServiceClient;
import com.drbeef.externalhapticsservice.HapticsConstants;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
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

	private static final String APPLICATION = "Doom3Quest";
	private static final int CFG_VERSION_CURRENT = 4;
	private static final String CFG_VERSION_KEY = "CFG_VERSION_KEY";

	private static final String[] DATA_DEMO = {"demo00.pk4", "demo01.pk4", "demo02.pk4", "demo03.pk4", "demo04.pk4", "demo05.pk4", "demo06.pk4", "demo07.pk4"};
	private static final String[] DATA_FULL = {"pak000.pk4", "pak001.pk4", "pak002.pk4", "pak003.pk4", "pak004.pk4"};
	private static final String[] DATA_MODS = {"ar_ep1.pk4", "lostcity.pk4", "mod_sounds.pk4", "omapsp_leonprey.pk4", "revelations_demo.pk4", "SurprisinglyFocused.pk4", "zPREY_SKY123.pk4"};

	private File root = new File("/sdcard/PreyVR");
	private File base = new File(root, "preybase");
	private File saves = new File(root, "saves/preybase");

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
		if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED){
			requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
		} else {
			// Permissions have already been granted.
			create();
		}
	}

	/** Handles the user accepting the permission. */
	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
		if (requestCode == 1) {
			checkPermissionsAndInitialize();
		}
	}

	public void create() {

		//Base game
		base.mkdirs();
		copy_asset(base.getAbsolutePath(), "vr_support.pk4", true);
		new File(base, "vr_support_update.pk4").delete();

		//Config
		saves.mkdirs();
		String configFile;
		boolean updateConfig = false;
		switch (Build.PRODUCT)
		{
			//Quest 1
			case "monterey":
			case "vr_monterey":
				configFile = "quest1_default.cfg";
				break;
			//Quest 2, Quest Pro, unknown
			case "hollywood":
			case "seacliff":
			default:
				configFile = "quest2_default.cfg";
				break;
			//Quest 3
			case "eureka":
			case "stinson":
				configFile = "quest3_default.cfg";
				SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(this);
				updateConfig = pref.getInt(CFG_VERSION_KEY, 0) != CFG_VERSION_CURRENT;
				if (updateConfig) {
					Log.d(APPLICATION, "User config will be recreated.");
					SharedPreferences.Editor e = pref.edit();
					e.putInt(CFG_VERSION_KEY, CFG_VERSION_CURRENT);
					e.commit();
				}
				break;
		}
		copy_asset(saves.getAbsolutePath(), configFile, "preyconfig.cfg", updateConfig);

		//Demo or full version menu layout
		String demoLayout = "vr_support_demo.pk4";
		if (has_files(base, DATA_FULL)) {
			new File(base, demoLayout).delete();
		} else {
			copy_asset(base.getAbsolutePath(), demoLayout, true);
		}

		new Thread(() -> {
			//Unpack data
			if (has_files(base, DATA_FULL)) {
				if (!has_files(base, DATA_MODS)) {
					unpack_data(DATA_MODS);
				}
				delete_files(base, DATA_DEMO);
			} else if (!has_files(base, DATA_DEMO)) {
				unpack_data(DATA_DEMO);
			}
			runOnUiThread(this::startGame);
		}).start();
	}

	public void startGame() {

		//Init screen
		mView = new SurfaceView( this );
		setContentView( mView );
		mView.getHolder().addCallback( this );

		try {
			setenv("USER_FILES", root.getAbsolutePath(), true);
			setenv("GAMELIBDIR", getApplicationInfo().nativeLibraryDir, true);
			setenv("GAMETYPE", "16", true); // hard coded for now
		} catch (Exception e) {
			e.printStackTrace();
		}

		//Parse the config file for these values
		float ss = -1.0F;
		long msaa = 1; // default for both HMDs
		File config = new File(saves, "preyconfig.cfg");
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
						if (s.contains("vr_msaa")) {
							msaa = Long.parseLong(value);
						} else if (s.contains("vr_supersampling")) {
							ss = Float.parseFloat(value);
						}
					}
				}
				br.close();
			} catch (Exception e) {
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

		GLES3JNILib.onCreate( this, commandLineParams, ss, msaa );
	}

	private void unpack_data(String[] data) {
		for (String file : data) {
			AssetManager assets = this.getAssets();
			InputStream in = null;
			try {
				in = assets.open(file);
			} catch (IOException e) {
				System.exit(1);
			}
			if (!unpack_file(in, new File(base, file), false)) {
				System.exit(1);
			}
		}
	}

	public boolean unpack_file(InputStream is, File target, boolean forced) {
		try {
			if (target.exists() && !forced) {
				return true;
			}
			File temp = new File(root, "temp");
			int length;
			byte[] buffer = new byte[8192];
			FileOutputStream fos = new FileOutputStream(temp);
			while ((length = is.read(buffer)) != -1) {
				fos.write(buffer, 0, length);
			}
			is.close();
			if (temp.renameTo(target)) {
				return true;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return false;
	}

	public void delete_files(File base, String[] files) {
		for (String file : files) {
			new File(base, file).delete();
		}
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
		copy_asset(path, name, name, force);
	}

	public void copy_asset(String path, String srcName, String dstName, boolean force) {
		File f = new File(path + "/" + dstName);
		if (!f.exists() || force) {
			
			//Ensure we have an appropriate folder
			String fullname = path + "/" + dstName;
			String directory = fullname.substring(0, fullname.lastIndexOf("/"));
			new File(directory).mkdirs();
			_copy_asset(srcName, path + "/" + dstName);
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

		System.exit(0);
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
