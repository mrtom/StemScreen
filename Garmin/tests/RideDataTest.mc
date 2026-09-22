using Toybox.Test;
using Toybox.Activity;

class SampleLocalTime {
    var year=2026; var month=9; var day=17;
    var hour=14; var min=35; var sec=59;
    function initialize() {}
}
(:test)
function rideWireVector(logger) {
    var ride = new RideData();
    var data = ride.encode(new SampleLocalTime(), 3661999, Activity.TIMER_STATE_ON);
    var expected = [0x42,0x53,3,2,7,3,0xea,7,9,17,14,35,59,0,0x4d,0x0e,0,0,0xe7,3]b;
    Test.assertEqual(data.size(), 20);
    for (var i=0;i<20;i++) { Test.assertEqual(data[i], expected[i]); }
    return true;
}
(:test)
function rideMissingData(logger) {
    var ride = new RideData();
    var data = ride.encode(null, null, null);
    for (var i=4;i<20;i++) { Test.assertEqual(data[i], 0); }
    data = ride.encode(null, -1, 99);
    Test.assertEqual(data[4], 0);
    return true;
}
(:test)
function rideFractionalBoundaries(logger) {
    var ride = new RideData();
    var times = [0, 1, 999, 1000, 1001];
    var seconds = [0, 0, 0, 1, 1];
    var fractions = [0, 1, 999, 0, 1];
    for (var i=0;i<times.size();i++) {
        var data = ride.encode(null, times[i], Activity.TIMER_STATE_ON);
        Test.assertEqual(data[2], 3);
        Test.assertEqual(data[14], seconds[i]);
        Test.assertEqual(data[18] | (data[19] << 8), fractions[i]);
    }
    return true;
}
(:test)
function ridePauseResumeAndReload(logger) {
    var states = [Activity.TIMER_STATE_OFF, Activity.TIMER_STATE_STOPPED,
                  Activity.TIMER_STATE_PAUSED, Activity.TIMER_STATE_ON];
    var ride = new RideData();
    for (var i=0;i<4;i++) {
        var data = ride.encode(null, 3661000, states[i]);
        Test.assertEqual(data[4], 6);
        Test.assertEqual(data[5], i);
        Test.assertEqual(data[14], 0x4d);
        Test.assertEqual(data[15], 0x0e);
    }
    // Recreating the sender still serializes the supplied Garmin timer.
    var reloaded = new RideData();
    var data = reloaded.encode(null, 3661000, Activity.TIMER_STATE_ON);
    Test.assertEqual(data[14], 0x4d);
    Test.assertEqual(data[15], 0x0e);
    data = reloaded.encode(null, 0, Activity.TIMER_STATE_OFF);
    Test.assertEqual(data[14], 0);
    Test.assertEqual(data[5], 0);
    return true;
}
