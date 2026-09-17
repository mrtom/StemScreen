# StemScreen — coding-agent handoff

## Purpose and working relationship

StemScreen is Tom's hobby project: a small bike-stem-mounted display showing time of day and ride time, one screen at a time, rotating approximately every 15–30 seconds. The primary ride-data source is a Garmin Edge 530.

Product strategy and tactical planning live in Tom's ChatGPT project. Implementation, debugging, tests, build tooling and repository maintenance happen in Codex sessions against this repository. Implement the task Tom brings into the session; make ordinary implementation decisions autonomously. Raise material product trade-offs or changes of direction for discussion rather than silently redefining the project. Do not treat the roadmap below as authorization to implement every stage immediately.

Tom is an experienced software engineer but new to microcontroller and Connect IQ development. Be technically precise and give explicit hardware setup, build and upload instructions. State what changed, how it was checked, and what Tom must verify on the physical devices.

## Start each session

1. Read this file and any applicable nested agent instructions.
2. Inspect `git status`, the current branch and recent commit history. Preserve uncommitted work and existing history. The live repository and Tom's latest instructions take precedence over this historical handoff.
3. Read `README.md`, `PROTOCOL.md` and `BUILD-NOTES.md`, then inspect the relevant source files before editing.
4. Determine which stage is actually working from recorded evidence. Do not infer successful hardware tests from a successful compile or from the existence of code.
5. Continue the requested task. Do not recreate the repository from the old ZIP, reset the checkout, or assume an old chat workspace/toolchain still exists.

## Naming and baseline layout

The project is called **StemScreen**. The earlier distribution was named `BikeStemBLE.zip`; that name is historical.

The ZIP's root contained:

| Path | Purpose |
| --- | --- |
| `WaveshareBLE/WaveshareBLE.ino` | ESP32 BLE counter receiver and display |
| `WaveshareBLE/BleProtocol.h` | Wire-packet decoder and freshness logic |
| `Garmin/source/StemApp.mc` | Connect IQ application and data field |
| `Garmin/source/StemLink.mc` | BLE discovery, connection and writes |
| `Garmin/manifest.xml` | Edge 530 target and BLE permission |
| `Garmin/monkey.jungle` | Garmin build configuration |
| `Garmin/resources/` | App name and icon |
| `tests/protocol_test.cpp` | Host-side packet/freshness tests |
| `StandaloneBackup/WaveshareHello/` | Previous standalone clock/timer sketch |
| `README.md`, `PROTOCOL.md`, `BUILD-NOTES.md` | Setup, protocol and validation records |

Tom subsequently requested `StemScreen.ino`. Arduino's main sketch and its containing folder must share the name: `StemScreen/StemScreen.ino`. Inspect whether this rename has already happened. If completing it as part of the requested work, move the header with the sketch and update paths in instructions/build scripts. Do not overwrite a separate standalone `StemScreen` sketch with the BLE prototype without checking what it contains.

Legacy UI/BLE names include `Stem BLE`, `STEM BLE` and `BikeStem`. Renaming the project does not require changing service UUIDs, packet format or the Garmin application ID. Preserve those identifiers unless deliberately migrating them.

## Hardware and development baseline

- Board: **Waveshare ESP32-S3-LCD-1.28, NON-TOUCH**. Purchased model: Amazon ASIN `B0CSSSYFYY`. Do not substitute touch-board pin mappings.
- Display: round GC9A01A, 240×240 pixels.
- ESP32-S3: 16 MB flash, 2 MB QSPI PSRAM; CH343 USB serial bridge.
- Display pins: SCK **10**, MOSI **11**, CS **9**, DC **8**, RESET **12**, backlight **40**.
- BOOT button: GPIO **0**, active low. Avoid declaring `BOOT_PIN`, which can collide with the ESP32 core; use a project-specific identifier.
- Host: Mac, Apple Silicon. Arduino IDE has already uploaded working display sketches.

Known build baseline:

| Setting/dependency | Value |
| --- | --- |
| Arduino board | ESP32S3 Dev Module |
| Espressif Arduino core | 3.3.0 |
| Flash size / partition | 16MB / `16M Flash (3MB APP/9.9MB FATFS)` |
| PSRAM | QSPI PSRAM |
| Flash mode | QIO, 80 MHz |
| USB CDC On Boot | Disabled |
| USB mode / upload mode | Hardware CDC and JTAG / UART0–Hardware CDC |
| Upload speed | 115200 |
| Adafruit GC9A01A | 1.1.1 |
| Adafruit GFX | 1.12.6 |
| Adafruit BusIO | 1.17.4 |

BLE uses the library bundled with Espressif's core. Do not add a competing Arduino BLE library or change frameworks without a concrete need. Keep working dependency versions unless an upgrade addresses the task.

## Architecture and implemented behaviour

The **Garmin is the BLE central/GATT client**. The **Waveshare is the peripheral/GATT server**. The Connect IQ data field discovers the custom advertised service and writes to its characteristic. Pairing is handled by the field, not Garmin's ordinary Add Sensor screen. No phone or internet service is required for this link.

The current BLE prototype sends a diagnostic counter approximately once per second from `DataField.compute()`. It is **not the ride timer**. The counter increments while disconnected and is not tied to activity timer pause state. It can reset when the app reloads.

Transmission must remain independent of screen rendering: use `compute()` for updates, `onUpdate()` for drawing, and `onShow()`/`onHide()` for visibility diagnostics. Do not disconnect in `onHide()`. The intended result is continued updates while the Garmin displays its map; this still requires physical verification. Leaving the activity/profile or sleeping the Garmin is a separate lifecycle case.

The sender keeps at most one write in flight, uses writes with response, and sends the latest state rather than queueing old values. The baseline includes timeout and reconnection attempts. Receiver BLE callbacks copy state under a short lock; display and serial work happen in the main loop. Preserve this separation.

The receiver displays WAITING, WAIT DATA, LIVE or STALE, the last received counter, packet age, and the last reported Garmin page visibility. Five seconds without valid data is stale. Invalid packets must not refresh data age. Never silently replace missing Garmin data with a locally advancing counter that appears live.

## Protocol compatibility

`PROTOCOL.md` is the detailed reference. Baseline identifiers:

- Service: `93e1b4a0-5c21-4e62-a738-9d608c17f201`
- Writable RX characteristic: `93e1b4a1-5c21-4e62-a738-9d608c17f201`
- Advertised name: `BikeStem`; discovery matches the service UUID.
- Garmin application ID: `9080770dece64c76b104c84a44d0bad1`
- Target: `edge530`; minimum API 3.1.0; permission `BluetoothLowEnergy`.

Counter packet: exactly 16 bytes; multibyte integers are unsigned little-endian.

| Offset | Meaning |
| --- | --- |
| 0–1 | Magic `42 53` hex |
| 2 | Version 1 |
| 3 | Kind 1: diagnostic counter |
| 4–7 | Counter |
| 8–11 | Garmin Unix timestamp, seconds; currently diagnostic |
| 12 | Page visible: 0 or 1 |
| 13–15 | Reserved zero bytes |

The receiver validates packet length and header/flag/reserved values. A BLE write acknowledgement proves transport completion, not application-level decoding. Treat receiver output as the evidence that a packet was accepted. Coordinate sender, receiver, tests and documentation for protocol changes; keep writes within the supported payload size. The baseline uses no authentication/bonding and connects to the first matching service; it is a bench prototype.

## Known progress and unresolved checks

Current bench evidence, 17 September 2026 (supersedes the historical list below):

- Edge 530 software 9.75 runs Garmin diag2; receiver RX diag2 runs with ESP32 core
  3.3.11. End-to-end LIVE counter reception is verified, including at least 120
  seconds with the Garmin map visible. Garmin power-off produces WAITING and a
  stopped counter; Garmin restart restores LIVE. Entering the activity profile
  can reset the diagnostic counter again.
- Start Garmin scanning after submitting the profile; waiting for registration's
  callback before scanning stalled. Keep successful registration required for pairing.
- Receiver valid writes arrived without onConnect. ReceiverState.h now accepts
  a valid write as connection evidence; invalid data never refreshes freshness.
  A custom Bluedroid GATTS hook supplements connection/disconnection callbacks.
  The reason for missing callbacks has not been established; do not claim a
  confirmed platform defect.
- The onShow/onHide flag stayed set while the map was visible. It is a view
  lifecycle diagnostic, not reliable selected-page visibility. Zero is also its
  initial state. Do not use it to gate transmission or infer activity state.
- Latest detailed observations and remaining physical checks are in BUILD-NOTES.md.

Historical handoff, 17 September 2026 — verify against newer commits and user reports:

- Tom successfully uploaded the original display demo and changed “Hello Tom” to “Ride Time”. A standalone clock/timer with rotating screens was then supplied.
- The BLE package was generated and the Waveshare firmware compiled successfully with the baseline above.
- Host packet/freshness tests passed, including malformed input and millisecond rollover cases.
- Garmin source/resources passed a **device-independent** compilation using Connect IQ SDK 8.1.0 and Java 17. A temporary copy had an empty product list for that check; the distributed manifest retained `edge530`.
- The **Edge 530-specific build, simulator run and physical BLE link were not verified** in that environment. Device definitions required Garmin sign-in. No deployable Garmin PRG was supplied.
- The last reported setup blocker was that the Edge 530 did not appear in Finder. No resolution or successful map-page test was reported before this handoff.
- Tom now has a Git repository with history. That repository has not been inspected by the author of this handoff; do not assume it is identical to the ZIP.

## Build and verification

Prefer repository scripts if newer commits introduce them. Otherwise, from the root:

```sh
c++ -std=c++11 -Wall -Wextra -pedantic tests/protocol_test.cpp -o /tmp/stem-protocol-test
/tmp/stem-protocol-test
```

Arduino CLI, after installing the listed board core and libraries (replace `WaveshareBLE` with `StemScreen` if renamed):

```sh
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=enabled,FlashMode=qio,CDCOnBoot=default,USBMode=hwcdc,UploadMode=default,UploadSpeed=115200' WaveshareBLE
```

Garmin requires its SDK and Edge 530 device definitions installed through SDK Manager, plus a developer signing key. Use VS Code's Garmin Monkey C extension and **Build for Device → Edge 530**, or, with SDK tools on PATH:

```sh
monkeyc -f Garmin/monkey.jungle -d edge530 -y /absolute/path/to/developer_key.der -o StemBLE.prg -w
```

Keep signing keys outside version control. Do not commit downloaded SDKs, build outputs, credentials or machine-specific absolute paths. Copy the device-specific `.prg` to the Garmin's `GARMIN/APPS`, eject it, then add the field once to the active cycling profile. Full beginner instructions are in `README.md`.

Report host tests, firmware compilation, device-target compilation, simulator execution and physical tests separately. If a toolchain is missing, report the exact gap and command Tom should run; do not label an unrun check as passed. Add focused tests for meaningful protocol, timing and reconnection changes rather than superficial coverage.

## Hardware troubleshooting context

`system_profiler SPUSBDataType` previously returned nothing on Tom's Mac. `ioreg -p IOUSB -w0` did work. The Waveshare appeared as `USB Single Serial` and `/dev/cu.usbmodem5ABA0067551`; rediscover the port rather than hardcoding it.

For the Edge Finder issue, compare `ioreg -p IOUSB -w0` before/after connection and inspect `diskutil list external`. Check the data cable, accessory permission with the Mac unlocked, USB adapter/port, and Finder external-disk visibility. Ask what the Garmin screen displays. A charging indicator alone is not evidence of a data connection. Do not erase or initialise its storage as a troubleshooting shortcut.

## Next milestones, when requested

1. Finish the actual Edge 530 build/sideload and prove received-counter updates on the Waveshare.
2. Confirm updates for at least 60 seconds while the Garmin map is visible, then verify reconnection after board power cycling and Garmin restart. Record observations and versions.
3. Once the link is proven, send actual Garmin time, activity timer and ride state; restore the clock/timer rotating UI. Clarify active timer versus elapsed wall time if the requested behaviour is ambiguous. Test manual pause, Auto Pause, resume, activity end and stale data explicitly.

Keep rendering, transport and ride/time state sufficiently separate to support later hardware changes. Alternative displays, custom enclosures and Wahoo integration are future topics, not current implementation requirements. BLE alone does not establish Wahoo compatibility. Avoid speculative abstraction or hardware redesign during the first prototype.

## Keeping the handoff useful

Keep setup and protocol documents aligned with changes. Record newly verified hardware results and remaining blockers in `BUILD-NOTES.md` (or a newer repository status document). Update this file when naming, architecture or durable working conventions change. End coding sessions with a concise account of changes, validation, and the next concrete hardware action, so Tom can bring the result back to the planning conversation.
