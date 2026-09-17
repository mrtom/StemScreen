using Toybox.BluetoothLowEnergy as Ble;
using Toybox.System;
using Toybox.Time;
using Toybox.Lang;

// Garmin is the central; Waveshare advertises one writable custom service.
class StemLink extends Ble.BleDelegate {
    const serviceUuid = Ble.stringToUuid("93e1b4a0-5c21-4e62-a738-9d608c17f201");
    const rxUuid = Ble.stringToUuid("93e1b4a1-5c21-4e62-a738-9d608c17f201");
    var counter = 0;
    var acked = 0;
    var visible = false;
    var status = "Starting";
    var active = false;
    var registered = false;
    var profileWaiting = false;
    var profileTicks = 0;
    var profileSubmitted = false;
    var profileResult = "wait";
    var scanResult = "wait";
    var advertisements = 0;
    var matches = 0;
    var scanning = false;
    var device = null;
    var rx = null;
    var pending = false;
    var pendingCounter = 0;
    var pendingTicks = 0;
    var idleTicks = 0;
    var retryTicks = 0;
    var failures = 0;

    function initialize() { BleDelegate.initialize(); }
    function start() {
        active = true;
        status = "Set delegate";
        try {
            Ble.setDelegate(self);
            status = "Registering";
            profileWaiting = true;
            profileTicks = 0;
            System.println("Stem BLE diag2: requesting profile registration");
            Ble.registerProfile({:uuid => serviceUuid,
                :characteristics => [{:uuid => rxUuid, :descriptors => []}]});
            System.println("Profile request returned");
            profileSubmitted = true;
            // Scan independently of registration, as Garmin's SDK example does.
            // Pairing and writes still require a successful profile callback.
            scan();
        } catch (e) {
            profileWaiting = false;
            profileResult = "error";
            status = "Profile error";
            System.println("BLE startup: " + e.getErrorMessage());
        }
    }
    function onProfileRegister(uuid, result) {
        System.println("Profile callback: " + uuid.toString() + " result=" + result.toString());
        if (!active) { return; }
        profileWaiting = false;
        if (!uuid.equals(serviceUuid)) {
            profileResult = "UUID?";
            status = "Profile UUID?";
            return;
        }
        registered = (result == Ble.STATUS_SUCCESS);
        profileResult = result.toString();
        if (registered) { scan(); }
        else { status = "Profile " + result.toString(); }
    }
    function scan() {
        if (!active || !profileSubmitted || device != null || scanning) { return; }
        try {
            scanning = true;
            status = registered ? "Searching" : "Scan / profile wait";
            scanResult = "request";
            Ble.setScanState(Ble.SCAN_STATE_SCANNING);
        } catch (e) {
            scanning = false; retryTicks = 5; status = "Scan retry";
            scanResult = "error";
            System.println("Scan exception: " + e.getErrorMessage());
        }
    }
    function onScanStateChange(state, result) {
        if (!active) { return; }
        scanning = (result == Ble.STATUS_SUCCESS && state == Ble.SCAN_STATE_SCANNING);
        scanResult = result == Ble.STATUS_SUCCESS ? (scanning ? "on" : "off") : result.toString();
        System.println("Scan state=" + state.toString() + " result=" + result.toString());
        if (result != Ble.STATUS_SUCCESS) { status = "Scan " + result.toString(); retryTicks = 5; }
    }
    function onScanResults(results) {
        if (!active || device != null || retryTicks > 0) { return; }
        for (var result = results.next(); result != null; result = results.next()) {
            if (!(result instanceof Ble.ScanResult)) { continue; }
            if (advertisements < 9999) { advertisements++; }
            var uuids = result.getServiceUuids();
            for (var uuid = uuids.next(); uuid != null; uuid = uuids.next()) {
                if (uuid.equals(serviceUuid)) {
                    if (matches < 9999) { matches++; }
                    // Observing an advertisement is not proof of registration.
                    if (!registered) { break; }
                    try {
                        Ble.setScanState(Ble.SCAN_STATE_OFF);
                        scanning = false;
                        status = "Connecting";
                        idleTicks = 0;
                        device = Ble.pairDevice(result);
                        if (device == null) { retryTicks = 5; status = "Pair retry"; }
                    } catch (e) { retryTicks = 5; status = "Pair error"; }
                    return;
                }
            }
        }
    }
    function onConnectedStateChanged(changed, state) {
        if (!active || device == null || !changed.equals(device)) { return; }
        rx = null; pending = false; pendingTicks = 0; idleTicks = 0;
        status = state == Ble.CONNECTION_STATE_CONNECTED ? "Connected" : "Reconnecting";
        // Keep the device handle: Garmin attempts reconnection automatically.
    }
    function resetConnection() {
        var oldDevice = device;
        device = null; rx = null; pending = false; pendingTicks = 0;
        idleTicks = 0; failures = 0; retryTicks = 3;
        status = "Retrying";
        if (oldDevice != null) {
            try { Ble.unpairDevice(oldDevice); } catch (e) { System.println("Unpair failed"); }
        }
    }
    function tick() {
        // Counter means compute calls, NOT ride seconds. Keep it nonnegative.
        counter = counter == 2147483647 ? 0 : counter + 1;
        if (!active) { return; }
        if (!registered) {
            if (profileWaiting && profileTicks < 10) {
                profileTicks++;
                if (profileTicks == 10) {
                    status = "No profile reply";
                    System.println("No matching profile callback after 10 compute ticks");
                }
            }
            if (profileSubmitted && profileWaiting) {
                if (retryTicks > 0) { retryTicks--; }
                else if (!scanning) { scan(); }
            }
            return;
        }
        if (retryTicks > 0) { retryTicks--; return; }
        if (device == null) { scan(); return; }
        if (!device.isConnected()) {
            idleTicks++;
            if (idleTicks >= 25) { resetConnection(); }
            return;
        }
        try {
            if (rx == null) {
                var service = device.getService(serviceUuid);
                if (service != null) { rx = service.getCharacteristic(rxUuid); }
                if (rx == null) {
                    status = "Finding service";
                    idleTicks++;
                    if (idleTicks >= 10) { resetConnection(); }
                    return;
                }
            }
            idleTicks = 0;
            if (pending) {
                pendingTicks++;
                if (pendingTicks >= 5) { resetConnection(); }
                return;
            }
            var data = [0x42,0x53,1,1,0,0,0,0,0,0,0,0,0,0,0,0]b;
            put32(data, 4, counter);
            put32(data, 8, Time.now().value());
            data[12] = visible ? 1 : 0;
            pending = true; pendingTicks = 0; pendingCounter = counter;
            rx.requestWrite(data, {:writeType => Ble.WRITE_TYPE_WITH_RESPONSE});
        } catch (e) {
            System.println(e.getErrorMessage());
            resetConnection();
        }
    }
    function put32(data as Lang.ByteArray, offset as Lang.Number, value as Lang.Number) as Void {
        for (var i = 0; i < 4; i++) { data[offset+i] = (value >> (i*8)) & 0xff; }
    }
    function onCharacteristicWrite(characteristic, result) {
        if (!active || rx == null || !characteristic.equals(rx) || !pending) { return; }
        pending = false; pendingTicks = 0;
        if (result == Ble.STATUS_SUCCESS) {
            acked = pendingCounter; failures = 0; status = "Sending";
        } else {
            failures++; status = "Write " + result.toString();
            if (failures >= 3) { resetConnection(); }
        }
    }
    function stop() {
        active = false;
        try { Ble.setScanState(Ble.SCAN_STATE_OFF); } catch (e) { }
        resetConnection();
    }
}
