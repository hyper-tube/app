import argparse
import collections.abc
import json
import pathlib
import re
import subprocess
import sys
import tempfile

TEMPLATE = pathlib.Path(__file__).with_name("nfpm.yaml.in")
PACKAGES = {"deb": "amd64.deb", "rpm": "x86_64.rpm"}
DPKG_INFO = pathlib.Path("/var/lib/dpkg/info")


def parse_arguments() -> argparse.Namespace:
    """Parse the AppDir, the version, the output prefix and the nfpm to run."""
    parser = argparse.ArgumentParser(description="Package a deployed AppDir as a deb and an rpm")
    parser.add_argument("--appdir", type=pathlib.Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output", required=True, help="path prefix every package name starts with")
    parser.add_argument("--nfpm", default="nfpm")
    return parser.parse_args()


def elf_files(root: pathlib.Path) -> collections.abc.Iterator[pathlib.Path]:
    """Yield every ELF file under a directory, symlinks excluded, in sorted order."""
    for path in sorted(root.rglob("*")):
        if path.is_file() and not path.is_symlink():
            with open(path, "rb") as handle:
                if handle.read(4) == b"\x7fELF":
                    yield path


def dynamic_requirements(path: pathlib.Path) -> tuple[set[str], set[tuple[str, str]]]:
    """Return the sonames an ELF file needs and the symbol versions it needs from each of them."""
    report = subprocess.run(["readelf", "--dynamic", "--version-info", "--wide", str(path)],
                            capture_output=True, text=True, check=True).stdout
    libraries = set(re.findall(r"\(NEEDED\)\s+Shared library: \[([^\]]+)\]", report))
    versions: set[tuple[str, str]] = set()
    library: str | None = None
    _, _, needs = report.partition("Version needs section")
    for line in needs.split("\n\n", 1)[0].splitlines():
        if match := re.search(r"File: (\S+)", line):
            library = match.group(1)
        elif (match := re.search(r"Name: (\S+)", line)) and library:
            versions.add((library, match.group(1)))
    return libraries, versions


def host_requirements(payload: pathlib.Path,
                      binaries: list[pathlib.Path]) -> tuple[list[str], list[tuple[str, str]]]:
    """Return the sonames and symbol versions the payload needs but does not carry itself."""
    bundled = {path.name for path in payload.rglob("*")}
    libraries: set[str] = set()
    versions: set[tuple[str, str]] = set()
    for path in binaries:
        needed, needed_versions = dynamic_requirements(path)
        libraries |= needed - bundled
        versions |= {entry for entry in needed_versions if entry[0] not in bundled}
    return sorted(libraries), sorted(versions)


def rpm_requirements(libraries: list[str], versions: list[tuple[str, str]]) -> list[str]:
    """Spell the host requirements the way rpm's own dependency generator spells them."""
    return [f"{library}()(64bit)" for library in libraries] + \
        [f"{library}({version})(64bit)" for library, version in versions]


def owning_package(library: str) -> str:
    """Return the name of the installed Debian package that ships a shared library."""
    listing = subprocess.run(["dpkg-query", "-S", f"*/{library}"], capture_output=True, text=True).stdout
    for line in listing.splitlines():
        owners, _, path = line.partition(": ")
        if path.endswith(f"/x86_64-linux-gnu/{library}"):
            return owners.split(", ")[0].split(":")[0]
    raise SystemExit(f"no installed package provides {library}")


def declared_dependency(package: str, library: str) -> str:
    """Return the dependency a package's shlibs file declares for a library, or the bare package name."""
    name = re.fullmatch(r"(.+)\.so\.(\d+(?:\.\d+)*)", library)
    for shlibs in (DPKG_INFO / f"{package}:amd64.shlibs", DPKG_INFO / f"{package}.shlibs"):
        if name and shlibs.exists():
            for line in shlibs.read_text().splitlines():
                fields = line.split(maxsplit=2)
                if len(fields) == 3 and fields[:2] == [name.group(1), name.group(2)]:
                    return fields[2]
    return package


def glibc_floor(versions: list[tuple[str, str]]) -> str:
    """Return the highest GLIBC symbol version among the requirements, spelled like 2.38."""
    found = [tuple(map(int, match.group(1).split("."))) for _, version in versions
             if (match := re.fullmatch(r"GLIBC_(\d+(?:\.\d+)*)", version))]
    return ".".join(map(str, max(found)))


def deb_requirements(libraries: list[str], versions: list[tuple[str, str]]) -> list[str]:
    """Turn the host requirements into Debian dependencies, with a version on glibc alone."""
    declared = {re.sub(r"\s*\([^)]*\)", "", declared_dependency(owning_package(library), library))
                for library in libraries}
    return sorted(f"{dependency} (>= {glibc_floor(versions)})" if dependency == "libc6" else dependency
                  for dependency in declared)


def render(values: dict[str, str]) -> str:
    """Return the nfpm template with every @KEY@ replaced by its value."""
    text = TEMPLATE.read_text()
    for key, value in values.items():
        text = text.replace(f"@{key}@", value)
    return text


def main() -> int:
    """Build the deb and the rpm from a deployed AppDir."""
    arguments = parse_arguments()
    payload = (arguments.appdir / "usr").resolve()
    binaries = list(elf_files(payload))

    libraries, versions = host_requirements(payload, binaries)
    rpm = rpm_requirements(libraries, versions)
    deb = deb_requirements(libraries, versions)
    print("deb depends:", ", ".join(deb))
    print("rpm requires:", ", ".join(rpm))

    with tempfile.NamedTemporaryFile("w", suffix=".yaml") as config:
        config.write(render({
            "VERSION": arguments.version,
            "PAYLOAD": str(payload),
            "DEB_DEPENDS": json.dumps(deb),
            "RPM_DEPENDS": json.dumps(rpm),
        }))
        config.flush()
        for packager, suffix in PACKAGES.items():
            subprocess.run([arguments.nfpm, "package", "--config", config.name, "--packager", packager,
                            "--target", f"{arguments.output}-{suffix}"], check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
