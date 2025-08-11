#!/usr/bin/env python3
"""
Simple Tkinter GUI to select RGB effect and global brightness on the Pico Game Controller over HID Feature reports.
Requires: hidapi (pip install hidapi).

Debugging:
- Run with --debug to print extra logs to the console.
- Use the "Debug HID" button to list all HID interfaces for this VID/PID.
"""

import sys
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import hid  # from hidapi
except ImportError:
    hid = None
    HIDErrors = (OSError,)
else:
    HIDException = getattr(hid, "HIDException", OSError)
    HIDErrors = (HIDException, OSError)

# Debug flag
DEBUG = "--debug" in sys.argv

# Simple logger


def log(*args):
    if DEBUG:
        try:
            print("[DEBUG]", *args)
        except Exception:
            pass


# Match firmware VID/PID
VID = 0x1CCF
PID = 0x8048

# Report IDs must match firmware
REPORT_ID_CONFIG = 5

# Commands
CMD_SET_EFFECT = 0x01
CMD_SET_BRIGHTNESS = 0x02
CMD_REBOOT_BOOTSEL = 0x03
CMD_SET_ENCODER_PPR = 0x10
CMD_SET_MOUSE_SENS = 0x11
CMD_SET_ENC_DEBOUNCE = 0x12
CMD_SET_WS_PARAMS = 0x13
CMD_GET_EXT_STATUS = 0x20

EFFECTS = [
    (0, "Color Cycle"),
    (1, "Turbocharger"),
    (2, "Trail"),
    (3, "Dual Orbit"),
    (4, "Velocity Comet"),
    (5, "Button Ripples"),
    (6, "Spokes"),
    (7, "Counter Stripes"),
    (8, "Palette Tint Gradient"),
    (9, "Multipoint Snap"),
    (10, "Center Pulse"),
    (11, "Sector Equalizer"),
    (12, "Radar Sweep"),
]


def hid_enumerate_info():
    """Return a human-readable list of HID interfaces for our VID/PID."""
    if hid is None:
        return "hidapi not available"
    lines = []
    try:
        for d in hid.enumerate(VID, PID):
            path = d.get("path")
            if isinstance(path, (bytes, bytearray)):
                try:
                    path = path.decode("utf-8", "ignore")
                except Exception:
                    path = str(path)
            lines.append(
                f"path={path}\n  usage_page=0x{int(d.get('usage_page', 0)):04X} usage=0x{int(d.get('usage', 0)):04X} interface={d.get('interface_number')}\n  product={d.get('product_string')} serial={d.get('serial_number')}"
            )
    except HIDErrors as e:
        lines.append(f"enumerate error: {e}")
    if not lines:
        lines.append("No HID interfaces found for this VID/PID")
    return "\n\n".join(lines)


def _find_config_path():
    """Return HID path for our vendor-specific config collection, if present."""
    try:
        all_devs = list(hid.enumerate(VID, PID))
        log(f"enumerate count={len(all_devs)}")
        for d in all_devs:
            # Prefer the vendor usage page we defined in the firmware (0xFFAF)
            if d.get("usage_page") == 0xFFAF:
                path = d.get("path")
                log("selected vendor path", path)
                return path
    except HIDErrors:
        pass
    return None


def open_device():
    if hid is None:
        raise RuntimeError("hidapi is not available. pip install hidapi")
    dev = hid.device()
    path = _find_config_path()
    if path:
        dev.open_path(path)
    else:
        # Fallback: open the first interface for this VID/PID
        dev.open(VID, PID)
    dev.set_nonblocking(True)
    log("device opened (path?", bool(path), ")")
    return dev


def get_status(dev):
    """Return (effect_id, brightness) or (None, None) if not available."""
    try:
        # Ask for feature data (report id prefix is required by hidapi)
        payload = bytes([REPORT_ID_CONFIG, 0x00] + [0] * 7)
        log("send_feature_report GET:", list(payload))
        dev.send_feature_report(payload)
        data = dev.get_feature_report(REPORT_ID_CONFIG, 9)  # id + 8 bytes
        log("get_feature_report ->", list(data) if data else None)
        if data and len(data) >= 4:
            # data[0] = report id, data[1]=status(0), data[2]=effect, data[3]=brightness
            return data[2], data[3]
    except HIDErrors as e:
        log("get_status error:", e)
    return None, None


def get_ext_status(dev):
    """Return a dict with encoder/LED settings from extended status."""
    try:
        payload = bytes([REPORT_ID_CONFIG, CMD_GET_EXT_STATUS] + [0] * 7)
        log("send_feature_report GET_EXT_STATUS:", list(payload))
        dev.send_feature_report(payload)
        data = dev.get_feature_report(REPORT_ID_CONFIG, 9)
        log("ext status ->", list(data) if data else None)
        if data and len(data) >= 9:
            # [id, status, ppr_lo, ppr_hi, mouse_sens, enc_debounce, sz_lo, sz_hi, zones]
            ppr = data[2] | (data[3] << 8)
            mouse = data[4]
            enc_db = 1 if data[5] else 0
            sz = data[6] | (data[7] << 8)
            zones = data[8]
            return {
                "enc_ppr": ppr,
                "mouse_sens": mouse,
                "enc_debounce": enc_db,
                "ws_led_size": sz,
                "ws_led_zones": zones,
            }
    except HIDErrors as e:
        log("get_ext_status error:", e)
    return {}


def set_effect(dev, effect_id: int):
    payload = [REPORT_ID_CONFIG, CMD_SET_EFFECT,
               int(effect_id) & 0xFF] + [0] * 6
    log("send_feature_report SET_EFFECT:", payload)
    dev.send_feature_report(bytes(payload))


def set_brightness(dev, value: int):
    v = max(0, min(255, int(value)))
    payload = [REPORT_ID_CONFIG, CMD_SET_BRIGHTNESS, v] + [0] * 6
    log("send_feature_report SET_BRIGHTNESS:", payload)
    dev.send_feature_report(bytes(payload))


def reboot_to_bootsel(dev):
    """Ask firmware to reboot into BOOTSEL (UF2) mode."""
    payload = [REPORT_ID_CONFIG, CMD_REBOOT_BOOTSEL] + [0] * 7
    log("send_feature_report REBOOT_BOOTSEL:", payload)
    dev.send_feature_report(bytes(payload))


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Pico IIDX Config Tool")
        self.geometry("520x360")
        self.dev = None

        frm = ttk.Frame(self, padding=10)
        frm.pack(fill=tk.BOTH, expand=True)

        ttk.Label(frm, text="RGB Effect:").grid(row=0, column=0, sticky="w")
        self.effect_var = tk.StringVar()
        self.combo = ttk.Combobox(
            frm,
            textvariable=self.effect_var,
            values=[name for _, name in EFFECTS],
            state="readonly",
        )
        self.combo.grid(row=0, column=1, sticky="ew", padx=(8, 0))
        frm.columnconfigure(1, weight=1)

        # Brightness controls
        ttk.Label(frm, text="Brightness:").grid(
            row=1, column=0, sticky="w", pady=(10, 0))
        self.brightness_val_label = ttk.Label(frm, text="255")
        self.brightness_val_label.grid(
            row=1, column=2, sticky="w", padx=(6, 0))
        self.brightness_scale = ttk.Scale(
            frm,
            from_=0,
            to=255,
            orient=tk.HORIZONTAL,
            command=self.on_brightness_change,
        )
        self.brightness_scale.set(255)
        self.brightness_scale.grid(
            row=1, column=1, sticky="ew", padx=(8, 0), pady=(8, 0))

    # Encoder/Mouse settings frame
        encfrm = ttk.LabelFrame(frm, text="Encoder & Mouse", padding=8)
        encfrm.grid(row=2, column=0, columnspan=3, sticky="ew", pady=(12, 0))
        encfrm.columnconfigure(1, weight=1)
        ttk.Label(encfrm, text="Encoder PPR:").grid(
            row=0, column=0, sticky="w")
        self.ppr_var = tk.IntVar(value=600)
        self.ppr_entry = ttk.Entry(encfrm, textvariable=self.ppr_var, width=8)
        self.ppr_entry.grid(row=0, column=1, sticky="w")
        ttk.Label(encfrm, text="Mouse Sens:").grid(
            row=0, column=2, sticky="e", padx=(12, 0))
        self.mouse_var = tk.IntVar(value=1)
        self.mouse_entry = ttk.Entry(
            encfrm, textvariable=self.mouse_var, width=6)
        self.mouse_entry.grid(row=0, column=3, sticky="w")
        self.db_var = tk.IntVar(value=0)
        self.db_check = ttk.Checkbutton(
            encfrm, text="Encoder Debounce (on reboot)", variable=self.db_var)
        self.db_check.grid(row=1, column=0, columnspan=2,
                           sticky="w", pady=(6, 0))

        # WS2812B stored params

        ledfrm = ttk.LabelFrame(
            frm, text="WS2812B (applied on reboot)", padding=8)
        ledfrm.grid(row=3, column=0, columnspan=3, sticky="ew", pady=(12, 0))
        ttk.Label(ledfrm, text="LED Count:").grid(row=0, column=0, sticky="w")
        self.led_count_var = tk.IntVar(value=10)
        self.led_count_entry = ttk.Entry(
            ledfrm, textvariable=self.led_count_var, width=8)
        self.led_count_entry.grid(row=0, column=1, sticky="w")
        ttk.Label(ledfrm, text="Zones:").grid(
            row=0, column=2, sticky="e", padx=(12, 0))
        self.zones_var = tk.IntVar(value=2)
        self.zones_entry = ttk.Entry(
            ledfrm, textvariable=self.zones_var, width=6)
        self.zones_entry.grid(row=0, column=3, sticky="w")

        # Debug button inside LED frame
        # Status (above bottom buttons)
        self.status_var = tk.StringVar(value="Not connected")
        ttk.Label(frm, textvariable=self.status_var).grid(
            row=4, column=0, columnspan=3, sticky="w", pady=(12, 0)
        )

        # Bottom buttons row (at the end)
        btnfrm = ttk.Frame(frm, padding=(0, 0, 0, 0))
        btnfrm.grid(row=5, column=0, columnspan=3, sticky="ew")
        btnfrm.columnconfigure(1, weight=1)
        self.btn_refresh = ttk.Button(
            btnfrm, text="Refresh", command=self.refresh)
        self.btn_refresh.grid(row=0, column=0, sticky="w", pady=(4, 0))
        self.btn_apply = ttk.Button(btnfrm, text="Apply", command=self.apply)
        self.btn_apply.grid(row=0, column=1, sticky="e", pady=(4, 0))
        self.btn_bootsel = ttk.Button(
            btnfrm, text="Reboot to BOOTSEL", command=self.on_bootsel)
        self.btn_bootsel.grid(row=0, column=2, sticky="e",
                              pady=(4, 0), padx=(8, 0))
        self.btn_debug = ttk.Button(
            btnfrm, text="Debug HID", command=self.on_debug)
        self.btn_debug.grid(row=0, column=3, sticky="e",
                            pady=(4, 0), padx=(8, 0))

        self.after(200, self.auto_connect)

    def on_brightness_change(self, v):
        self.brightness_val_label.configure(text=str(int(float(v))))

    def auto_connect(self):
        try:
            self.dev = open_device()
            self.status_var.set("Connected")
            self.refresh()
        except HIDErrors as e:
            log("auto_connect error:", e)
            self.status_var.set(f"Not connected: {e}")
            # retry later
            self.after(1500, self.auto_connect)

    def refresh(self):
        if not self.dev:
            messagebox.showwarning("Device", "Not connected")
            return
        eff, bri = get_status(self.dev)
        if eff is not None and 0 <= eff < len(EFFECTS):
            self.combo.current(eff)
        else:
            self.combo.current(0)
        if bri is not None:
            self.brightness_scale.set(bri)
            self.brightness_val_label.configure(text=str(int(bri)))
        # Extended settings
        ext = get_ext_status(self.dev)
        if ext:
            self.ppr_var.set(ext.get("enc_ppr", self.ppr_var.get()))
            self.mouse_var.set(ext.get("mouse_sens", self.mouse_var.get()))
            self.db_var.set(1 if ext.get("enc_debounce") else 0)
            self.led_count_var.set(
                ext.get("ws_led_size", self.led_count_var.get()))
            self.zones_var.set(ext.get("ws_led_zones", self.zones_var.get()))

    def apply(self):
        if not self.dev:
            messagebox.showwarning("Device", "Not connected")
            return
        idx = self.combo.current()
        try:
            set_effect(self.dev, idx)
            bri = int(float(self.brightness_scale.get()))
            set_brightness(self.dev, bri)
            # Apply encoder/mouse immediate settings
            ppr = max(1, min(4000, int(self.ppr_var.get())))
            devh = self.dev
            devh.send_feature_report(bytes(
                [REPORT_ID_CONFIG, CMD_SET_ENCODER_PPR, ppr & 0xFF, (ppr >> 8) & 0xFF, 0, 0, 0, 0, 0]))
            ms = max(1, min(50, int(self.mouse_var.get())))
            devh.send_feature_report(
                bytes([REPORT_ID_CONFIG, CMD_SET_MOUSE_SENS, ms, 0, 0, 0, 0, 0, 0]))
            # Persist debounce and WS params (take effect after reboot)
            db = 1 if self.db_var.get() else 0
            devh.send_feature_report(
                bytes([REPORT_ID_CONFIG, CMD_SET_ENC_DEBOUNCE, db, 0, 0, 0, 0, 0, 0]))
            count = max(1, min(300, int(self.led_count_var.get())))
            zones = max(1, min(16, int(self.zones_var.get())))
            devh.send_feature_report(bytes(
                [REPORT_ID_CONFIG, CMD_SET_WS_PARAMS, count & 0xFF, (count >> 8) & 0xFF, zones, 0, 0, 0, 0]))
            self.status_var.set(
                f"Applied: {EFFECTS[idx][1]}, Brightness {bri} (PPR {ppr}, Sens {ms})")
        except HIDErrors as e:
            log("apply error:", e)
            messagebox.showerror("Error", str(e))

    def on_bootsel(self):
        if not self.dev:
            messagebox.showwarning("Device", "Not connected")
            return
        if not messagebox.askyesno(
            "Reboot to BOOTSEL",
            "The device will reboot into BOOTSEL (UF2) mode. Continue?",
        ):
            return
        try:
            reboot_to_bootsel(self.dev)
            self.status_var.set("Rebooting to BOOTSEL…")
        except HIDErrors as e:
            log("reboot error:", e)
            messagebox.showerror("Error", str(e))

    def on_debug(self):
        info = hid_enumerate_info()
        log("HID enumerate info:\n" + info)
        messagebox.showinfo("HID Interfaces", info)


if __name__ == "__main__":
    # Headless CLI: send reboot-to-bootloader if requested
    if "--reboot-bootsel" in sys.argv:
        try:
            device = open_device()
            reboot_to_bootsel(device)
            print("OK: reboot command sent")
            sys.exit(0)
        except HIDErrors as e:
            log("cli reboot error:", e)
            print(f"ERR: {e}")
            sys.exit(1)
    # Default: launch GUI
    app = App()
    app.mainloop()
