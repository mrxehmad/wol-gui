# wol-gui

A simple Wake-on-LAN GUI for Linux (Arch-first), written in C11 with GTK3.
One window, one list of hosts, one click to send the magic packet — no
NetworkManager plugins, no Python, no shelling out to `wol`.

![screenshot](docs/screenshot.png)
<!-- placeholder: drop a screenshot at docs/screenshot.png -->

## Features

- Host list (Name / MAC / Broadcast IP / Port) in a `GtkTreeView`
- Add / Edit / Delete / **Wake** / **Wake All**; double-click a row to wake it
- Native magic packet (6x`0xFF` + 16xMAC) over UDP with `SO_BROADCAST`
- MAC validation (`AA:BB:CC:DD:EE:FF` or `AA-BB-CC-DD-EE-FF`) and IPv4
  validation with error dialogs
- Persistence via GKeyFile: `$XDG_CONFIG_HOME/wol-gui/hosts.ini`
  (default `~/.config/wol-gui/hosts.ini`; if you keep a tidy home with
  `~/.local/config` and no `XDG_CONFIG_HOME` set, the app uses that).
  Saved on every change; corrupt/missing files handled gracefully
- Status bar reports every action, including raw `sendto` errors
- Tiny dependency footprint: GTK3 only (glib/gtk3)

## Install

### AUR

Once published, install `wol-gui` from the AUR:

```sh
paru -S wol-gui     # or: yay -S wol-gui
```

No AUR helper? Build it manually with `makepkg`:

```sh
sudo pacman -S --needed base-devel git
git clone https://aur.archlinux.org/wol-gui.git
cd wol-gui && makepkg -si
```

> Note: the package is not in the AUR yet (submission pending); the steps
> above work as soon as it is published. See `packaging/aur/README.md` for
> the maintainer-side process.

### Release tarball (prebuilt binary)

From the assets of a [GitHub Release](https://github.com/mrxehmad/wol-gui/releases):

```sh
tar xzf wol-gui-<version>-x86_64.tar.gz
cd wol-gui-<version>-x86_64
sudo install -m755 wol-gui /usr/bin/wol-gui
sudo install -Dm644 wol-gui.desktop /usr/share/applications/wol-gui.desktop
sudo install -Dm644 wol-gui.svg /usr/share/icons/hicolor/scalable/apps/wol-gui.svg
```

### From source

Requires `gtk3`, `gcc`, `make`, `pkgconf` (`sudo pacman -S gtk3 base-devel`):

```sh
make
sudo make install            # PREFIX=/usr by default
# staging example:
make DESTDIR="$PWD/pkg" PREFIX=/usr install
```

## Usage

1. **Add** — fill in name, MAC, broadcast address (default `255.255.255.255`)
   and UDP port (default `9`). Invalid input shows an error dialog.
2. Select a host and press **Wake**, or just **double-click** its row.
   **Wake All** sends to every entry.
3. Everything is stored in `~/.config/wol-gui/hosts.ini`:

```ini
[host0]
name=desktop
mac=AA:BB:CC:DD:EE:FF
broadcast=192.168.1.255
port=9

[host1]
name=laptop
mac=11-22-33-44-55-66
broadcast=255.255.255.255
port=7
```

### WoL gotchas

- The target machine must have WoL enabled in BIOS/UEFI **and** on the NIC
  (e.g. `ethtool -s eth0 wol g`; Windows Fast Startup can also break it).
- Magic packets only travel within the **broadcast domain**. For hosts on
  other VLANs/subnets you need a *directed broadcast* (e.g. `192.168.5.255`)
  that your router forwards as a L2 broadcast — plain `255.255.255.255` will
  not cross routers.

## Release process

1. Bump `VERSION` in the `Makefile`, then `git tag vX.Y.Z && git push --tags`.
2. The `release.yml` workflow builds in Arch and uploads
   `wol-gui-X.Y.Z-x86_64.tar.gz` (+ `.sha256`) to a GitHub Release with auto
   notes.
3. Update the AUR manually (see `packaging/aur/README.md`); the optional
   `aur-publish` job can automate this later once configured.

## Credits

- Inspired by **[gwakeonlan](https://github.com/benbotello/gwakeonlan)** by
  Benbotello — the idea and UX of a small GTK Wake-on-LAN manager.
- Application icon based on
  ["totalcmd-lan-windows-shares"](https://www.svgrepo.com/svg/519808/totalcmd-lan-windows-shares)
  from [svgrepo.com](https://www.svgrepo.com).
- Wake-on-LAN itself is an AMD-defined standard (magic packet).

## Contributing

Issues and pull requests welcome. Keep the code warning-free under
`-Wall -Wextra`, stick to the small-module layout (`main.c`, `ui.c`,
`hosts.c`, `wol.c`), and describe your change in the PR.

## License

MIT — see [LICENSE](LICENSE).
