# MSFS 2024 Add-on Manager

This native Astraea plugin scans the Microsoft Flight Simulator 2024 `Community`
folder when its page is opened.

## Features

- Detects `InstalledPackagesPath` from the MSFS 2024 `UserCfg.opt` file.
- Lists first-level Community packages and their approximate size.
- Safely enables/disables packages by adding or removing `.disabled`; content is
  never deleted.
- Opens a selected package or the Community folder in Windows Explorer.
- Installs PMDG livery ZIP archives into a new `pmdg-<package-name>` Community
  package. ZIP extraction uses Windows' built-in `tar` command and rejects
  extraction failures.
- Supports a manually selected Community folder when automatic detection fails.

The installer is intentionally conservative: it does not download liveries from
third-party sites, execute package files, or overwrite an existing package.
