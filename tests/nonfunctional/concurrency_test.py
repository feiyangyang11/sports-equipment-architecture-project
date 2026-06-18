import argparse
import concurrent.futures
import json
import threading
import urllib.error
import urllib.request
from datetime import datetime, timedelta, timezone
from pathlib import Path


def request_json(
    base_url: str,
    method: str,
    path: str,
    body: dict | None = None,
    token: str = "",
) -> tuple[int, object]:
    data = None
    headers = {}
    if body is not None:
        data = json.dumps(body).encode("utf-8")
        headers["Content-Type"] = "application/json"
    if token:
        headers["Authorization"] = f"Bearer {token}"

    request = urllib.request.Request(
        f"{base_url}{path}",
        data=data,
        headers=headers,
        method=method,
    )
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            text = response.read().decode("utf-8")
            return response.status, json.loads(text) if text else {}
    except urllib.error.HTTPError as exc:
        text = exc.read().decode("utf-8", errors="replace")
        try:
            payload: object = json.loads(text)
        except json.JSONDecodeError:
            payload = text
        return exc.code, payload


def require_status(actual: int, expected: int, operation: str) -> None:
    if actual != expected:
        raise RuntimeError(f"{operation}: expected {expected}, actual {actual}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--base-url", default="http://127.0.0.1:8000")
    parser.add_argument("--requests", type=int, default=20)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    student_status, student_login = request_json(
        args.base_url,
        "POST",
        "/api/login",
        {"username": "stu2026003", "password": "Student@123"},
    )
    require_status(student_status, 200, "student login")
    admin_status, admin_login = request_json(
        args.base_url,
        "POST",
        "/api/login",
        {"username": "admin02", "password": "Admin@123"},
    )
    require_status(admin_status, 200, "admin login")
    student_token = student_login["token"]
    admin_token = admin_login["token"]

    reservation_start = (datetime.now() + timedelta(days=60)).replace(
        hour=15, minute=0, second=0, microsecond=0
    )
    reservation_end = reservation_start + timedelta(hours=2)
    create_status, reservation = request_json(
        args.base_url,
        "POST",
        "/api/reservations",
        {
            "equipmentId": 1002,
            "reservationStartAt": reservation_start.strftime("%Y-%m-%d %H:%M:%S"),
            "reservationEndAt": reservation_end.strftime("%Y-%m-%d %H:%M:%S"),
            "quantity": 1,
            "requestNote": "T6 concurrency test",
        },
        student_token,
    )
    require_status(create_status, 201, "create reservation")
    reservation_id = reservation["id"]

    approve_status, _ = request_json(
        args.base_url,
        "POST",
        f"/api/admin/reservations/{reservation_id}/approve",
        {"reviewNote": "T6 concurrency approved"},
        admin_token,
    )
    require_status(approve_status, 200, "approve reservation")

    stock_status, equipment_before = request_json(
        args.base_url, "GET", "/api/equipment/1002"
    )
    require_status(stock_status, 200, "load stock before")
    stock_before = equipment_before["availableStock"]

    now = datetime.now().replace(microsecond=0)
    borrow_payload = {
        "reservationId": reservation_id,
        "borrowedAt": now.strftime("%Y-%m-%d %H:%M:%S"),
        "dueAt": (now + timedelta(hours=4)).strftime("%Y-%m-%d %H:%M:%S"),
    }
    start_event = threading.Event()

    def borrow_once() -> tuple[int, object]:
        start_event.wait()
        return request_json(
            args.base_url,
            "POST",
            "/api/admin/borrows",
            borrow_payload,
            admin_token,
        )

    with concurrent.futures.ThreadPoolExecutor(
        max_workers=args.requests
    ) as executor:
        futures = [executor.submit(borrow_once) for _ in range(args.requests)]
        start_event.set()
        responses = [future.result() for future in futures]

    successful = [payload for status, payload in responses if status == 201]
    status_counts = {
        str(status): sum(item_status == status for item_status, _ in responses)
        for status in sorted({status for status, _ in responses})
    }

    after_status, equipment_after = request_json(
        args.base_url, "GET", "/api/equipment/1002"
    )
    require_status(after_status, 200, "load stock after")
    stock_after = equipment_after["availableStock"]

    borrow_id = successful[0]["id"] if successful else 0
    restored_stock = stock_after
    return_status = 0
    if borrow_id:
        return_status, _ = request_json(
            args.base_url,
            "POST",
            f"/api/admin/borrows/{borrow_id}/return",
            {
                "returnedAt": (now + timedelta(minutes=1)).strftime(
                    "%Y-%m-%d %H:%M:%S"
                ),
                "returnNote": "T6 concurrency cleanup",
            },
            admin_token,
        )
        restored_status, restored_equipment = request_json(
            args.base_url, "GET", "/api/equipment/1002"
        )
        require_status(restored_status, 200, "load restored stock")
        restored_stock = restored_equipment["availableStock"]

    only_conflict_failures = all(
        status in (201, 409) for status, _ in responses
    )
    passed = (
        len(successful) == 1
        and stock_after == stock_before - 1
        and return_status == 200
        and restored_stock == stock_before
        and only_conflict_failures
    )

    result = {
        "generatedAt": datetime.now(timezone.utc).astimezone().isoformat(),
        "requests": args.requests,
        "reservationId": reservation_id,
        "successfulBorrows": len(successful),
        "statusCounts": status_counts,
        "stockBefore": stock_before,
        "stockAfter": stock_after,
        "stockRestored": restored_stock,
        "returnStatus": return_status,
        "onlyExpected201Or409": only_conflict_failures,
        "passed": passed,
        "criterion": (
            "exactly one 201, all other responses 409, stock decreases once "
            "and returns to the original value"
        ),
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
