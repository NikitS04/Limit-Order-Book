#!/usr/bin/env python3
"""Generate a synthetic event stream for benchmarking the LOB engine.

Default mix is closer to a real order flow than an add-only stream: lots of
cancels, some modifies and aggressive market orders, with a drifting mid and
orders spread across a few hundred ticks.

  --mix NEW,CXL,MOD,MKT   percentages, must sum to 100 (default 45,40,10,5)
  --legacy                reproduce the original add-only generator
"""
import argparse
import random


def write_legacy(f, n, mid, rng):
    ts = 1
    oid = 1
    for i in range(n):
        side = 'B' if rng.random() < 0.5 else 'S'
        if i % 10 < 7:
            delta = rng.randint(5, 50)
            px = mid - delta if side == 'B' else mid + delta
        else:
            delta = rng.randint(0, 3)
            px = mid + (3 - delta) if side == 'B' else mid - (3 - delta)
        f.write(f"{ts},{side},NEW,{oid},{px},{rng.randint(1, 10)}\n")
        ts += 1
        oid += 1


def write_mixed(f, n, mid, rng, mix, depth):
    p_new, p_cxl, p_mod, p_mkt = (m / 100.0 for m in mix)
    # live: ids we believe are resting -> (side, px). The generator can't see
    # fills, so some cancels/modifies hit already-filled orders. The engine
    # rejects those, which is realistic (late cancels happen on real venues).
    live_ids = []
    live = {}
    oid = 1
    for ts in range(1, n + 1):
        if rng.random() < 0.01:
            mid += rng.choice((-1, 1))
        r = rng.random()
        if not live_ids or r < p_new:
            side = 'B' if rng.random() < 0.5 else 'S'
            # Most orders rest near the touch, a tail sits deeper in the book;
            # a few percent are aggressive and cross the spread.
            if rng.random() < 0.05:
                offset = -rng.randint(0, 3)
            else:
                offset = min(int(rng.expovariate(1 / 8)) + 1, depth)
            px = mid - offset if side == 'B' else mid + offset
            qty = rng.randint(1, 100)
            f.write(f"{ts},{side},NEW,{oid},{px},{qty}\n")
            live[oid] = (side, px)
            live_ids.append(oid)
            oid += 1
        elif r < p_new + p_cxl:
            i = rng.randrange(len(live_ids))
            cid = live_ids[i]
            live_ids[i] = live_ids[-1]
            live_ids.pop()
            side, px = live.pop(cid)
            f.write(f"{ts},{side},CXL,{cid},{px},0\n")
        elif r < p_new + p_cxl + p_mod:
            mid_id = live_ids[rng.randrange(len(live_ids))]
            side, px = live[mid_id]
            if rng.random() < 0.5:
                new_px = px  # size change at same price
            else:
                new_px = px + rng.choice((-2, -1, 1, 2))
            live[mid_id] = (side, new_px)
            f.write(f"{ts},{side},MOD,{mid_id},{new_px},{rng.randint(1, 100)}\n")
        else:
            side = 'B' if rng.random() < 0.5 else 'S'
            f.write(f"{ts},{side},MKT,{oid},0,{rng.randint(1, 200)}\n")
            oid += 1


def main():
    ap = argparse.ArgumentParser(description="Generate benchmark CSV for the LOB engine")
    ap.add_argument("--out", default="events.csv", help="output CSV path")
    ap.add_argument("-n", "--num", type=int, default=1_000_000, help="number of events")
    ap.add_argument("--mid", type=int, default=10_000, help="starting mid price (ticks)")
    ap.add_argument("--depth", type=int, default=200, help="max distance from mid for resting orders (ticks)")
    ap.add_argument("--mix", default="45,40,10,5", help="NEW,CXL,MOD,MKT percentages")
    ap.add_argument("--seed", type=int, default=1, help="PRNG seed")
    ap.add_argument("--legacy", action="store_true", help="original add-only generator")
    args = ap.parse_args()

    rng = random.Random(args.seed)
    with open(args.out, "w") as f:
        f.write("ts_ns,side,type,id,price,qty\n")
        if args.legacy:
            write_legacy(f, args.num, args.mid, rng)
        else:
            mix = [float(x) for x in args.mix.split(",")]
            if len(mix) != 4 or abs(sum(mix) - 100) > 1e-6:
                ap.error("--mix needs four percentages summing to 100")
            write_mixed(f, args.num, args.mid, rng, mix, args.depth)
    print(f"Wrote {args.num} events to {args.out}")


if __name__ == "__main__":
    main()
