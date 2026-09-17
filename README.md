# Bike Stem Computer — first Bluetooth test

This sends a counter from your **Garmin Edge 530** to your **Waveshare ESP32-S3-LCD-1.28 non-touch** board, roughly once a second. The board only displays values it actually receives.

The aim is to prove that the connection works and keeps updating when you switch the Garmin to its map page. The clock, ride timer and rotating pages will come back in the next version. This counter is **not ride time**: it increments whenever Garmin calls the field's compute function, including while the activity timer is paused.

You will upload two programs: an Arduino sketch to the Waveshare, then a Connect IQ data field to the Garmin. There is no phone app or internet connection in the data path.

## 1. Upload the Waveshare program

1. Unzip this download. Keep the folder contents together.
2. Open **StemScreen/StemScreen.ino** in Arduino IDE. You should also see **BleProtocol.h** and **ReceiverState.h** tabs automatically.
3. Plug in the Waveshare. Use **ESP32S3 Dev Module** and the USB port that worked previously (your last port was `/dev/cu.usbmodem5ABA0067551`; the suffix can change).
4. Keep your working board settings. For reference: flash **16MB**, partition **16M Flash (3MB APP/9.9MB FATFS)**, PSRAM **QSPI PSRAM**, USB CDC On Boot **Disabled**, upload speed **115200**.
5. This sketch uses the same **Adafruit GC9A01A** and **Adafruit GFX** libraries as before. BLE comes with Espressif's board package; you do not need to install a separate Bluetooth library. It targets **esp32 by Espressif Systems 3.3.0**, selectable in Boards Manager.
6. Click **Upload**. After it completes, press the board's RESET/RUN button once if necessary.

**Expected result:** the round screen says **STEM RX d2**, **WAITING**, and `--`. That is correct until the Garmin program connects. You can leave the board plugged into your Mac for power.

If uploading stalls, use the same recovery sequence as before: hold BOOT, press and release RESET/RUN, release BOOT, then upload. Press RESET/RUN after upload. If the display is black, check the selected sketch and the Serial Monitor at **115200 baud**.

The original standalone clock/timer is included under **StandaloneBackup/StemScreen**. To restore it, open its `StemScreen.ino` and upload it. Keep the sketch inside its matching folder.

## 2. Set up Garmin development on your Mac

The Garmin source is included, but **there is no ready-to-copy Edge 530 PRG in this download**. Garmin requires sign-in to download its device definitions; I could run a general compiler check here but could not perform the final Edge 530 build. The following setup gets those definitions onto your Mac.

1. Install [Visual Studio Code](https://code.visualstudio.com/download) if you do not already have it.
2. Download the Mac **Connect IQ SDK Manager** from [Garmin's SDK page](https://developer.garmin.com/connect-iq/sdk/). Open it and sign in with your Garmin account.
3. In its **SDK** tab, install a stable SDK and make it active with **Use as SDK**. The source was checked with SDK **8.1.0**; you can select that version to match, or use a newer stable SDK.
4. In **Devices**, find **Edge 530** and download its device definition. Wait for the download to finish. This step is essential even though you physically own an Edge 530.
5. In VS Code's Extensions view, install **Monkey C**, published by **Garmin**. [Direct extension link](https://marketplace.visualstudio.com/items?itemName=garmin.monkey-c).
6. Press **Cmd–Shift–P**, run **Monkey C: Verify Installation**, and resolve any requirements it reports. If Java is missing, install a JDK supported by the extension; Java 17 was used for the general compiler check here. Restart VS Code after installing Java.
7. Press **Cmd–Shift–P** again and run **Monkey C: Generate a Developer Key**. Save the key somewhere you will retain, outside the downloaded project. This is the local signing key for your builds.

Garmin's [getting-started guide](https://developer.garmin.com/connect-iq/connect-iq-basics/getting-started/) covers the tools and key setup.

## 3. Build and copy the Garmin data field

1. In VS Code choose **File → Open Folder**, then select the **Garmin** folder in this download. You should see `manifest.xml`, `monkey.jungle`, `source` and `resources` in the sidebar. Open that folder itself, rather than an individual source file.
2. Plug your **Edge 530** into the Mac using a USB data cable. It should appear in Finder as a mounted Garmin drive. The Waveshare can remain connected separately.
3. Press **Cmd–Shift–P**, choose **Monkey C: Build for Device**, and select **Edge 530**. Choose an output folder on your Mac, such as a new `StemBuild` folder in Documents.
4. When the build finishes successfully, find its **.prg** file in that output folder. The exact filename is chosen by the build wizard/project.
5. In Finder, copy that `.prg` into **GARMIN/APPS** on the Edge. Copy the file itself, not the source folder or ZIP.
6. Eject the Garmin in Finder, then unplug its USB cable. Allow it to return to normal operation; restart it if the new field is not listed.

This is Garmin's supported [side-loading workflow](https://developer.garmin.com/connect-iq/connect-iq-basics/your-first-app/). There is no Connect IQ Store submission involved.

If Edge 530 is missing from the build menu, return to SDK Manager and confirm its definition is installed. If the build fails, copy the first error and the lines around it from VS Code's Output panel.

## 4. Put the field on a Garmin data page

On the Edge 530:

1. Go to **Menu → Settings → Activity Profiles**, then choose the cycling profile you will use.
2. Open **Data Screens → Add New → Data Screen**.
3. Select the **Connect IQ** category and **Stem BLE**. Finish with a layout containing just this one field, so its diagnostics are easy to read.
4. Return to that profile's ride screens and open the new page.

Garmin documents the screen setup in its [Edge 530 manual](https://www8.garmin.com/manuals/webhelp/edge530/EN-US/GUID-58CCEE56-34BF-44F4-ACCF-B81F6D716CA9.html).

**Do not use Add Sensor or your Mac's Bluetooth pairing screen.** The data field itself finds and connects to the board. For the first test, have one BikeStem board powered and keep it within a metre of the Garmin. Add this field only once to the active profile.

**Expected result:** Garmin progresses through **Searching → Connecting → Sending**. The Waveshare changes to **LIVE** and displays a growing number. Allow about 30 seconds for initial discovery.

The Garmin's large number counts compute calls, including calls before connection. It therefore need not begin at 1 on the board. **ACK** is the last counter acknowledged by the Bluetooth write operation; the Waveshare screen confirms the payload was decoded. A one-tick difference between screens is normal.

## 5. Run the useful tests

Start a short test activity while stationary. If Auto Pause intervenes, note that separately or temporarily disable it for this test.

| Test | What to look for |
| --- | --- |
| Leave Stem BLE visible for 30 seconds | Board stays LIVE and its received counter keeps increasing. |
| Switch the Garmin to its map page for 60 seconds | Board remains LIVE and its counter continues increasing. The view flag may stay set. |
| Return to the data field | Updates continue. Do not use the view flag as proof of the selected page. |
| Pause the Garmin activity timer | This diagnostic counter should continue; it is not yet the activity timer. |
| Unplug the Waveshare, wait 10 seconds, reconnect power | Board starts at WAITING; the Garmin should reconnect and resume updates. Allow up to 45 seconds. |
| Power off the Garmin while leaving the board powered | Board stops changing the number and shows STALE or WAITING. It must not invent further counts. |
| Restart Garmin and re-enter the profile's ride screens | Connection should resume; the Garmin counter may restart because the field was reloaded. |

Stopping or saving the activity, leaving its profile, or sleeping the Garmin is different from switching data pages. Continuous transmission occurs while the field remains loaded and Garmin continues calling it. Tom verified updates with the map visible for 120 seconds and WAITING/LIVE recovery after Garmin power-off/restart; see BUILD-NOTES.md for the exact observations and remaining checks.

You can discard the test activity afterwards. Once the map-page test works, we can replace the counter with Garmin's actual ride timer, pause state and time of day.

## Understanding the Waveshare screen

| Display | Meaning |
| --- | --- |
| WAITING | No BLE connection. Any number shown is an old value. |
| WAIT DATA | A central has connected, but no valid packet has arrived on this connection. |
| LIVE | A valid packet arrived within the last five seconds. |
| STALE | Connected, but no valid packet for at least five seconds. |
| Received … ago | Age of the last accepted packet. It increases when transmissions stop. |

The `View flag: set/clear` label comes from the last packet's onShow/onHide flag
(older installed builds say `Garmin page: visible/hidden`). It does not reliably
track selected data pages on Edge 530: the flag stayed set during the map test.
Clear is also its initial value before any onShow callback. When data is not live,
the label is replaced with a waiting/last-value message.

## If something does not work

- **Garmin stays at Starting:** install the current diagnostic build, identified
  by `Stem BLE diag2` on a full-height field. After 30 seconds report its exact
  status, P/S/A/Match/ACK values and your Edge firmware version. P is the profile
  result (0 = success); S is the scan callback state; A counts advertisement
  results, and Match counts results advertising our service (not unique devices).
  Scanning now starts independently of the registration callback; pairing still
  requires confirmed registration success. `No profile reply` means no registration
  callback arrived within 10 compute ticks; `Profile UUID?` means a callback
  arrived with an unexpected UUID; `Profile <number>` means registration failed.
  These registration diagnostics concern the Garmin's local BLE setup, not the
  receiver's packet decoder. Diag2 can scan while waiting for registration and
  still accepts a late success reply. `P:0 S:off` with `Sending` and increasing
  ACK is expected: registration succeeded and scanning stopped after selecting
  the board. Confirm LIVE on the Waveshare to verify packet decoding as well.
- **Board says WAITING; Garmin says Searching:** confirm the BLE sketch is uploaded, the field is open in your active profile, and both devices are nearby. A phone BLE scanner connected to the board can occupy the connection; disconnect it. Restart the board, then re-enter the Garmin profile.
- **Board says WAITING; Garmin says Sending with increasing ACK:** check whether
  ACK stops when this board is powered off. Upload the current receiver sketch
  (`STEM RX d2` heading), then capture startup and 30 seconds of Serial Monitor
  output at 115200. `BLE t=...` lines expose connect/disconnect and write counts,
  accepted/rejected packets, malformed packets, writes while marked disconnected,
  and last packet length. Raw GATTS connection/disconnection counts are included.
  RX d2 accepts valid writes even when onConnect was missed; malformed writes
  never refresh freshness. ACK alone does not confirm decoding on this board.
- **Board says WAIT DATA or STALE:** open Arduino Serial Monitor at **115200**. Accepted packets print `RX counter=… page=…`. Send me those lines and the Garmin status/ACK value.
- **Garmin shows IQ! or crashes:** connect it to the Mac and look in `GARMIN/APPS/LOGS` for `CIQ_LOG.YML` or `CIQ_LOG.TXT`. Share the relevant error entry, plus your Garmin firmware and SDK versions.
- **Works on the data page, stops on the map:** leave the board powered and note whether it shows STALE or WAITING. Confirm the field still exists in the same active profile. This is precisely the hardware behaviour this prototype is designed to test.
- **PRG copied but field absent:** check it is in `GARMIN/APPS`, was built for Edge 530, and that the device was ejected/unplugged. Restart Garmin and check the Connect IQ data-field category again.

Please report the visible-page result, map-page result, and power-cycle reconnection result. Those three observations will tell us whether the link is ready for real ride data.
