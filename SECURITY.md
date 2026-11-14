# Security Policy

## Supported versions

K2S Credential Reader is currently a small open source project under active development.

Security fixes will normally target the latest version on the `main` branch and the most recent release.

Older releases may not receive security updates.

## Reporting a vulnerability

Please do not report security vulnerabilities through a public GitHub issue.

Use GitHub's private vulnerability reporting feature for this repository if it is available.

If private vulnerability reporting is not available, contact the repository maintainer privately through their GitHub profile before sharing technical details publicly.

Please include:

- a description of the issue,
- affected version or commit,
- steps required to reproduce it,
- potential impact,
- any suggested mitigation or fix.

Do not include real access credentials, keys, secrets, personally identifiable information, or sensitive information about a production access-control installation unless it is strictly necessary to demonstrate the issue.

Synthetic examples are strongly preferred.

## Scope

Security reports relating to the following are particularly relevant:

- malformed input causing crashes or memory corruption,
- unsafe handling of credential data,
- unintended credential disclosure,
- HID output producing unintended or unsafe behaviour,
- vulnerabilities in credential parsers or decoders,
- dependency or build-chain vulnerabilities,
- GitHub Actions or release-pipeline security,
- vulnerabilities that enable functionality beyond the intended receive/decode scope.

## Project security boundaries

K2S Credential Reader is designed primarily for:

```text
receive
→ decode
→ normalize
→ display / HID output
```

The project is deliberately not designed to provide:

```text
capture → replay
credential cloning
radio transmission
rolling-code replay/storage
MIFARE key extraction
protected-sector dumping
access-control bypass
```

A report showing that the application unintentionally crosses one of these boundaries is considered security-relevant.

## Credential and radio data

Credential identifiers and radio frames may represent real access-control devices.

When submitting reports, test cases, issues, or pull requests:

- use synthetic credential identifiers whenever possible,
- do not publish credentials captured from production systems,
- do not publish facility-specific access data,
- do not publish cryptographic keys or secrets,
- remove unrelated identifying information from logs and captures.

## Disclosure

Please allow reasonable time for investigation and remediation before publicly disclosing a security issue.

Once a fix is available, relevant security information may be documented publicly where doing so helps users understand the affected versions and required action.

## Good-faith research

Good-faith security research that respects applicable law, privacy, and systems you are authorized to test is welcome.

Please avoid testing against systems, credentials, or infrastructure without authorization.
