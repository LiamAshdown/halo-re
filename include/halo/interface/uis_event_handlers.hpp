#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::ui {

/**
 * Widget event handlers reachable only through ui_event_function_table. Each handler takes the widget
 * instance, the event record and an out-flag and returns whether the event was consumed.
 * Stateless behaviour class: the functions are static members, the state they act on lives in the engine globals.
 */
struct UiEventHandlers {
    static uint8_t event_49cdd0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49cfa0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d0d0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d100(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d120(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d140(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d160(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d1a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d1b0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d440(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d450(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d480(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d520(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d540(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d5b0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d5f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d7a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49d8b0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49dab0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49dbc0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49dca0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e170(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e210(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e220(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e2c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e300(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e5d0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49e7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49ea50(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49edc0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f030(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f300(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f470(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f560(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f610(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f680(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49f8f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49fad0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_49fd30(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a02a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0590(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0700(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a07e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0860(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0a80(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0ae0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0bc0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0c00(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0c60(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0d60(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0e90(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a0fb0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a10f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1180(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a11e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1280(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a12c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a12f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1310(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1480(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1570(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a15e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1650(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a16a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a16c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a16d0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a16e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1700(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1740(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1790(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1900(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1b00(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1b60(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1bf0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1c80(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1ca0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1cd0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1d00(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1d30(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1d60(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1d90(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a1dc0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2190(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a21c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2490(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a24c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2950(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2a00(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2c50(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2c80(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a2f10(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3000(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3050(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3150(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a33a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3510(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3540(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3790(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3870(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a39c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a39e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3a70(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a3d40(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4110(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4190(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a41a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4270(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a44f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4570(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4580(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a45d0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a45f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a47b0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4870(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4a4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b4980(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b4af0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b4c40(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b52f0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b5350(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b54a0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4b54c0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4bb290(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4bb300(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4bb360(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4bb7e0(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4bb970(widget_instance *widget, int16_t *event, uint8_t *out_handled);
    static uint8_t event_4bba80(widget_instance *widget, int16_t *event, uint8_t *out_handled);
};

}
