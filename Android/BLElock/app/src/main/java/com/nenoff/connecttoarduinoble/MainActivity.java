package com.nenoff.connecttoarduinoble;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity implements BLEControllerListener {
    private TextView logView;
    private Button connectButton;
    private Button disconnectButton;
    private Button lockButton;
    private Button unlockButton;

    private BLEController bleController;
    private RemoteControl remoteControl;
    private String deviceAddress;

    private final byte LOCK = (byte) 1;
    private final byte UNLOCK = (byte) 0;

    @RequiresApi(api = Build.VERSION_CODES.O)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        this.bleController = BLEController.getInstance(this);
        this.remoteControl = new RemoteControl(this.bleController);

        this.logView = findViewById(R.id.logView);
        this.logView.setMovementMethod(new ScrollingMovementMethod());

        initConnectButton();
        initDisconnectButton();
        initLockButton();
        initUnlockButton();

        checkBLESupport();
        checkPermissions();

        disableButtons();

    }

    private void initConnectButton() {
        this.connectButton = findViewById(R.id.connectButton);
        this.connectButton.setOnClickListener(v -> {
            connectButton.setEnabled(false);
            log("Connecting...");
            bleController.connectToDevice(deviceAddress);
        });
    }

    private void initDisconnectButton() {
        this.disconnectButton = findViewById(R.id.disconnectButton);
        this.disconnectButton.setOnClickListener(v -> {
            disconnectButton.setEnabled(false);
            log("Disconnecting...");
            bleController.disconnect();
        });
    }

    private void initLockButton() {
        this.lockButton = findViewById(R.id.lock);
        this.lockButton.setOnClickListener(v -> {
            remoteControl.locking(LOCK);
        });
    }

    private void initUnlockButton() {
        this.unlockButton = findViewById(R.id.unlock);
        this.unlockButton.setOnClickListener(v -> {
            remoteControl.locking(UNLOCK);
        });
    }

    private void disableButtons() {
        runOnUiThread(() -> {
            connectButton.setEnabled(false);
            disconnectButton.setEnabled(false);
            lockButton.setEnabled(false);
            unlockButton.setEnabled(false);
        });
    }

    private void log(final String text) {
        runOnUiThread(() -> logView.setText(String.format("%s\n%s", logView.getText(), text)));
    }

    private void checkPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            log("\"Access Fine Location\" permission not granted yet!");
            log("Without this permission BLE devices cannot be searched!");
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.ACCESS_FINE_LOCATION},
                    42);
        }
    }

    private void checkBLESupport() {
        // Check if BLE is supported on the device.
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, "BLE not supported!", Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        if(!BluetoothAdapter.getDefaultAdapter().isEnabled()){
            Intent enableBTIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBTIntent, 1);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        this.deviceAddress = null;
        this.bleController = BLEController.getInstance(this);
        this.bleController.addBLEControllerListener(this);
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                == PackageManager.PERMISSION_GRANTED) {
            log("[BLE]\tSearching for the Locking Device...");
            this.bleController.init();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        this.bleController.removeBLEControllerListener(this);
    }

    @Override
    public void BLEControllerConnected() {
        log("[BLE]\tConnected");
        runOnUiThread(() -> {
            disconnectButton.setEnabled(true);
            lockButton.setEnabled(true);
            unlockButton.setEnabled(true);
        });
    }

    @Override
    public void BLEControllerDisconnected() {
        log("[BLE]\tDisconnected");
        disableButtons();
        runOnUiThread(() -> connectButton.setEnabled(true));
    }

    @Override
    public void BLEDeviceFound(String name, String address) {
        log("Device " + name + " found with address " + address);
        this.deviceAddress = address;
        this.connectButton.setEnabled(true);
    }
}
