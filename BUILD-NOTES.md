# Validation

## Activity colours and paused ring — 22 September 2026

The current ride UI now uses blue for fresh data with no activity or unknown
activity state, steady green for running, and a green/black circumference ring
alternating every 500 ms for manual pause and Auto Pause. Connection text and
the active page dot retain the steady status tint. Cyan (awaiting data), orange
(stale) and red (disconnected) override activity state. Packet arrivals do not
restart flashing; the main loop redraws when the ring changes phase.

Host protocol/receiver/UI tests passed with C++11 and warnings treated as errors,
including both pause types on both pages, 500 ms boundaries, packet arrival during
the black phase, resume/no activity/unknown state, and link-warning precedence.
Receiver compilation passed with ESP32 core 3.3.11 and the documented board
settings: 635,831 bytes program storage and 29,708 bytes globals. Build output:
`/tmp/stem-activity-colours-build`. `git diff --check` passed.

Garmin source and packet format were unchanged; no Garmin build or simulator run
was performed. This receiver update has not been uploaded or physically tested.
Next: upload `StemScreen/StemScreen.ino` using the existing Arduino settings;
check blue before starting/after ending an activity, steady green while running,
500 ms green/500 ms black while manually paused and Auto Paused, and steady green
on resume. Check both clock and ride-time pages, steady text/dot during flashing,
and that stale/disconnection warnings still replace the activity colour.

## Map updates and Garmin restart verified — 17 September 2026

With Garmin diag2 and receiver RX diag2, Tom reports:

- Waveshare stayed LIVE and continued counting while the Garmin showed the map,
  for at least 120 seconds. Its page label nevertheless remained visible.
- Powering off Garmin changed Waveshare to WAITING and stopped the counter.
  Exact transition latency was not measured.
- Powering Garmin on immediately restored connection and LIVE, with the counter
  restarting from zero and the view flag initially hidden.
- Entering the activity profile set the flag to visible and reset the counter
  again. App/field recreation is consistent with this, but no lifecycle trace
  was captured to establish the exact instance sequence.

The main map-page transmission requirement and receiver recovery after Garmin
restart are now physically verified. The visible/hidden flag is not a dependable
page detector on this Edge firmware. Source display wording now says
`View flag: set/clear`; wire bytes and transport behavior are unchanged. No
additional upload is required to retain the working link.

The wording-only receiver update compiled successfully with core 3.3.11
(633,911 bytes program storage, 29,676 bytes globals); `git diff --check` passed.
This updated label has not been physically uploaded/tested in this session.

Remaining checks include RX diag2 LIVE recovery after cycling board power,
manual/Auto Pause behavior, and connected-but-silent STALE behavior on hardware.
The five-second freshness boundary is host-tested, but powering Garmin off
tested disconnection, not a silent connection. No ride-timer feature is implemented.

## Confirmed end-to-end LIVE — 17 September 2026

Tom reports RX diag2 displaying `STEM RX d2`, `LIVE`, counter `889`,
`Garmin page: Visible`, and `Received 0s ago`, with Garmin diag2 running on
Edge 530 software 9.75. This is the first confirmed receiver acceptance and
fresh display of a Garmin packet. RX diag2 was compiled for ESP32 core 3.3.11.

Two changes resolved the observed blockers: Garmin begins scanning after
submitting its profile without waiting for the registration callback; the
receiver accepts valid writes as connection evidence when onConnect is missed.
The underlying platform callback behavior remains unexplained. No further
firmware change is needed before the remaining bench tests.

Still to verify: counter progression and LIVE for 60 seconds with the Garmin
map visible (`Garmin page: hidden`), loss of LIVE within five seconds of the
last valid packet when Garmin is powered off, and reconnection with accepted
packets after power cycling either device. Garmin-side reconnect/ACK recovery
after board power cycling was already observed before RX diag2; receiver-side
LIVE recovery with the new firmware has not yet been reported.

## Receiver callback recovery — 17 September 2026

Tom's RX diag1 log identifies ESP32 core 3.3.11 and shows 29 valid 16-byte writes,
zero connect/disconnect callbacks, zero accepted packets, and 29 rejections
solely because the receiver's connected flag stayed false. Packet encoding and
arrival are verified; the exact cause of the missing callbacks is not established.

RX diag2 accepts a valid GATT write as connection evidence. It keeps validation
and the five-second freshness threshold, and adds the bundled Bluedroid library's
custom GATTS hook for connect/disconnect handling without replacing the library's
own ESP-IDF callback. State transitions are idempotent across duplicate/late
connection notifications. If no disconnect event arrives, silence still gives
STALE. Missing disconnect events and advertising recovery need physical testing.

Host tests passed with warnings treated as errors: valid writes without a connect
callback, malformed writes before/after connection, stale boundary, duplicate
events, disconnect/reconnect, pending advertising cancellation and millis rollover.
The pure state logic is in StemScreen/ReceiverState.h. Garmin diag2 is unchanged.
Receiver compilation passed with ESP32 core 3.3.11 and the documented board
settings: 633,895 bytes program storage and 29,676 bytes global variables.
Build artifacts are in `/tmp/stem-rx-diag2-build`; no upload has been performed
by Codex. `git diff --check` passed.
Next physical test: upload RX diag2, confirm LIVE and accepted/RX counters, then
stop Garmin transmission and verify it cannot remain LIVE after five seconds.

## Latest hardware result — 17 September 2026

Tom reports the following on Edge 530 software 9.75 with diag2 after 30 ticks:
`Sending`, `P:0 S:off A:1`, `Match 1 ACK 30`.
This verifies successful profile registration, discovery of the matching service,
connection and acknowledged GATT writes from the Garmin. Scanning is intentionally
off after selecting the peripheral. Starting scanning after submitting the profile,
without waiting for its callback, resolved the observed startup stall in this test.
The underlying Garmin callback scheduling behavior has not been established.

Tom subsequently reports the Waveshare still shows WAITING. End-to-end delivery
is therefore not verified. The Garmin ACK result does not establish that this
board's callbacks accepted the packets. Map-page testing is deferred until the
receiver shows LIVE.

Receiver diag1 adds a `STEM RX d1` heading, core version and BLE address at startup,
and main-loop serial diagnostics for connect/disconnect events, write count,
accepted/rejected packets, malformed writes, writes while marked disconnected,
and last write length. Counts are snapshotted under the existing short lock;
logging remains outside callbacks. A status line prints every five seconds and
on connection/rejection changes. No packet acceptance or connection logic changed.
Tom confirmed the board power-off test: Garmin changes to Reconnecting and ACK
stops when the Waveshare is unplugged; after restoring power, Sending and
incrementing ACK resume. This ties the observed link to this board and verifies
Garmin-side reconnection after board power cycling. Receiver acceptance remains
unverified because its screen still reports WAITING.

Receiver diag1 compiled successfully with installed ESP32 core 3.3.11 and the
documented ESP32S3/16MB/QSPI PSRAM settings: 633,863 bytes program storage and
29,668 bytes global variables (runtime allocations excluded). Build output is
in `/tmp/stem-rx-diag1-build`. It has not yet been uploaded or physically tested.
Next: upload receiver diag1 and capture startup plus 30 seconds of serial output
at 115200 with Garmin diag2 running. The physical cause remains unconfirmed.

## Hardware report — 17 September 2026

Tom reports successfully building and running both the standalone backup and
current StemScreen sketch on the Waveshare. He also successfully compiled and
installed the Garmin app on his Edge 530 and added the Connect IQ field to a data
screen. After starting an activity, the devices did not connect successfully.
BLE discovery, packet delivery, hidden-page updates and reconnection are not yet
verified. The earlier Edge build/sideload blocker is resolved.

Repository review found two commits: the standalone prototype (6c73c8c), followed
by the BLE prototype and preserved standalone backup (df2548c). Corrected stale
sketch paths in README and the protocol test include after the folder rename.
The installed local toolchains are ESP32 core 3.3.11 and Connect IQ SDK 9.2.0;
the versions used for the reported device builds have not yet been confirmed.
After the include correction, host protocol/freshness tests passed with
`c++ -std=c++11 -Wall -Wextra -pedantic`; `git diff --check` also passed.
No firmware rebuild, Garmin rebuild, simulator run or physical test was performed
during this initial review. Awaiting the exact statuses shown by both devices and
the receiver's startup serial output to locate the failing connection stage.

## Startup diagnostic — 17 September 2026

Tom observed an increasing Garmin counter (60), status `Starting`, ACK 0, and
Waveshare `WAITING`. After reset the board printed its ROM boot messages followed
by `BikeStem ready: waiting for the Garmin data field.` This locates the observed
stall before the Garmin's scan path; it does not establish successful advertising.

Added `Stem BLE diag1` identification, explicit registration status, callback
logging, visible UUID-mismatch reporting, and a `No profile reply` message after
10 compute ticks. A late successful callback can still advance to scanning.
No automatic re-registration or bypass of registration success was added.
The root cause remains unconfirmed; install this diagnostic build and report the
status after 15 seconds, plus Edge firmware version.

Edge 530-target compilation succeeded with local SDK 9.2.0 and the configured
developer key. The compiler warns that the existing 32x32 icon is scaled to
35x35. Output: `Garmin/builds/diagnostic/Garmin.prg` (ignored build artifact).
No simulator or physical test of this diagnostic version has been performed.

## Independent scan diagnostic — 17 September 2026

On Edge 530 software 9.75, Tom installed diag1 and observed `No profile reply`
and ACK 0 after 15 seconds. The device reports no software update available.
This confirms the registration-wait branch is reached, without proving a
firmware defect or determining why the callback is absent.

Diag2 starts scanning after registerProfile returns instead of waiting for its
callback. Garmin's bundled NordicThingy52 example likewise registers profiles
in onStart and initiates scanning separately without implementing
onProfileRegister. This is a startup experiment: StemScreen still requires a
successful matching registration callback before pairing or writing.

The full-height field displays P (profile result, 0 means success), S (scan
callback state), A (advertisement results), Match (matching service results),
and ACK. Advertisement counts are observations, not unique devices, and cap at
9999. A missing profile reply still gets a timeout message after 10 compute ticks.
No repeated profile registration is performed. Late success can enable pairing.

Diag2 compiled successfully for Edge 530 with SDK 9.2.0; only the existing icon
scaling warning remains. Simulator execution is untested. Subsequent physical
results are recorded at the top of this document.

## Historical validation — 9 September 2026

## Waveshare

Compiled successfully for ESP32S3 Dev Module using Espressif Arduino core 3.3.0, Adafruit GC9A01A 1.1.1, Adafruit GFX 1.12.6 and Adafruit BusIO 1.17.4.

Compiler report: 681,839 bytes of program storage out of 3,145,728; 34,440 bytes of global variables out of 327,680. These figures exclude runtime allocations by BLE and the display strip buffer; they are not measurements of free RAM on a running board.

Equivalent Arduino CLI command from this package's root, with the libraries/core already installed:

```sh
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=enabled,FlashMode=qio,CDCOnBoot=default,USBMode=hwcdc,UploadMode=default,UploadSpeed=115200' StemScreen
```

## Garmin

Both source files and resources passed a general, device-independent compilation with Connect IQ SDK 8.1.0 and Java 17. This check used a temporary copy with an empty product list; its only final warning was that no supported devices were defined. The delivered manifest retains the correct `edge530` target.

**This is not an Edge 530 build or simulator run.** The official device-definition endpoint returned HTTP 401 because a Garmin login is required. Device-specific API availability, resource limits and on-device behaviour remain to be checked by the final build and hardware tests. No generic PRG is included because it is not an Edge 530 deliverable.

Use the README's Build for Device instructions. If using the CLI instead, with Garmin's SDK bin folder on PATH and Edge 530 definitions installed:

```sh
monkeyc -f Garmin/monkey.jungle -d edge530 -y /absolute/path/to/your/developer_key.der -o StemBLE.prg -w
```

## Packet tests

The host C++ test passed: fixed wire bytes/endian decoding, short/long/null input rejection, invalid headers/flags/reserved-byte rejection, five-second freshness boundary, disconnected/new-connection state, and millisecond counter wrap.

Run from this package's root:

```sh
c++ -std=c++11 -Wall -Wextra -pedantic tests/protocol_test.cpp -o /tmp/stem-protocol-test
/tmp/stem-protocol-test
```

No physical Waveshare or Edge was available here. BLE discovery, timing, screen appearance, hidden-page updates, and reconnection require the bench tests in README.md. The included standalone backup was preserved from the previous prototype; it was not modified for BLE.
