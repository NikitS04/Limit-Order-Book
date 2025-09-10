#!/usr/bin/env python3
import argparse, random
def write_events(path: str, n: int, mid: int = 10_000, seed: int = 1):
    random.seed(seed)
    with open(path, "w") as f:
        f.write("ts_ns,side,type,id,price,qty\n")
        ts = 1; oid = 1
        for i in range(n):
            if i % 10 < 7:
                side = 'B' if random.random() < 0.5 else 'S'
                delta = random.randint(5, 50)
                px = mid - delta if side == 'B' else mid + delta
                qty = random.randint(1, 10)
                f.write(f"{ts},{side},NEW,{oid},{px},{qty}\n")
            else:
                side = 'B' if random.random() < 0.5 else 'S'
                delta = random.randint(0, 3)
                px = mid + (3 - delta) if side == 'B' else mid - (3 - delta)
                qty = random.randint(1, 10)
                f.write(f"{ts},{side},NEW,{oid},{px},{qty}\n")
            ts += 1; oid += 1
if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Generate stress-test CSV for LOB engine")
    ap.add_argument("--out", default="events.csv", help="output CSV path")
    ap.add_argument("-n", "--num", type=int, default=1_000_000, help="number of events")
    ap.add_argument("--mid", type=int, default=10_000, help="mid price")
    ap.add_argument("--seed", type=int, default=1, help="PRNG seed")
    args = ap.parse_args()
    write_events(args.out, args.num, args.mid, args.seed)
    print(f"Wrote {args.num} events to {args.out}")
