import argparse
import dataclasses
import gzip
import hashlib
import json
import os
import pathlib
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid

MAGIC_NUMBERS = (
    b"\x7fELF",
    b"\xfe\xed\xfa\xce",
    b"\xfe\xed\xfa\xcf",
    b"\xce\xfa\xed\xfe",
    b"\xcf\xfa\xed\xfe",
    b"\xca\xfe\xba\xbe",
    b"\xca\xfe\xba\xbf",
    b"MZ",
    b"Microsoft C/C++ MSF 7.00\r\n\x1aDS",
)
MAGIC_LENGTH = max(len(magic) for magic in MAGIC_NUMBERS)
REQUEST_TIMEOUT_SECONDS = 600
ATTEMPTS = 4
USER_AGENT = "ht-music-symbol-upload/1"


@dataclasses.dataclass
class DebugFile:
    """A debug information file split the way the server wants to receive it."""

    path: pathlib.Path
    checksum: str
    chunks: list[str]
    size: int


class Server:
    """The subset of the GlitchTip API that uploads debug information files."""

    def __init__(self, url: str, token: str, organization: str, project: str) -> None:
        """Remember where to send requests and how to authenticate them."""
        self.base = url.rstrip("/")
        self.token = token
        self.organization = organization
        self.project = project

    def request(self, method: str, url: str, body: bytes | None = None, content_type: str | None = None) -> bytes:
        """Send one authenticated request, retrying transient failures, and return the response body."""
        target = urllib.parse.urlsplit(url)
        origin = urllib.parse.urlsplit(self.base)
        if (target.scheme, target.netloc) != (origin.scheme, origin.netloc):
            raise SystemExit(f"refusing to send the token to {target.scheme}://{target.netloc}")
        headers = {"Authorization": f"Bearer {self.token}", "User-Agent": USER_AGENT}
        if content_type:
            headers["Content-Type"] = content_type
        for attempt in range(1, ATTEMPTS + 1):
            try:
                with urllib.request.urlopen(urllib.request.Request(url, body, headers, method=method),
                                            timeout=REQUEST_TIMEOUT_SECONDS) as response:
                    return response.read()
            except urllib.error.HTTPError as error:
                detail = error.read().decode(errors="replace")[:500]
                if error.code < 500 or attempt == ATTEMPTS:
                    raise SystemExit(f"{method} {target.path} returned {error.code}: {detail}")
            except (urllib.error.URLError, TimeoutError) as error:
                if attempt == ATTEMPTS:
                    raise SystemExit(f"{method} {target.path} failed: {error}")
            time.sleep(2 ** attempt)
        raise AssertionError("unreachable")

    def api(self, path: str) -> str:
        """Return the absolute URL of an API path."""
        return f"{self.base}/api/0/{path}"

    def chunk_upload_options(self) -> dict:
        """Ask the server how large chunks may be and where they go."""
        options = json.loads(self.request("GET", self.api(f"organizations/{self.organization}/chunk-upload/")))
        options["url"] = urllib.parse.urljoin(self.base + "/", options["url"])
        if "debug_files" not in options.get("accept", []):
            raise SystemExit("the server does not accept debug information files through chunk upload")
        if "gzip" not in options.get("compression", []):
            raise SystemExit("the server does not accept gzip compressed chunks")
        return options

    def upload_chunk(self, url: str, checksum: str, content: bytes) -> None:
        """Upload one gzip compressed chunk named after the SHA-1 of its uncompressed content."""
        boundary = uuid.uuid4().hex
        body = b"".join((
            f"--{boundary}\r\n".encode(),
            f'Content-Disposition: form-data; name="file_gzip"; filename="{checksum}"\r\n'.encode(),
            b"Content-Type: application/octet-stream\r\n\r\n",
            gzip.compress(content, compresslevel=6),
            f"\r\n--{boundary}--\r\n".encode(),
        ))
        self.request("POST", url, body, f"multipart/form-data; boundary={boundary}")

    def assemble(self, files: list[DebugFile]) -> dict[str, dict]:
        """Ask the server to build each file from its chunks and return its state for every file."""
        payload = {file.checksum: {"name": file.path.name, "chunks": file.chunks} for file in files}
        path = f"projects/{self.organization}/{self.project}/files/difs/assemble/"
        return json.loads(self.request("POST", self.api(path), json.dumps(payload).encode(), "application/json"))


def parse_arguments() -> argparse.Namespace:
    """Parse the directory to upload and how long to wait for the server to process it."""
    parser = argparse.ArgumentParser(description="Upload debug information files to GlitchTip in chunks")
    parser.add_argument("directory", type=pathlib.Path)
    parser.add_argument("--wait", type=int, default=900, help="seconds to wait for the server to process the files")
    return parser.parse_args()


def required_environment(name: str) -> str:
    """Return an environment variable, or stop when it is missing or empty."""
    value = os.environ.get(name, "")
    if not value:
        raise SystemExit(f"{name} is not set")
    return value


def is_debug_file(path: pathlib.Path) -> bool:
    """Tell whether a file starts like an ELF, Mach-O or PE binary or an MSF PDB."""
    with open(path, "rb") as handle:
        head = handle.read(MAGIC_LENGTH)
    return any(head.startswith(magic) for magic in MAGIC_NUMBERS)


def describe(path: pathlib.Path, chunk_size: int) -> DebugFile:
    """Hash a file whole and in chunks of the size the server accepts."""
    whole = hashlib.sha1()
    chunks: list[str] = []
    size = 0
    with open(path, "rb") as handle:
        while content := handle.read(chunk_size):
            whole.update(content)
            chunks.append(hashlib.sha1(content).hexdigest())
            size += len(content)
    return DebugFile(path, whole.hexdigest(), chunks, size)


def upload_missing(server: Server, url: str, files: list[DebugFile], states: dict[str, dict], chunk_size: int) -> int:
    """Upload every chunk the server reported missing and return how many were sent."""
    sent = 0
    for file in files:
        missing = set(states.get(file.checksum, {}).get("missingChunks", []))
        if not missing:
            continue
        uploaded = 0
        with open(file.path, "rb") as handle:
            for checksum in file.chunks:
                content = handle.read(chunk_size)
                if checksum in missing:
                    server.upload_chunk(url, checksum, content)
                    missing.discard(checksum)
                    uploaded += 1
        print(f"{file.path.name}: uploaded {uploaded} of {len(file.chunks)} chunks", flush=True)
        sent += uploaded
    return sent


def main() -> int:
    """Upload the debug information files under a directory and wait until the server has processed them."""
    arguments = parse_arguments()
    server = Server(required_environment("GLITCHTIP_URL"), required_environment("GLITCHTIP_AUTH_TOKEN"),
                    required_environment("GLITCHTIP_ORG"), required_environment("GLITCHTIP_PROJECT"))

    options = server.chunk_upload_options()
    chunk_size = int(options["chunkSize"])
    maximum = int(options.get("maxFileSize") or sys.maxsize)

    paths = sorted(path for path in arguments.directory.rglob("*") if path.is_file() and is_debug_file(path))
    if not paths:
        raise SystemExit(f"no debug information files under {arguments.directory}")
    files = [describe(path, chunk_size) for path in paths]
    for file in files:
        if file.size > maximum:
            raise SystemExit(f"{file.path} is {file.size} bytes, over the server's limit of {maximum}")
        print(f"{file.path.relative_to(arguments.directory)}: {file.size} bytes, {len(file.chunks)} chunks", flush=True)

    states = server.assemble(files)
    print(f"uploaded {upload_missing(server, options['url'], files, states, chunk_size)} chunks", flush=True)

    deadline = time.monotonic() + arguments.wait
    delay = 5
    while True:
        states = server.assemble(files)
        upload_missing(server, options["url"], files, states, chunk_size)
        pending = [file for file in files if states.get(file.checksum, {}).get("state") != "ok"]
        failed = [file for file in pending if states.get(file.checksum, {}).get("state") == "error"]
        if failed:
            for file in failed:
                print(f"{file.path.name}: {states[file.checksum].get('detail')}", file=sys.stderr)
            return 1
        if not pending:
            print(f"the server has processed all {len(files)} files", flush=True)
            return 0
        if time.monotonic() + delay > deadline:
            names = ", ".join(f"{file.path.name} ({states.get(file.checksum, {}).get('state')})" for file in pending)
            print(f"not processed within {arguments.wait} seconds: {names}", file=sys.stderr)
            return 1
        time.sleep(delay)
        delay = min(delay * 2, 60)


if __name__ == "__main__":
    sys.exit(main())
