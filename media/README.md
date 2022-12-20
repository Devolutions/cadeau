# Reference Test Media

This folder contains compressed reference media for testing.

## Decompressing

```
xz -d -k *.xz
```

## VLC Player

```
vlc --demux rawvideo --rawvid-fps 10 --rawvid-width 1280 --rawvid-height 720 --rawvid-chroma RV32 .\window_dragging.rgb
```
