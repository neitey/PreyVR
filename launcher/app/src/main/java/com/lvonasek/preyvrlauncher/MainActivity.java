package com.lvonasek.preyvrlauncher;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.StrictMode;
import android.view.WindowManager;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;

public class MainActivity extends Activity {

    private static String DEMO_DATA_URL = "https://github.com/lvonasek/PreyVR/raw/master/data/";
    private static String GAME_PACKAGE = "com.lvonasek.preyvr";

    private static String UPDATES_URL = "https://github.com/lvonasek/PreyVR/releases";
    private static final int WRITE_EXTERNAL_STORAGE_PERMISSION_ID = 2;

    private static File root = new File("/sdcard/PreyVR");
    private static File base = new File(root, "preybase");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags( WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );

        findViewById(R.id.button_start).setOnClickListener(view -> startGame());

        checkPermissionsAndInitialize(true);
    }

    private void checkPermissionsAndInitialize(boolean ask) {
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            if (ask) {
                requestPermissions(new String[]{Manifest.permission.READ_EXTERNAL_STORAGE,
                                Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        WRITE_EXTERNAL_STORAGE_PERMISSION_ID);
            } else {
                finish();
            }
        }
        else {
            create();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] results) {
        if (requestCode == WRITE_EXTERNAL_STORAGE_PERMISSION_ID) {
            checkPermissionsAndInitialize(false);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        System.exit(0);
    }

    private void create() {
        StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
        StrictMode.setVmPolicy(builder.build());
        base.mkdirs();
    }

    public boolean downloadFile(String url, File target, boolean forced) {
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

    private void downloadDemo() {
        ProgressDialog pd = new ProgressDialog(this);
        pd.setMessage(getString(R.string.downloading));
        pd.setCancelable(false);
        pd.show();

        new Thread(() -> {
            int count = 8;
            for (int i = 0; i < count; i++) {
                int i1 = i + 1;
                runOnUiThread(() -> pd.setMessage(getString(R.string.downloading) + " " + i1 + "/" + count));
                String file = "demo0" + i + ".pk4";
                String url = DEMO_DATA_URL + file;
                if (!downloadFile(url, new File(base, file), false)) {
                    runOnUiThread(() -> {
                        new AlertDialog.Builder(getApplicationContext())
                                .setTitle(R.string.app_name)
                                .setMessage(R.string.download_failed)
                                .setIcon(android.R.drawable.ic_dialog_alert)
                                .setNegativeButton(android.R.string.no, null).show();
                        pd.dismiss();
                    });
                    return;
                }
            }
            runOnUiThread(pd::dismiss);
        }).start();
    }

    private void startGame() {
        try {
            if (!new File(base, "pak006.pk4").exists() && !new File(base, "demo07.pk4").exists()) {
                new AlertDialog.Builder(this)
                        .setTitle(R.string.app_name)
                        .setMessage(R.string.no_data)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setPositiveButton(android.R.string.yes, (dialog, whichButton) -> {
                            downloadDemo();
                        })
                        .setNegativeButton(android.R.string.no, null).show();
                return;
            }
            getApplicationContext().startActivity(getPackageManager().getLaunchIntentForPackage(GAME_PACKAGE));
        } catch (Exception e) {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.not_installed)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.yes, (dialog, whichButton) -> {
                        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(UPDATES_URL)));
                    })
                    .setNegativeButton(android.R.string.no, null).show();
            e.printStackTrace();
        }
    }
}