# kapua-tracker

A HTTP JSON API for tracking and discovering [kapua](https://github.com/tomdionysus/kapua) nodes.

## Building

### Dependencies:

- [cmake](https://github.com/Kitware/CMake)
- [libyaml](https://github.com/yaml/libyaml)
- [boost](https://github.com/boostorg/boost)

**Debain/Ubuntu/etc**

```sh
apt-get install cmake libyaml libboost-all-dev
```

**MacOS**

```sh
brew install libyaml boost
```

### Build

```sh
git clone https://github.com/tomdionysus/kapua-tracker
cd kapua-tracker
mkdir build
cd build
cmake ..
make
```

## Documentation

- Please see the [kapua](https://github.com/tomdionysus/kapua) project

## License

Kapua is licensed under the [MIT License](https://en.wikipedia.org/wiki/MIT_License). Please see [LICENSE](LICENSE) for details.
