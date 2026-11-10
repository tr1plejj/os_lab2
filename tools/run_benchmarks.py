"""Последовательные запуски C++-программы; Python матрицы не умножает."""

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import random
import re
import subprocess
import time


FIELDS = ["sequence", "size", "threads", "repeat", "seconds", "checksum",
          "wall_seconds", "cpu_seconds", "started_utc", "log"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", default="build/matrix_demo")
    parser.add_argument("--output", default="artifacts/benchmarks")
    parser.add_argument("--sizes", type=int, nargs="+", default=[800, 1600, 2200])
    parser.add_argument("--threads", type=int, nargs="+",
                        default=[*range(1, 17), 20, 24, 28, 32, 64, 128])
    parser.add_argument("--repeats", type=int, default=5)
    args = parser.parse_args()
    if args.repeats < 3 or min(args.sizes + args.threads) < 1:
        parser.error("Положительные размеры и потоки, не меньше трех повторов")
    if 1 not in args.threads:
        parser.error("Для расчета ускорения нужен однопоточный запуск T=1")
    if len(set(args.sizes)) != len(args.sizes) or len(set(args.threads)) != len(args.threads):
        parser.error("Размеры и числа потоков не должны повторяться")

    executable = Path(args.executable).resolve()
    output = Path(args.output)
    logs = output / "runs"
    logs.mkdir(parents=True, exist_ok=True)
    binary_hash = hashlib.sha256(executable.read_bytes()).hexdigest()
    plan = {"sizes": args.sizes, "threads": args.threads,
            "repeats": args.repeats, "shuffle_seed": 20261110,
            "binary_sha256": binary_hash}
    plan_path = output / "plan.json"
    if plan_path.exists():
        assert json.loads(plan_path.read_text()) == plan, "Изменился план или бинарник"
    else:
        plan_path.write_text(json.dumps(plan, indent=2) + "\n")

    raw = output / "raw.csv"
    previous = list(csv.DictReader(raw.open())) if raw.exists() else []
    done = {(int(r["size"]), int(r["threads"]), int(r["repeat"])) for r in previous}
    assert len(done) == len(previous), "Повторяющиеся записи в CSV"
    checksums = {}
    for row in previous:
        n = int(row["size"])
        checksum = int(row["checksum"])
        assert checksums.setdefault(n, checksum) == checksum

    jobs = []
    order = random.Random(plan["shuffle_seed"])
    for repeat in range(1, args.repeats + 1):
        batch = [(n, t, repeat) for n in args.sizes for t in args.threads]
        order.shuffle(batch)
        jobs.extend(batch)

    with raw.open("a", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS)
        if not previous:
            writer.writeheader()
        for n, threads, repeat in jobs:
            if (n, threads, repeat) in done:
                continue
            sequence = len(done) + 1
            started = datetime.now(timezone.utc).isoformat()
            before = os.times()
            start = time.monotonic()
            process = subprocess.run(
                [str(executable), str(n), str(n), str(n), str(threads)],
                text=True, capture_output=True, timeout=300, check=False,
            )
            wall = time.monotonic() - start
            after = os.times()
            log = logs / f"n{n}-t{threads}-r{repeat}.txt"
            log.write_text(process.stdout + process.stderr, encoding="utf-8")
            assert process.returncode == 0 and not process.stderr, str(log)
            seconds_match = re.search(r"Время умножения, с: ([0-9.]+)", process.stdout)
            checksum_match = re.search(r"Контрольная сумма: (\d+)", process.stdout)
            assert seconds_match and checksum_match, str(log)
            seconds = float(seconds_match[1])
            checksum = int(checksum_match[1])
            assert seconds > 0 and seconds <= wall, str(log)
            assert checksums.setdefault(n, checksum) == checksum, str(log)
            progress = list(map(int, re.findall(r"Прогресс: (\d+)%", process.stdout)))
            assert progress[0] == 0 and progress[-1] == 100, str(log)
            assert all(a < b for a, b in zip(progress, progress[1:])), str(log)
            cpu = ((after.children_user + after.children_system) -
                   (before.children_user + before.children_system))
            writer.writerow(dict(zip(FIELDS, [
                sequence, n, threads, repeat, f"{seconds:.9f}", checksum,
                f"{wall:.9f}", f"{cpu:.6f}", started, str(log),
            ])))
            stream.flush()
            os.fsync(stream.fileno())
            done.add((n, threads, repeat))
            print(f"[{len(done)}/{len(jobs)}] N={n}, T={threads}, "
                  f"повтор={repeat}: {seconds:.6f} с; сумма={checksum}", flush=True)
    assert hashlib.sha256(executable.read_bytes()).hexdigest() == binary_hash
    print("Серия завершена, все контрольные суммы и сообщения прогресса совпали.")


if __name__ == "__main__":
    main()
