using Toybox.Application;
using Toybox.WatchUi;
using Toybox.Graphics;

class StemApp extends Application.AppBase {
    var link;
    function initialize() { AppBase.initialize(); }
    function onStart(state) {
        link = new StemLink();
        link.start();
    }
    function getInitialView() { return [new StemField(link)]; }
    function onStop(state) { if (link != null) { link.stop(); } }
}

class StemField extends WatchUi.DataField {
    var link;
    function initialize(connection) {
        DataField.initialize();
        link = connection;
    }
    // compute continues when this loaded field's page is hidden.
    function compute(info) { link.tick(info); }
    function onUpdate(dc) {
        dc.setColor(Graphics.COLOR_BLACK, Graphics.COLOR_WHITE);
        dc.clear();
        var x = dc.getWidth()/2;
        var h = dc.getHeight();
        if (h < 100) {
            dc.drawText(x, 0, Graphics.FONT_XTINY, link.status, Graphics.TEXT_JUSTIFY_CENTER);
            dc.drawText(x, h/2, Graphics.FONT_SMALL, link.ride.durationText(), Graphics.TEXT_JUSTIFY_CENTER);
        } else {
            dc.drawText(x, h/10, Graphics.FONT_SMALL, "StemScreen ride v2", Graphics.TEXT_JUSTIFY_CENTER);
            dc.drawText(x, h/4, Graphics.FONT_MEDIUM, link.ride.durationText(), Graphics.TEXT_JUSTIFY_CENTER);
            dc.drawText(x, h/2, Graphics.FONT_SMALL, link.status, Graphics.TEXT_JUSTIFY_CENTER);
            dc.drawText(x, h*7/10, Graphics.FONT_XTINY,
                link.ride.stateText(),
                Graphics.TEXT_JUSTIFY_CENTER);
            dc.drawText(x, h*17/20, Graphics.FONT_XTINY,
                "TX " + link.counter.toString() + " ACK " + link.acked.toString(), Graphics.TEXT_JUSTIFY_CENTER);
        }
    }
}
