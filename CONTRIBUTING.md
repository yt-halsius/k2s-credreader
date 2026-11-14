# Contributing to K2S Credential Reader

Thanks for your interest in contributing to K2S Credential Reader.

K2S Credential Reader is intended as a receive-and-decode tool for supported credential technologies. Contributions that improve protocol decoding, interoperability, reliability, user experience, testing, or documentation are welcome.

## Project scope

The project is intentionally focused on:

- receiving credential data,
- decoding and normalizing supported formats,
- displaying credential information,
- HID output,
- diagnostic and development tooling.

The project is not intended to provide functionality for:

- replaying captured radio transmissions,
- transmitting credential or remote-control signals,
- cloning access credentials,
- storing or replaying rolling codes,
- extracting MIFARE keys or protected sector contents,
- bypassing access-control systems.

Pull requests adding functionality outside this scope may be declined.

## Before submitting an issue

Please search existing issues before opening a new one.

When reporting a bug, include:

- firmware/application version,
- Flipper Zero firmware version if relevant,
- credential technology involved,
- steps to reproduce the issue,
- expected behaviour,
- actual behaviour,
- relevant logs or screenshots where appropriate.

Do not include real access credentials, facility identifiers, secrets, keys, personal information, or other sensitive production data.

Where sample credential data is required, use synthetic or anonymized values.

## Development

Clone the repository and create a branch for your change:

```bash
git clone https://github.com/yt-halsius/k2s-credreader.git
cd k2s-credreader
git switch -c feature/my-change
```

Keep changes focused and avoid unrelated formatting or refactoring in the same pull request.

## Testing

Changes affecting credential decoding should include or update regression tests where practical.

Test vectors committed to the repository must be synthetic. Do not submit captured production credentials or identifiers from real access-control installations.

All existing tests and CI checks should pass before a pull request is submitted.

## Commits

Use concise and descriptive commit messages.

The project generally follows Conventional Commit-style prefixes, for example:

```text
feat: add support for new credential format
fix: handle malformed frame length
test: add decoder regression vectors
docs: clarify HID output behaviour
ci: update build workflow
```

A perfect Conventional Commits implementation is not required, but clear history is appreciated.

## Pull requests

Pull requests should describe:

- what was changed,
- why the change is needed,
- how it was tested,
- any compatibility or security implications.

Keep pull requests reasonably small and focused where possible.

Changes to credential decoding should explain the format or behaviour sufficiently for the implementation to be reviewed without requiring access to private infrastructure.

## Security-related changes

Do not open a public issue containing information that could expose credentials, secrets, exploitable vulnerabilities, or sensitive details about a real access-control installation.

See [SECURITY.md](SECURITY.md) for reporting security concerns.

## Documentation

Documentation improvements are welcome.

If a change affects user-visible behaviour, supported credential formats, configuration, or output, update the relevant documentation as part of the same pull request.

## Code of conduct

Be constructive and respectful when discussing issues, reviews, and contributions.

Technical disagreement is welcome. Personal attacks are not.
