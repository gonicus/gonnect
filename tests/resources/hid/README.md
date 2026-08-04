# HID report descriptor fixtures

Test data for `usbTest` (`tests/Usb.test.cpp`), which exercises `ReportDescriptorParser`.

## Format

Each fixture is a **raw binary** file containing the exact bytes of a HID report
descriptor - byte-for-byte what the parser receives at runtime.

On Linux you can capture a device's descriptor directly from sysfs:

```sh
cat /sys/class/hidraw/hidrawN/device/report_descriptor > tests/resources/hid/<name>.bin
```

## Naming

`<vendorId>-<productId>.bin` for real captures (e.g. `6993-b02b.bin`), or
`synthetic-<purpose>.bin` for hand-assembled descriptors. New files need to
be added to `tests/Usb.test.cpp`.
