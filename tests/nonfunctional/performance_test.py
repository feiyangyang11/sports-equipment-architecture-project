import argparse
import concurrent.futures
import json
import math
import statistics
import threading
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path


def query_once(url: str, start_event: threading.Event) -> dict:
    start_event.wait()
    started = time.perf_counter()
    status = 0
    error = ""
    try:
        with urllib.request.urlopen(url, timeout=10) as response:
            response.read()
            status = response.status
    except urllib.error.HTTPError as exc:
        status = exc.code
        error = exc.read().decode("utf-8", errors="replace")
    except Exception as exc:  # noqa: BLE001
        error = str(exc)
    return {
        "status": status,
        "elapsedMs": round((time.perf_counter() - started) * 1000, 3),
        "error": error,
    }


def percentile(values: list[float], ratio: float) -> float:
    ordered = sorted(values)
    index = max(0, math.ceil(len(ordered) * ratio) - 1)
    return ordered[index]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default="http://127.0.0.1:8000")
    parser.add_argument("--requests", type=int, default=200)
    parser.add_argument("--concurrency", type=int, default=200)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    target = f"{args.base_url}/api/equipment?pageNo=1&pageSize=10"
    start_event = threading.Event()
    overall_started = time.perf_counter()

    with concurrent.futures.ThreadPoolExecutor(
        max_workers=args.concurrency
    ) as executor:
        futures = [
            executor.submit(query_once, target, start_event)
            for _ in range(args.requests)
        ]
        start_event.set()
        records = [future.result() for future in futures]

    wall_seconds = time.perf_counter() - overall_started
    durations = [record["elapsedMs"] for record in records]
    success_count = sum(record["status"] == 200 for record in records)
    average_ms = statistics.fmean(durations)
    p95_ms = percentile(durations, 0.95)
    max_ms = max(durations)
    success_rate = success_count / args.requests
    passed = success_rate == 1.0 and average_ms <= 2000 and p95_ms <= 3000

    result = {
        "generatedAt": datetime.now(timezone.utc).astimezone().isoformat(),
        "target": target,
        "requests": args.requests,
        "concurrency": args.concurrency,
        "successCount": success_count,
        "successRate": round(success_rate, 4),
        "averageMs": round(average_ms, 3),
        "p95Ms": round(p95_ms, 3),
        "maxMs": round(max_ms, 3),
        "wallSeconds": round(wall_seconds, 3),
        "threshold": {
            "successRate": 1.0,
            "averageMs": 2000,
            "p95Ms": 3000,
        },
        "passed": passed,
        "statusCounts": {
            str(status): sum(record["status"] == status for record in records)
            for status in sorted({record["status"] for record in records})
        },
        "errors": [record["error"] for record in records if record["error"]][:10],
    }

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(result, ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    raise SystemExit(main())
