Decent Store
============

Decentralised chunk-based storage with a simple CLI and Qt GUI client. Peers store encrypted chunks; a Kademlia-style DHT maps chunk hashes to peers.

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
./build/client get <peers> <manifest_path> <output_file> [rsa_private_key_pem]
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
# Output: path to manifest, e.g. file.bin.manifest.txt

# Download (decrypt)
./build/client get 127.0.0.1:9101,127.0.0.1:9102 file.bin.manifest.txt restored.bin rsa_private.pem
```

If no RSA keys are provided, chunks are stored in plaintext (but still integrity-checked by SHA-256 hash).

Repair
------

Tracker-based automatic repair is disabled in DHT mode; `client repair` currently prints a message and exits.

Qt GUI client
-------------

If Qt6 (Widgets + Network) is installed, `client_gui` is built:

```bash
./build/client_gui
```

Main elements:

- **Peers**: top section where you enter the peer list used by the client:
  - Format: `ip:port,ip:port` (e.g. `127.0.0.1:9101,127.0.0.1:9102`).

- **Put tab (Upload File)**:
  - Choose file to upload.
  - Configure chunk size and replicas.
  - Upload button.
  - Global progress bar and log at the bottom.
  - `Stored Files` table showing:
    - CID (currently the manifest path)
    - Filename
    - Size
    - Download button (prompts for output path and downloads via the same peers).

- **Get tab**:
  - Select an existing manifest and an output path, then download.

- **Repair tab**:
  - Present but repair is disabled in DHT mode (no tracker).

- **Network tab**:
  - `Refresh peers` button.
  - Peer table: `Peer | Status | Node ID (prefix)`.
  - Simple text “network map” showing peers with their Kademlia ID prefix and online/offline status.

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