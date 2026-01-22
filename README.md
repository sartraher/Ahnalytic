<p align="center">
  <a target="_blank">
    <img src="https://github.com/sartraher/Ahnalytic/blob/main/images/logo_big.png"
         height="200"
         width="200"
         alt="Ahnalytic Logo" />
  </a>
</p>

# Ahnalytic

> ⚠️ **Important – Pre-Alpha Software**  
> This project is currently in a **pre-alpha** state.  
> Core components are still under active development and **not all essential features are present yet**.  
> Expect breaking changes.

---

## What is Ahnalytic?

**Ahnalytic** is a **source snippet scanner** that analyzes your source code and compares it against a large, curated database of **open-source code** to detect reused or inherited code fragments.

Key principles:

- 🔒 **Fully local scanning** — your source code never leaves your machine
- 📦 Open-source reference data is **downloaded locally** for comparison
- 🧬 Focus on *code ancestry*

Scan performance depends heavily on:
- the hardware running the scan
- the amount of input source code
- the size of the reference database

---

## AhnalyticScannerServer

### What it is

- A scanner service for **Windows** and **Linux**
- A built-in **web server** providing a UI interface

### What it is not (and likely never will be)

- ❌ User or rights management  
  → You are responsible for controlling access to the server and its data
- ❌ Hardened against external attacks (DDoS, intrusion, etc.)  
  → If an attacker can reach this server, the security model has already failed

This software is intended to run **inside a trusted environment**.

---

## AhnalyticUpdateServer

> ⚠️ **Important – Unfinished**  
> The update server is **not ready for use yet**.  
> Do **not** rely on it in production.

---

## Requirements

### General
- Premake
- Git CLI
- Node.js (Web UI)

### OpenSSL build dependencies
- Perl
- NASM

### Platform-specific
- **Windows**
  - Visual Studio 2022+
- **Linux**
  - GCC / G++
  - make

### Future use
- SVN CLI
- Mercurial CLI

---

## Hardware Requirements

- Little endian(for now)
- AVX2 support(for now)

---

## Build Instructions

### Windows

```bash
premake5 vs2022
Open Ahnalytic.sln in the repository root and build using Visual Studio.
```

### Linux

```bash
premake5 gmake2
make config=release_x64
```

# Data

The largest component of Ahnalytic is the generated reference data.

You can generate this data yourself using the AhnalyticUpdateServer,
but this is strongly discouraged unless absolutely necessary.

⚠️ Generating the full dataset can take multiple months.

# Licensing

- Software source code: MIT License
- Provided reference data: CC BY-SA 4.0

If you generate the data yourself, you may license it however you want.

# Downloads

Databases:
- Stackoverflow — [DB Based von Datadump Q3 2025](https://drive.google.com/file/d/17swDsXb57s9IVkeRSDesefxS29xgUt06/view?usp=sharing)
- GitHub — currently scanning, check back in a few months
- SourceForge — scanner (not written yet)

# Setup

Place the database content into the `db` folder.

Your project structure should look like this:

Ahnalytic

```
├─ db
  │ ├─ base
  │ └─ CPP
```

If you want to have subfolders in different locations, edit the paths in `ahnalytic.cfg`.

## Optional

- Change the address and port in the `ScanServer` section of `ahnalytic.cfg`.
- Start the Scan Server from the `bin` folder with:

## Start

AhnalyticScannerServer -f

- Open a web browser and navigate to the configured address, e.g.:
http://127.0.0.1:9080/www/index.html

## <ANYFILENAME>.analytic

To provide the scanner with additional information, you can add a <ANYFILENAME>.analytic file into a subfolder inside the ZIP archive you upload for scanning.

The file uses a standard config-style format:

```ini
[ANYBLOCKNAME]
type=Content|3rdParty|CVE|Ignore
...
```

### Content

Allows you to add filters to your scan results.

```ini
[ANYBLOCKNAME]
type=Content
dbFile=Database file shown in scan results
searchFile=Search file shown in scan results
reason=FalsePositive|NoCreativeValue|AllowedByAuthor|Other
comment=Optional comment, mostly used for the "Other" reason
```

### 3rdParty

Reserved for later use; currently behaves the same as Ignore.

```ini
[ANYBLOCKNAME]
type=3rdParty
vendor=
product=
copyright=
version=
displayName=
displayVersion=
url=
date=
```

## CVE

Reserved for later use. Allows closing CVEs with a reason.

```ini
[ANYBLOCKNAME]
type=CVE
id=
status=Open|Closed
comment=
```

## Ignore

Prevents the contents of this folder and all its subfolders from being scanned.

```ini
[ANYBLOCKNAME]
type=Ignore
```

# Supported Languages

Currently supported:
- C
- C++

The initial focus on C/C++ is due to the significant time required to scan and prepare the reference data.
Once essential C/C++ sources are covered, the next language will be selected.

# Roadmap

- Further search performance improvements
- Add SourceForge crawler
- Decide on and add the next programming language