import argparse
import concurrent.futures
import hashlib
import json
import os
import pathlib
import subprocess
import sys


class FileDigests:
    """Hash files by content, reading each path at most once."""

    def __init__(self) -> None:
        """Start with no file hashed."""
        self.known: dict[str, str | None] = {}

    def of(self, path: str) -> str | None:
        """Return the SHA-256 of a file, or None when it cannot be read."""
        if path not in self.known:
            try:
                with open(path, "rb") as handle:
                    self.known[path] = hashlib.file_digest(handle, "sha256").hexdigest()
            except OSError:
                self.known[path] = None
        return self.known[path]


def parse_arguments() -> argparse.Namespace:
    """Parse the options before -- and keep everything after it as the tool command."""
    argv = sys.argv[1:]
    separator = argv.index("--") if "--" in argv else len(argv)
    parser = argparse.ArgumentParser(usage="%(prog)s [--jobs N] [--depends-on FILE]... build results -- tool [arguments]")
    parser.add_argument("build", type=pathlib.Path)
    parser.add_argument("results", type=pathlib.Path)
    parser.add_argument("--jobs", type=int, default=os.cpu_count())
    parser.add_argument("--depends-on", type=pathlib.Path, action="append", default=[])
    arguments = parser.parse_args(argv[:separator])
    arguments.command = argv[separator + 1:]
    if not arguments.command:
        parser.error("no tool command after --")
    return arguments


def translation_units(build: pathlib.Path) -> dict[pathlib.Path, list[dict[str, str]]]:
    """Group the compile database entries by source file, leaving out sources generated in the build."""
    units: dict[pathlib.Path, list[dict[str, str]]] = {}
    for entry in json.loads((build / "compile_commands.json").read_text()):
        source = pathlib.Path(entry["directory"], entry["file"]).resolve()
        if not source.is_relative_to(build):
            units.setdefault(source, []).append(entry)
    return units


def recorded_dependencies(build: pathlib.Path) -> dict[str, list[str]]:
    """Map each object file to every file the compiler read for it, as ninja recorded them."""
    listing = subprocess.run(["ninja", "-C", str(build), "-t", "deps"], capture_output=True, text=True, check=True)
    dependencies: dict[str, list[str]] = {}
    current: list[str] | None = None
    for line in listing.stdout.splitlines():
        if line.startswith(" "):
            if current is not None:
                current.append(os.path.normpath(build / line.strip()))
        elif line:
            output, _, state = line.partition(": #deps")
            current = [] if state.endswith("(VALID)") else None
            if current is not None:
                dependencies[os.path.normpath(build / output)] = current
    return dependencies


def tool_identity(command: list[str], inputs: list[pathlib.Path]) -> str:
    """Hash the tool's command line, its version and the configuration files it reads."""
    version = subprocess.run([command[0], "--version"], capture_output=True, text=True, check=True).stdout
    hasher = hashlib.sha256()
    for part in [*command, version]:
        hasher.update(part.encode() + b"\0")
    for path in inputs:
        hasher.update(path.read_bytes())
    return hasher.hexdigest()


def fingerprint(identity: str, entries: list[dict[str, str]], dependencies: dict[str, list[str]],
                digests: FileDigests) -> str | None:
    """Hash everything a unit's result depends on, or return None when some of it is unknown."""
    hasher = hashlib.sha256(identity.encode())
    for entry in entries:
        recorded = dependencies.get(os.path.normpath(pathlib.Path(entry["directory"], entry.get("output", ""))))
        if recorded is None:
            return None
        hasher.update(json.dumps(entry, sort_keys=True).encode())
        for path in sorted(set(recorded)):
            digest = digests.of(path)
            if digest is None:
                return None
            hasher.update(f"{path}\0{digest}\0".encode())
    return hasher.hexdigest()


def analyze(command: list[str], source: pathlib.Path) -> tuple[bool, str]:
    """Run the tool on one unit and return whether it passed, with everything it printed."""
    completed = subprocess.run([*command, str(source)], capture_output=True, text=True)
    output = completed.stdout + completed.stderr
    clean = completed.returncode == 0 and "warning:" not in output and "error:" not in output
    return clean, output


def run(command: list[str], sources: list[pathlib.Path], jobs: int | None) -> set[pathlib.Path]:
    """Analyze the units in parallel, print each result as it arrives, and return the units that failed."""
    failures: set[pathlib.Path] = set()
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(analyze, command, source): source for source in sources}
        for index, future in enumerate(concurrent.futures.as_completed(futures), start=1):
            source = futures[future]
            clean, output = future.result()
            print(f"[{index}/{len(sources)}] {'passed' if clean else 'FAILED'} {source}", flush=True)
            if not clean:
                failures.add(source)
                print(output, flush=True)
    return failures


def record(results: pathlib.Path, fingerprints: set[str]) -> None:
    """Leave exactly the given fingerprints in the results directory."""
    for stale in {entry.name for entry in results.iterdir()} - fingerprints:
        (results / stale).unlink()
    for fresh in fingerprints:
        (results / fresh).touch()


def main() -> int:
    """Analyze the units that changed since they last passed and remember the ones that pass now."""
    arguments = parse_arguments()
    build = arguments.build.resolve()
    units = translation_units(build)
    dependencies = recorded_dependencies(build)
    identity = tool_identity(arguments.command, arguments.depends_on)
    digests = FileDigests()

    arguments.results.mkdir(parents=True, exist_ok=True)
    passed = {entry.name for entry in arguments.results.iterdir()}
    fingerprints = {source: fingerprint(identity, entries, dependencies, digests) for source, entries in units.items()}
    pending = sorted(source for source, known in fingerprints.items() if known not in passed)

    failures = run(arguments.command, pending, arguments.jobs)
    record(arguments.results, {known for source, known in fingerprints.items() if known and source not in failures})

    print(f"{len(units)} translation units: {len(units) - len(pending)} unchanged since they passed, "
          f"{len(pending)} analyzed, {len(failures)} failed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
