"""Отдельное однопоточное сравнение циклов; запускается после основной серии."""

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import random
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", default="build/loop_order_benchmark")
    parser.add_argument("--output", default="artifacts/benchmarks")
    parser.add_argument("--size", type=int, default=2200)
    args = parser.parse_args()
    if args.size < 1:
        parser.error("Размер должен быть положительным")
    executable = Path(args.executable).resolve()
    directory = Path(args.output)
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / "loop-order.csv"
    assert not output.exists(), "CSV уже существует; исходные замеры не перезаписываются"
    checksum = None
    randomizer = random.Random(20261110)
    plan = {"size": args.size, "repeats": 5, "shuffle_seed": 20261110,
            "binary_sha256": hashlib.sha256(executable.read_bytes()).hexdigest()}
    (directory / "loop-plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    with output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=[
            "size", "order", "repeat", "seconds", "checksum", "started_utc", "log"])
        writer.writeheader()
        for repeat in range(1, 6):
            orders = ["ijt", "itj"]
            randomizer.shuffle(orders)
            for order in orders:
                started = datetime.now(timezone.utc).isoformat()
                process = subprocess.run([str(executable), str(args.size), order],
                                         capture_output=True, text=True,
                                         timeout=300, check=False)
                log = directory / f"loop-{order}-r{repeat}.txt"
                log.write_text(process.stdout + process.stderr, encoding="utf-8")
                assert process.returncode == 0 and not process.stderr, str(log)
                seconds = float(re.search(r"Время умножения, с: ([0-9.]+)", process.stdout)[1])
                current_sum = int(re.search(r"Контрольная сумма: (\d+)", process.stdout)[1])
                if checksum is None:
                    checksum = current_sum
                assert checksum == current_sum and seconds > 0, str(log)
                writer.writerow({"size": args.size, "order": order, "repeat": repeat,
                                 "seconds": f"{seconds:.9f}", "checksum": current_sum,
                                 "started_utc": started, "log": str(log)})
                stream.flush()
                print(f"N={args.size}, {order}, повтор={repeat}: {seconds:.6f} с", flush=True)
    assert hashlib.sha256(executable.read_bytes()).hexdigest() == plan["binary_sha256"]


if __name__ == "__main__":
    main()
