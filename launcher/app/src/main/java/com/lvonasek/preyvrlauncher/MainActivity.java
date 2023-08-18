package com.lvonasek.preyvrlauncher;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.StrictMode;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends Activity {

    private static final String DATA_URL = "https://github.com/lvonasek/PreyVR/raw/master/data/";

    private static final String[] DEMO_DATA = {
        "demo00.pk4","demo01.pk4","demo02.pk4","demo03.pk4","demo04.pk4","demo05.pk4","demo06.pk4","demo07.pk4"
    };

    private static final String GAME_PACKAGE = "com.lvonasek.preyvr";

    private static final String FULL_URL = "https://www.youtube.com/watch?v=OPXp0RYOSoA&t=542s";

    private static final String UPDATES_URL = "https://github.com/lvonasek/PreyVR/releases";
    private static final int WRITE_EXTERNAL_STORAGE_PERMISSION_ID = 2;

    private static final File root = new File("/sdcard/PreyVR");
    private static final File base = new File(root, "preybase");

    private Button mFullButton;
    private TextView mFullWarning;

    private ModView[] mModsView;
    private ProgressDialog mProgress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags( WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON );

        findViewById(R.id.game_prey).setOnClickListener(view -> startGame(null));
        mFullWarning = findViewById(R.id.full_warning);
        mFullButton = findViewById(R.id.full_button);
        mFullButton.setOnClickListener(view -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(FULL_URL))));
        mModsView = new ModView[]{
                findViewById(R.id.mod_ar),
                findViewById(R.id.mod_lp),
                findViewById(R.id.mod_lc),
                findViewById(R.id.mod_pr),
                findViewById(R.id.mod_sf),
                findViewById(R.id.mod_wh),
        };

        checkPermissionsAndInitialize(true);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mProgress != null) {
            mProgress.show();
        }
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

    private void create() {
        StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
        StrictMode.setVmPolicy(builder.build());
        base.mkdirs();

        if (hasFullVersion()) {
            mFullWarning.setVisibility(View.GONE);
            mFullButton.setVisibility(View.GONE);
            for (ModView mod : mModsView) {
                mod.setOnClickListener(view -> startModGame(mod.getFilename(), mod.getMapname()));
                mod.setVisibility(View.VISIBLE);
            }
        }super.onResume();
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

    private void downloadData(String[] data) {
        mProgress = new ProgressDialog(this);
        mProgress.setMessage(getString(R.string.downloading));
        mProgress.setCancelable(false);
        mProgress.show();

        new Thread(() -> {
            int index = 0;
            int count = data.length;
            for (String file : data) {
                index++;
                String progress = index + "/" + count;
                runOnUiThread(() -> mProgress.setMessage(getString(R.string.downloading) + " " + progress));
                String url = DATA_URL + file;
                if (!downloadFile(url, new File(base, file), false)) {
                    runOnUiThread(() -> {
                        new AlertDialog.Builder(getApplicationContext())
                                .setTitle(R.string.app_name)
                                .setMessage(R.string.download_failed)
                                .setIcon(android.R.drawable.ic_dialog_alert)
                                .setNegativeButton(android.R.string.no, null).show();
                        mProgress.dismiss();
                        mProgress = null;
                    });
                    return;
                }
            }
            runOnUiThread(() -> {
                mProgress.dismiss();
                mProgress = null;
            });
        }).start();
    }

    private boolean hasDemoVersion() {
        return new File(base, "demo07.pk4").exists();
    }
    private boolean hasFullVersion() {
       return new File(base, "pak004.pk4").exists();
    }

    private void startModGame(String filename, String mapname) {
        if (!new File(base, filename).exists()) {
            new AlertDialog.Builder(this)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.mod_not_installed)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.yes, (dialog, whichButton) -> {
                        String[] data = {"mod_sounds.pk4", filename};
                        downloadData(data);
                    })
                    .setNegativeButton(android.R.string.no, null).show();
        } else {
            startGame(mapname);
        }
    }

    private void startGame(String map) {
        try {
            if (!hasFullVersion() && !hasDemoVersion()) {
                new AlertDialog.Builder(this)
                        .setTitle(R.string.app_name)
                        .setMessage(R.string.no_data)
                        .setIcon(android.R.drawable.ic_dialog_alert)
                        .setPositiveButton(android.R.string.yes, (dialog, whichButton) -> downloadData(DEMO_DATA))
                        .setNegativeButton(android.R.string.no, null).show();
                return;
            }

            Intent intent = getPackageManager().getLaunchIntentForPackage(GAME_PACKAGE);
            if (map != null) {
                intent.setData(Uri.parse(map));
            }
            boolean hasGameInstalled = false;
            for (ApplicationInfo app : getPackageManager().getInstalledApplications(PackageManager.GET_META_DATA)) {
                if (app.packageName.compareTo(GAME_PACKAGE) == 0) {
                    hasGameInstalled = true;
                    break;
                }
            }
            if (!hasGameInstalled) {
                throw new Exception("Game not installed!");
            }
            finish();
            new Timer().schedule(new TimerTask() {
                @Override
                public void run() {
                    startActivity(intent);
                }
            }, 600);
        } catch (Exception e) {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(UPDATES_URL));
            new AlertDialog.Builder(this)
                    .setTitle(R.string.app_name)
                    .setMessage(R.string.not_installed)
                    .setIcon(android.R.drawable.ic_dialog_alert)
                    .setPositiveButton(android.R.string.yes, (dialog, whichButton) -> startActivity(intent))
                    .setNegativeButton(android.R.string.no, null).show();
            e.printStackTrace();
        }
    }
}