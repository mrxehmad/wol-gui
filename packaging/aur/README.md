# wol-gui — AUR package (skeleton)

This directory contains the **template** PKGBUILD for publishing `wol-gui`
to the AUR. It is intentionally not set up yet (AUR submission was closed at
the time of writing) — finish the steps below when you are ready.

## Before first submission

1. Set the real `# Maintainer:` name/email at the top of `PKGBUILD`.
2. The URLs already point to <https://github.com/mrxehmad/wol-gui>; adjust if
   the repo moves.
3. Add your SSH public key to your AUR account:
   <https://aur.archlinux.org/account/> (same account as the Arch wiki).

## Submitting / updating

```sh
# First time only: create the package via the AUR web UI ("Submit new package"),
# then clone it over SSH:
git clone ssh://aur@aur.archlinux.org/wol-gui.git
cp PKGBUILD .SRCINFO wol-gui/    # or edit directly in the clone

cd wol-gui

# Whenever you push a new upstream tag vX.Y.Z:
#   1. bump pkgver= in PKGBUILD to match the tag
#   2. refresh metadata + checksum:
makepkg --printsrcinfo > .SRCINFO   # regenerates .SRCINFO
updpkgsums                           # rewrites sha256sums from the real tarball

git add PKGBUILD .SRCINFO
git commit -m "release vX.Y.Z"
git push
```

Notes:

- `.SRCINFO` here is generated from the skeleton; always regenerate it with
  `makepkg --printsrcinfo > .SRCINFO` before pushing.
- The optional `aur-publish` job in `.github/workflows/release.yml` can do
  this automatically via KSXGitHub/github-actions-deploy-aur once you add the
  `AUR_SSH_PRIVATE_KEY` repository secret (the job is commented out for now).
- Test locally before pushing: `makepkg -si` inside this directory.

## Users: install from the AUR without an AUR helper

```sh
sudo pacman -S --needed base-devel git
git clone https://aur.archlinux.org/wol-gui.git
cd wol-gui && makepkg -si
```
