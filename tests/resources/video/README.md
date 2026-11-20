# Encoded video fixtures

Test data for `videoDecoderTest` (`tests/VideoDecoder.test.cpp`), which exercises `VideoDecoder`.

## Format

Each fixture is an IVF file: a 32 byte file header ("DKIF"), followed by frames with a 12 byte
header (little endian payload size and timestamp). The payloads are the same bitstream that
WebRTC hands to an encoded transform.

All fixtures show 5 frames of plain red (BT.601 limited range Y/U/V = 81/90/240) in 64×48.
They are encoded without hidden frames, so every payload decodes to exactly one image.

## Generation

```sh
SRC="-f lavfi -i color=c=red:s=64x48:r=10:d=0.5"
ffmpeg $SRC -c:v libvpx -deadline realtime -auto-alt-ref 0 -lag-in-frames 0 vp8.ivf
ffmpeg $SRC -c:v libvpx-vp9 -deadline realtime -lag-in-frames 0 vp9.ivf
ffmpeg $SRC -c:v libvpx-vp9 -profile:v 2 -pix_fmt yuv420p10le -deadline realtime -lag-in-frames 0 vp9-10bit.ivf
ffmpeg $SRC -c:v libaom-av1 -usage realtime -cpu-used 8 -lag-in-frames 0 av1.ivf
```
