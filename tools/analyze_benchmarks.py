"""Проверка полноты CSV, усреднение по методике задания и научные графики."""

import argparse
from collections import defaultdict
import csv
import json
import os
from pathlib import Path
import statistics


def write_csv(path, rows):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)


def summarize(directory, output):
    output.mkdir(parents=True, exist_ok=True)
    plan = json.loads((directory / "plan.json").read_text())
    with (directory / "raw.csv").open() as stream:
        raw = list(csv.DictReader(stream))
    expected = {(n, t, r) for n in plan["sizes"] for t in plan["threads"]
                for r in range(1, plan["repeats"] + 1)}
    actual = {(int(r["size"]), int(r["threads"]), int(r["repeat"])) for r in raw}
    assert actual == expected and len(raw) == len(expected), "Неполная серия"
    groups = defaultdict(list)
    for row in raw:
        assert float(row["seconds"]) > 0
        groups[int(row["size"]), int(row["threads"])].append(row)
    means = {}
    retained = set()
    for key, rows in groups.items():
        ordered = sorted(rows, key=lambda r: (float(r["seconds"]), int(r["repeat"])))
        central = ordered[1:-1]
        means[key] = statistics.mean(float(r["seconds"]) for r in central)
        retained.update(int(r["sequence"]) for r in central)
    summary = []
    for (n, threads), rows in sorted(groups.items()):
        seconds = [float(r["seconds"]) for r in rows]
        mean = means[n, threads]
        speedup = means[n, 1] / mean
        checksum = {int(r["checksum"]) for r in raw if int(r["size"]) == n}
        assert len(checksum) == 1
        ordered = sorted(seconds)
        summary.append({
            "size": n, "threads": threads, "samples": len(rows),
            "trimmed_mean_seconds": mean,
            "min_seconds": min(seconds), "max_seconds": max(seconds),
            "trimmed_min_seconds": ordered[1], "trimmed_max_seconds": ordered[-2],
            "speedup": speedup, "efficiency": speedup / threads,
            "checksum": next(iter(checksum)),
        })
    processed = [dict(row, included_in_mean=int(int(row["sequence"]) in retained))
                 for row in raw]
    write_csv(output / "processed.csv", processed)
    write_csv(output / "summary.csv", summary)
    analysis = {"raw_runs": len(raw), "combinations": len(summary), "sizes": {}}
    for n in plan["sizes"]:
        rows = [r for r in summary if r["size"] == n]
        best = min(rows, key=lambda r: r["trimmed_mean_seconds"])
        analysis["sizes"][str(n)] = {
            "one_matrix_mib": n * n * 8 / 1024**2,
            "three_matrices_mib": n * n * 24 / 1024**2,
            "baseline_seconds": means[n, 1],
            "best": best,
            "within_5_percent_of_best": [r["threads"] for r in rows
                if r["trimmed_mean_seconds"] <= best["trimmed_mean_seconds"] * 1.05],
        }
    assert analysis["sizes"][str(max(plan["sizes"]))]["baseline_seconds"] >= 10
    (output / "analysis.json").write_text(
        json.dumps(analysis, ensure_ascii=False, indent=2) + "\n")
    tables = []
    for n in plan["sizes"]:
        tables.extend([f"### Матрицы {n}×{n}", "",
                       "| T | Среднее время, с | Ускорение S | Эффективность, % |",
                       "|---:|---:|---:|---:|"])
        for row in summary:
            if row["size"] == n:
                tables.append(f"| {row['threads']} | {row['trimmed_mean_seconds']:.6f} "
                              f"| {row['speedup']:.3f} | {100*row['efficiency']:.2f} |")
        tables.append("")
    (output / "tables.md").write_text("\n".join(tables) + "\n")
    return summary, analysis


def plot(directory, summary, output):
    os.environ.setdefault("MPLCONFIGDIR", str(Path(".cache/matplotlib").resolve()))
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    plt.rcParams.update({"font.family": "DejaVu Sans", "font.size": 10,
                         "axes.spines.top": False, "axes.spines.right": False,
                         "svg.fonttype": "none"})
    figures = output / "figures"
    figures.mkdir(parents=True, exist_ok=True)
    sizes = sorted({r["size"] for r in summary})
    colors = ["#2563a6", "#c35a18", "#26735a"]

    def finish(fig, name):
        fig.tight_layout()
        fig.savefig(figures / f"{name}.png", dpi=170)
        plt.close(fig)

    def cores(ax):
        ax.axvline(8, color="#777777", linestyle=":", linewidth=1,
                   label="8 физических ядер")
        ax.axvline(16, color="#333333", linestyle="--", linewidth=1,
                   label="16 логических процессоров")
        ax.set_xlabel("Число рабочих потоков T")
        ax.grid(alpha=.2)

    fig, ax = plt.subplots(figsize=(11, 5.5))
    for n, color in zip(sizes, colors):
        rows = [r for r in summary if r["size"] == n]
        ax.plot([r["threads"] for r in rows],
                [r["trimmed_mean_seconds"] for r in rows],
                marker="o", markersize=4, color=color, label=f"{n}×{n}")
    cores(ax)
    ax.set(xlim=(0, 131), ylim=(0, max(r["trimmed_mean_seconds"] for r in summary)*1.07),
           ylabel="Время умножения, с",
           title="Время умножения: ниже — быстрее")
    ax.legend(fontsize=9)
    finish(fig, "time")

    fig, axes = plt.subplots(1, 2, figsize=(13, 5))
    for ax in axes:
        for n, color in zip(sizes, colors):
            rows = [r for r in summary if r["size"] == n]
            ax.plot([r["threads"] for r in rows], [r["speedup"] for r in rows],
                    marker="o", markersize=3, color=color, label=f"{n}×{n}")
        ax.plot([1, 128], [1, 128], color="#888888", linestyle="-.", label="Идеальное S = T")
        cores(ax)
        ax.set_ylabel("Ускорение S = t(1) / t(T)")
    axes[0].set(xlim=(0, 33), ylim=(0, max(17, max(r["speedup"] for r in summary)*1.2)),
                title="От 1 до 32 потоков: видны изменения скорости")
    axes[1].set(xlim=(0, 130), ylim=(0, 130), title="Все числа потоков: сравнение с идеалом")
    axes[0].legend(fontsize=8)
    fig.suptitle("Ускорение: выше — больше выигрыш относительно одного потока", fontsize=12)
    finish(fig, "speedup")

    fig, ax = plt.subplots(figsize=(10, 5))
    for n, color in zip(sizes, colors):
        rows = [r for r in summary if r["size"] == n]
        ax.plot([r["threads"] for r in rows], [100*r["efficiency"] for r in rows],
                marker="o", markersize=3, color=color, label=f"{n}×{n}")
    cores(ax)
    ax.set(xlim=(0, 130), ylim=(0, 110), ylabel="Эффективность E = S / T, %",
           title="Эффективность: доля идеального ускорения")
    ax.legend(fontsize=8)
    finish(fig, "efficiency")

    loop_path = directory / "loop-order.csv"
    if loop_path.exists():
        rows = list(csv.DictReader(loop_path.open()))
        loop_sizes = {int(r["size"]) for r in rows}
        assert len(loop_sizes) == 1
        loop_size = next(iter(loop_sizes))
        data = {}
        for order in ["ijt", "itj"]:
            values = sorted(float(r["seconds"]) for r in rows if r["order"] == order)
            assert len(values) == 5
            data[order] = statistics.mean(values[1:-1])
        fig, ax = plt.subplots(figsize=(7, 5))
        bars = ax.bar(["i → j → t", "i → t → j"], list(data.values()),
                      color=[colors[0], colors[2]], width=.55)
        ax.bar_label(bars, labels=[f"{v:.3f} с" for v in data.values()], padding=5)
        ax.set(ylabel="Время умножения, с", xlabel="Порядок циклов",
               title=f"Порядок доступа к памяти: {loop_size}×{loop_size}, один поток")
        ax.set_ylim(0, max(data.values())*1.18)
        ax.grid(axis="y", alpha=.2)
        finish(fig, "loop-order")
        (output / "loop-analysis.json").write_text(json.dumps({
            "trimmed_means": data, "ratio_ijt_to_itj": data["ijt"]/data["itj"]
        }, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=Path("results/grade3"))
    parser.add_argument("--output", type=Path, default=Path("artifacts/analysis"))
    args = parser.parse_args()
    summary, analysis = summarize(args.input, args.output)
    plot(args.input, summary, args.output)
    print(json.dumps(analysis, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
