# ReOxide plugin template

This repository serves as template for creating new Ghidra decompiler
plugins. **Creating plugins this way currently only works on Linux.**
Plugins compile into native shared objects and the decompiler loads them
using `dlopen`. This will change in the future, but the current setup
relies on the C++ Application Binary Interface (ABI). This means you have
to compile your plugin with exactly the same compiler as ReOxide itself.


## Building

You need working ReOxide setup for building plugins, see the setup
[guide](https://reoxide.eu/guide/getting-started). If you can run
the `reoxide` command, then you can build the plugin by using meson:

```sh
$ meson setup build
```

### Native
**NB!** This will only work safely if you also compiled ReOxide yourself.

```
$ meson install -C build
```

### Docker
If you want to build a portable plugin or use the `reoxide` pip package,
you need to use the `manylinux2014` container provided by
[the Python Packaging Authority](quay.io/pypa/manylinux2014). 
If you have Docker installed, you can use the provided Dockerfile to
build your plugin:

```sh
$ docker build . -o output_dir
```

You can then find the shared objects for your plugins in the
`output_dir` folder and copy them manually into your ReOxide
plugin folder. Running `reoxide print-plugin-dir` will show you the
path to the plugin folder.
