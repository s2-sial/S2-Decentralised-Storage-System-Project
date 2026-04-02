Decent Store
============

Decentralised chunk-based storage with a simple CLI and Qt GUI client. Peers store encrypted chunks; a Kademlia-style DHT maps chunk hashes to peers. Uploads return a **share ID** (`dss://file/<sha256(manifest_text)>`); manifests are stored on peers and a local `.manifest.txt` copy is written for debugging.

Build
-----

1. Open Ubuntu WSL and change to the project root:
   ```bash
   cd /home/saad/~projects/decent-store
   ```
2. Configure and build:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build . -j
   ```

Running peers
-------------

Start one or more peers. Each peer has its own storage directory and optional max storage limit:

```bash
./build/peer <advertise_ip> <peer_port> <storage_dir> [max_bytes]

# Examples:
./build/peer 127.0.0.1 9101 ./store1
./build/peer 127.0.0.1 9102 ./store2 1073741824   # 1 GiB limit
```

Peers use a local storage manager with:
- on-disk layout: `storage_dir/chunks/`, `storage_dir/manifests/`, `storage_dir/metadata.db`
- max storage limit (bytes)
- LRU chunk eviction
- disk usage tracking

Client (CLI)
------------

Usage:

```text
./build/client put <peers> <file_path> [chunk_size_bytes] [replicas] [rsa_public_key_pem]
./build/client get <peers> <share_id_or_manifest_path> <output_file> [rsa_private_key_pem]
./build/client repair
```

- `<peers>`: comma-separated list of `ip:port`, e.g. `127.0.0.1:9101,127.0.0.1:9102`.
- `chunk_size_bytes`: default `1048576` (1 MiB) if omitted.
- `replicas`: number of peers to store each chunk on (capped at peer count).
- `rsa_public_key_pem` (optional): if provided, the file is encrypted with AES-256-GCM and the AES key+IV are wrapped with this RSA public key and stored in the manifest.
- `rsa_private_key_pem` (optional): if provided, encrypted files are decrypted using this RSA private key on download.

Examples:

```bash
# Start peers
./build/peer 127.0.0.1 9101 ./store1
./build/peer 127.0.0.1 9102 ./store2

# Generate RSA key pair (for hybrid encryption)
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out rsa_private.pem
openssl rsa -in rsa_private.pem -pubout -out rsa_public.pem

# Upload (encrypted)
./build/client put 127.0.0.1:9101,127.0.0.1:9102 ./file.bin 1048576 2 rsa_public.pem
# Prints share ID (dss://file/...) and local manifest path

# Download (decrypt) by share ID or local manifest path
./build/client get 127.0.0.1:9101,127.0.0.1:9102 dss://file/<hex> restored.bin rsa_private.pem
./build/client get 127.0.0.1:9101,127.0.0.1:9102 file.bin.manifest.txt restored.bin rsa_private.pem
```

If no RSA keys are provided, chunks are stored in plaintext (but still integrity-checked by SHA-256 hash).

Repair
------

Tracker-based automatic repair is disabled in DHT mode; `client repair` currently prints a message and exits.

Single application (recommended)
----------------------------------

The primary user-facing binary is **`decent_store`**: one Qt process that runs the client UI, optional **embedded peer** (storage + listener), **bootstrap discovery**, and **DHT routing** without a separate peer process for normal use. Standalone `peer` and CLI `client` remain useful for scripting, tests, and headless nodes.

If Qt6 (Widgets + Network) is installed, `decent_store` is built:

```bash
./build/decent_store
```

Install (optional; installs `decent_store`, `client`, `peer`, and `tracker` under `CMAKE_INSTALL_PREFIX/bin`):

```bash
cmake --install build
```

Qt GUI (`decent_store`)
-----------------------

Settings align with `QApplication` organisation/name (`decent_store` / `decent_store`), e.g. `~/.config/decent_store/decent_store.conf` on Linux. First run can prompt for peer contribution (embedded peer), storage limit, and port. Keys include `peer/enabled`, `peer/port`, `peer/max_bytes`, `peer/storage_dir`, `network/bootstrap_seeds`, and `network/advertise_ip`.

Main elements:

- **Network Join (Bootstrap)** (top): read-only **Discovered peers** field (`ip:port,...`) populated after TCP probes and optional local embedded peer merge. Status line explains client-only vs routing mode.

- **Put tab (Upload File)**:
  - Choose file to upload.
  - Configure chunk size and replicas.
  - Optional **RSA public key** (`.pem`) for hybrid encryption (AES-256-GCM + RSA-OAEP key wrap); leave empty for plaintext chunks.
  - Upload button.
  - Global progress bar and log at the bottom.
  - **Stored Files** table: **Share ID**, name, size, **Download** (uses the share ID with the same peer list and the **Get** tab’s private key if set).

- **Get tab**:
  - Enter `dss://file/<sha256>` or a path to `.manifest.txt`, plus output path, then download.
  - Optional **RSA private key** (`.pem`) when the upload was encrypted; required to decrypt.

- **Repair tab**:
  - Explains that tracker-based repair is unavailable in DHT-only mode; controls are disabled.

- **Network tab**:
  - **Routing configuration**: edit bootstrap seeds and advertise address; **Apply network settings** saves to `QSettings` and restarts the embedded peer if needed.
  - **Refresh peers**, peer table (`Peer | Status | Node ID (prefix)`), and a short network map.

DHT and integrity
-----------------

- A simplified Kademlia DHT assigns each `chunk_hash` to peers based on XOR distance between node IDs and the chunk’s key ID.
- On upload, chunks are stored to the K closest peers.
- On download, the client:
  - uses the DHT mapping to choose peers,
  - downloads encrypted chunks (if enabled),
  - decrypts using the file key from the manifest (if encrypted),
  - verifies the SHA-256 hash of the plaintext against the manifest’s `chunkId`.

Chunks that fail integrity verification are rejected and the client tries another peer; if none succeed, the download fails for that chunk.


