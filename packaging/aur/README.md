# wol-gui (AUR) — skeleton package

This directory contains a **template** PKGBUILD for publishing wolbox to the
AUR. It is intentionally not set up yet — fill in the placeholders before use.

## Before first submission

1. Replace `USER` in `PKGBUILD` (`url` and `source`) with your GitHub
   username.
2. Decide on the AUR package name. This skeleton uses `wol-gui`
   (`pkgname=wol-gui`); if you prefer `wolbox`, change `pkgname` and the git
   URL below accordingly.
3. Add your SSH public key to your AUR account at
   <https://aur.archlinux.org/account/> (the same account as the Arch wiki).

## Submitting / updating

```sh
# First time only: create the package via the AUR web UI ("Submit new package"),
# then clone it over SSH:
git clone ssh://aur@aur.archlinux.org/wol-gui.git
cp PKGBUILD .SRCINFO wol-gui/    # or edit directly in the clone

cd wol-gui

# Whenever you push a new upstream tag vX.Y.Z:
#   1. bump pkgver= in PKGBUILD to match the tag
#   2. refresh the metadata + checksum:
makepkg --printsrcinfo > .SRCINFO   # regenerates .SRCINFO
updpkgsums                           # rewrites sha256sums from the real tarball

git add PKGBUILD .SRCINFO
git commit -m "release vX.Y.Z"
git push
```

Notes:

- `.SRCINFO` in this repo is generated once from the skeleton; always
  regenerate it with `makepkg --printsrcinfo > .SRCINFO` before pushing.
- The CI `aur-publish` job in `.github/workflows/release.yml` can do this
  automatically via KSXGitHub/github-actions-deploy-aur once you configure the
  `AUR_SSH_PRIVATE_KEY` repository secret (the job is skipped while the secret
  is absent).
- Test locally before pushing: `makepkg -si` inside this directory.
