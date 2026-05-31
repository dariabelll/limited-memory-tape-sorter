import argparse
import shutil
import struct
import subprocess
import sys
from pathlib import Path


def write_values(path: Path, values: list[int]) -> None:
    with path.open("wb") as file:
        for value in values:
            file.write(struct.pack("<i", value))


def read_values(path: Path) -> list[int]:
    values = []

    with path.open("rb") as file:
        while True:
            data = file.read(4)

            if not data:
                break

            if len(data) != 4:
                raise RuntimeError(f"invalid binary file size: {path}")

            values.append(struct.unpack("<i", data)[0])

    return values


def write_config(path: Path, tmp_dir: Path, memory_limit_bytes: int) -> None:
    path.write_text(
        "\n".join(
            [
                "read_delay_ms=0",
                "write_delay_ms=0",
                "move_delay_ms=0",
                "rewind_delay_ms=0",
                f"memory_limit_bytes={memory_limit_bytes}",
                f"tmp_dir={tmp_dir}",
                "",
            ]
        ),
        encoding="utf-8",
    )


def run_case(
    executable: Path,
    work_dir: Path,
    case_name: str,
    input_values: list[int],
    expected_values: list[int],
    memory_limit_bytes: int,
) -> None:
    case_dir = work_dir / case_name

    if case_dir.exists():
        shutil.rmtree(case_dir)

    case_dir.mkdir(parents=True)

    input_path = case_dir / "input.bin"
    output_path = case_dir / "output.bin"
    config_path = case_dir / "config.txt"
    tmp_dir = case_dir / "tmp"

    write_values(input_path, input_values)
    write_config(config_path, tmp_dir, memory_limit_bytes)

    result = subprocess.run(
        [
            str(executable),
            str(input_path),
            str(output_path),
            str(config_path),
            "--verify",
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if result.returncode != 0:
        raise RuntimeError(
            f"sorter failed in case '{case_name}'\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}\n"
        )

    actual_values = read_values(output_path)

    if actual_values != expected_values:
        raise AssertionError(
            f"wrong output in case '{case_name}'\n"
            f"expected: {expected_values}\n"
            f"actual:   {actual_values}\n"
        )


def run_missing_config_case(executable: Path, work_dir: Path) -> None:
    case_dir = work_dir / "missing_config"

    if case_dir.exists():
        shutil.rmtree(case_dir)

    case_dir.mkdir(parents=True)

    input_path = case_dir / "input.bin"
    output_path = case_dir / "output.bin"
    config_path = case_dir / "missing_config.txt"

    write_values(input_path, [3, 2, 1])

    result = subprocess.run(
        [
            str(executable),
            str(input_path),
            str(output_path),
            str(config_path),
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if result.returncode == 0:
        raise AssertionError("sorter must fail when config file is missing")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--work-dir", required=True)

    args = parser.parse_args()

    executable = Path(args.exe)
    work_dir = Path(args.work_dir)

    if not executable.exists():
        raise RuntimeError(f"sorter executable does not exist: {executable}")

    work_dir.mkdir(parents=True, exist_ok=True)

    run_case(
        executable=executable,
        work_dir=work_dir,
        case_name="simple",
        input_values=[7, 1, 5, 3, 9, 2, 8, 4, 6, 0],
        expected_values=[0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
        memory_limit_bytes=2 * 4,
    )

    run_case(
        executable=executable,
        work_dir=work_dir,
        case_name="negative_and_duplicates",
        input_values=[4, -1, 4, 0, -10, 3, 3, -1],
        expected_values=[-10, -1, -1, 0, 3, 3, 4, 4],
        memory_limit_bytes=2 * 4,
    )

    run_missing_config_case(executable, work_dir)

    return 0


if __name__ == "__main__":
    sys.exit(main())
