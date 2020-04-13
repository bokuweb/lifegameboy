# lifegameboy

R.I.P John Conway.
Conway's Game of Life written in Rust on GameBoyAdvance.

![video](./video.gif)

## Build

```
docker run --rm -v `pwd`:/code bokuweb/rust-gba
```

## Play in emulator

```
visualboyadvance-m game.gba
```

## Transfer binary to GameBoyAdvance

```
sudo optusb/optusb game.gba
```
