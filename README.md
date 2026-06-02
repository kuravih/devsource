# devsource

Source devices for the testbed.
Source devices are devices that source data (i.e camera).
A devsource program interfaces the hardware device and pulls images from the camera hardware in to a shared memory file.
All source device shared memory file headers contain the following keywords.

### Keywords

| Keyword  | Type   | Python type | Description                            |
| -------- | ------ | ----------- | -------------------------------------- |
| KIND     | STRING | str         | Device kind (`CAMERA`)                 |
| SN       | STRING | str         | Serial number (8 char value)           |
| PXMAX    | DOUBLE | float       | Pixel max                              |
| FULL.W   | LONG   | int         | Detector width (px)                    |
| FULL.H   | LONG   | int         | Detector height (px)                   |
| PORT     | LONG   | int         | Link port (-1 if no control link)      |
| WIDTH    | LONG   | int         | Width (px)                             |
| HEIGHT   | LONG   | int         | Height (px)                            |
| EXPTIME  | DOUBLE | float       | Exposure time (s)                      |
| FRMRATE  | DOUBLE | float       | Frame rate (fps)                       |
| GAIN     | DOUBLE | float       | Gain (units)                           |
| TEMP     | DOUBLE | float       | Temperature (C)                        |
| ROI.TL.X | LONG   | int         | Region of interest top left x (px)     |
| ROI.TL.Y | LONG   | int         | Region of interest top left y (px)     |
| ROI.BR.X | LONG   | int         | Region of interest bottom right x (px) |
| ROI.BR.Y | LONG   | int         | Region of interest bottom right y (px) |

A devsource program also establishes a control link (a zmq interface) to send/receive additional (non-image) control data to/from the hardware.
Control data is sent as [TOML][1] and varies for each device.
The `sync` message will show the available control data.

Example transactions:


<table>
<tr>
<th>Send</th>
<th>Receive</th>
<th>Description</th>
</tr>
<tr>
<td>

```toml
settings= "sync"
```
</td>
<td>

```toml
[settings]
roi = "(220,140),(420,340)"
gain = 0.0
temperature_C = 20.0
exposureTime_s = 0.001
```
</td>
<td>
Get settings
</td>
</tr>
<tr>
<td>

```toml
[settings]
exposureTime_s = 10.001
```
</td>
<td>

```toml
[settings]
exposureTime_s = 9.99
```
</td>
<td>
Set the Exposure time (s)
</td>
</tr>
<tr>
<td>

```toml
[settings]
temperature_C = 25.0
```
</td>
<td>

```toml
[settings]
temperature_C = 20.0
```
</td>
<td>
Set the Temperature (C)
</td>
</tr>
<tr>
<td>

```toml
[settings.nudge]
x = 0
y = -8
```
</td>
<td>

```toml
[settings]
roi = "(220,140),(420,340)"
```
</td>
<td>
Move the ROI on the detector
</td>
</tr>

</table> 
Invalid control data should be ignored without crashing.
This repository includes three source device implementations.

- stbsource: Dummy camera data source.
- flisource: Finger lakes camera data source.
- vmbsource: Allied vision camera data source.

## Instructions

#### build
```bash
devsource/src$ meson setup builddir
devsource/src$ meson compile -C builddir/
```
#### run
```bash
devsource/src$ ./builddir/stbsource/stbsource
devsource/src$ ./builddir/stbsource/flisource
devsource/src$ ./builddir/stbsource/vmbsource
```

[1]: https://en.wikipedia.org/wiki/TOML