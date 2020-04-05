# lifegameboy

Conway's Game of Life written in Rust on GameBoyAdvance

## Environment

- Ubuntu19.10
- rustc 1.44.0-nightly (f509b26a7 2020-03-18)

## Setup

```
$ wget http://ftp.gnu.org/gnu/binutils/binutils-2.27.tar.gz
$ tar -zxvf binutils-2.27.tar.gz
$ cd binutils-2.27
$ sudo make
$ sudo make install
$ arm-none-eabi-objdump
```

```
$ cargo install cargo-xbuild
```

## build

```
make build
```

## Play

```
visualboyadvance-m hello.gba
```

## Transfer binary to GameBoyAdvance

```
sudo optusb/optusb hello.bin
```
