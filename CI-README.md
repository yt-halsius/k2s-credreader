# CI baseline

This bundle adds two deliberately simple quality gates.

## 1. Host-side RCO regression tests

Run locally:

```bash
make -C tests test
```

The tests cover:

- all eight known factory-code vectors
- all four channels for Remote 1
- all four channels for Remote 2
- all eight known 34-bit diagnostic vectors
- invalid channel, factory-width, prefix and frame-length inputs

The tests compile only the pure `app/rco_leb.c` decoder. They do not need the
Flipper SDK or Momentum.

## 2. Momentum FAP build

`.github/workflows/ci.yml` clones Momentum and checks out a pinned revision
before copying `app/` into `applications_user/k2s_credential_reader`.

The initial pin is:

```text
8ed809f
```

That was the `dev` head shown by Momentum's repository when this CI bundle was
created. If your local Momentum checkout is intentionally on another revision,
replace `MOMENTUM_REF` with:

```bash
git -C /path/to/Momentum-Firmware rev-parse HEAD
```

Do not change the pin automatically on every build; update it deliberately,
build/test locally, then commit the new hash.

The successful FAP is uploaded as the GitHub Actions artifact:

```text
k2s-credential-reader-fap
```
