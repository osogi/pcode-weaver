# Pcode Weaver

Experimental ReOxide(Ghidra) plugin for applying custom p-code rewrite rules
with validation checks.

The project has two parts:

- a rule compiler that reads text `.rule` files and emits compiled `.pwrule`
  files;
- a ReOxide plugin that loads `.pwrule` files and applies them during
  decompilation.

<img width="813" height="583" alt="Demo" src="https://github.com/user-attachments/assets/8b72f234-2b61-44bd-b9e8-46f61d8ef7ec" />

---

## Requirements

- Linux
- Ghidra, tested on 12.0
- ReOxide 0.7.2+

For building:

- Meson `>= 1.10`
- Ninja
- C++23 compiler
- Flex and Bison
- Docker, for the portable plugin build

---

## Quick Start

1. Install ReOxide in a virtual environment:

```sh
python3 -m venv venv
source venv/bin/activate
python3 -m pip install reoxide==0.7.2
```

2. Set up ReOxide. If something goes wrong, check the
   [official setup guide](https://reoxide.eu/guide/getting-started).

```sh
reoxide init-config
# Creating new basic config.
# Enter a Ghidra root install directory: /home/user/ghidra
# Config saved to /home/user/.config/reoxide/reoxide.toml

reoxide link-ghidra
```

3. Install `libpcode-weaver.so`:

```sh
# Download or build it first, then copy it into the ReOxide plugin directory.
cp libpcode-weaver.so "$(reoxide print-plugin-dir)/"
```

4. Add the `pcodeweaver` action to the ReOxide decompilation pipeline:

```sh
scripts/install-pcodeweaver-action.sh
```

You can also add it manually in `$(reoxide print-plugin-dir)/../current.yaml`
or `~/.local/share/reoxide/current.yaml`:

```yaml
# ...
- action: prototypewarnings
  group: protorecovery

# Add pcodeweaver just before the stop action.
- action: pcodeweaver
  group: analysis

- action: stop
  group: base
```

5. Start the ReOxide daemon:

```sh
reoxided
# 2026-05-08T04:38:58 INFO reoxide - Restarting with updated LD_LIBRARY_PATH...
# 2026-05-08T04:38:58 INFO reoxide - Using data_dir: /home/user/.local/share/reoxide
# 2026-05-08T04:38:58 INFO reoxide - Loading /home/user/.local/share/reoxide/plugins/libcore.so
# 2026-05-08T04:38:58 INFO reoxide - Loading /home/user/.local/share/reoxide/plugins/libpcode-weaver.so
# ...
```

6. Download or build `compiler.elf`.

7. Compile and install a rule:

```sh
echo "o_tmp -> v1 -> (0) o1(INT_ADD, _) -- #4 ->> (0) o1" > /tmp/change_plus_first_arg.rule

compiler.elf --rules-dir /tmp/change_plus_first_arg.rule
```

8. Launch Ghidra and enjoy.

Rule language documentation: [RULE_SYNTAX.md](RULE_SYNTAX.md).

---

## Build

### Compiler

```sh
cd src/compiler
# Or any other C++23 compiler
CXX=g++-13 meson setup build --buildtype release
meson compile -j 1 -C build
```

`-j 1` is recommended because the compiler project uses Meson's unstable
Flex/Bison code-generation module.

Compiler binary:

```sh
src/compiler/build/bin/compiler.elf
```

### Plugin: Docker Build

This is the recommended way to build a plugin for the `reoxide` pip package.

The Dockerfile defaults to `REOXIDE_VERSION=0.7.2`. Change it if your ReOxide
version is different.

```sh
docker build -o tmp/out .
```

Compiled plugin:

```sh
tmp/out/libpcode-weaver.so
```

Install it into ReOxide, with the ReOxide environment active:

```sh
cp tmp/out/libpcode-weaver.so "$(reoxide print-plugin-dir)/"
```

### Plugin: Native Build

Use this only when your local compiler ABI matches the ReOxide build. This is
usually safest when ReOxide was built locally with the same compiler.

```sh
cd src/plugin
meson setup build --buildtype release
meson install -C build
```

`meson install` installs the plugin into the ReOxide plugin directory.

---

## Details

### Rule Directory

Compiled rules are loaded from the first available directory in this order:

1. `$PCODE_WEAVER_RULE_DIR`
2. `$XDG_DATA_HOME/pcode-weaver`
3. `$HOME/.local/share/pcode-weaver`
4. `.local/share/pcode-weaver`

Manual rule installation:

```sh
mkdir -p "$HOME/.local/share/pcode-weaver"
cp path/to/rule.pwrule "${XDG_DATA_HOME:-$HOME/.local/share}/pcode-weaver/"
```

### Updating Rules

Rules are loaded when the plugin starts. To use updated rules, restart the
decompilation process in Ghidra, for example by closing and reopening the
CodeBrowser.

### Rule Application Order

Rules are applied in lexicographic order by filename.

Each rule is retried while it can still be applied. Be careful not to create a
rule that can apply forever. When a rule no longer matches, Pcode Weaver moves
to the next rule.

---

## Related Projects

Used by this project:

- [ReOxide](https://reoxide.eu/) - native plugin support for the Ghidra
  decompiler.
- [Ghidra](https://github.com/NationalSecurityAgency/ghidra) - the reverse
  engineering framework and p-code/decompiler backend.
- [cereal](https://uscilab.github.io/cereal/) - serialization for compiled
  `.pwrule` files.

Related work and inspiration:

- [High P-Code Graph Viewer](https://github.com/osogi/HighPcodeGraphViewer) -
  my Ghidra plugin for viewing high p-code graphs.
- [RULECOMPILE - Undocumented Ghidra decompiler rule language](https://msm.lt/re/ghidra/rulecompile/) -
  a write-up about Ghidra's hidden decompiler rule compiler and one of the main
  inspirations for this project.
- [RuleChef](https://github.com/LukeSerne/RuleChef) - DSL that generates Ghidra
  decompiler C++ rule code.
