using Toybox.Activity;
using Toybox.Lang;
using Toybox.Time;
using Toybox.Time.Gregorian;

// One complete snapshot per compute; no locally accumulated activity timer.
class RideData {
    var durationSeconds = null;
    var timerState = null;
    var packet as Lang.ByteArray or Null = null;

    function initialize() {}
    function update(info) {
        var local = Gregorian.info(Time.now(), Time.FORMAT_SHORT);
        var encoded = encode(local, info.timerTime, info.timerState);
        packet = encoded;
        durationSeconds = (encoded[4] & 2) != 0 ? (info.timerTime / 1000).toNumber() : null;
        timerState = (encoded[4] & 4) != 0 ? info.timerState : null;
    }
    // Kept independent of BLE so null/state/time conversion can be unit-tested.
    function encode(local, timerMs, state) as Lang.ByteArray {
        var data = [0x42,0x53,3,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]b;
        if (local != null && local.year >= 2000 && local.year <= 2099) {
            data[4] |= 1;
            data[6] = local.year & 0xff; data[7] = (local.year >> 8) & 0xff;
            data[8] = local.month; data[9] = local.day;
            data[10] = local.hour; data[11] = local.min; data[12] = local.sec;
        }
        if (timerMs != null && timerMs >= 0) {
            data[4] |= 2;
            var seconds = (timerMs / 1000).toNumber();
            for (var i = 0; i < 4; i++) { data[14+i] = (seconds >> (i*8)) & 0xff; }
            var millis = (timerMs % 1000).toNumber();
            data[18] = millis & 0xff; data[19] = (millis >> 8) & 0xff;
        }
        if (state == Activity.TIMER_STATE_OFF || state == Activity.TIMER_STATE_STOPPED ||
            state == Activity.TIMER_STATE_PAUSED || state == Activity.TIMER_STATE_ON) {
            data[4] |= 4;
            // Explicit mapping keeps the wire protocol independent of API values.
            if (state == Activity.TIMER_STATE_STOPPED) { data[5] = 1; }
            else if (state == Activity.TIMER_STATE_PAUSED) { data[5] = 2; }
            else if (state == Activity.TIMER_STATE_ON) { data[5] = 3; }
        }
        return data;
    }
    function durationText() {
        if (durationSeconds == null) { return "--:--:--"; }
        var s = durationSeconds;
        return (s / 3600).format("%02d") + ":" + ((s / 60) % 60).format("%02d") + ":" + (s % 60).format("%02d");
    }
    function stateText() {
        if (timerState == Activity.TIMER_STATE_OFF) { return "NO ACTIVITY"; }
        if (timerState == Activity.TIMER_STATE_STOPPED) { return "PAUSED"; }
        if (timerState == Activity.TIMER_STATE_PAUSED) { return "AUTO PAUSED"; }
        if (timerState == Activity.TIMER_STATE_ON) { return "RUNNING"; }
        return "STATUS UNKNOWN";
    }
}
