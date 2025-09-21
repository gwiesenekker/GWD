#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PDN "Luck" Analyzer (multi-game, RI/SI, blunder-aware, bands)

Parses a PDN file that may contain one or more games. Each game may include
engine annotations inside braces { } like:
  {bestmove score depth}  e.g., {12-18 -2 18}
or
  {only move}

For every game, the script computes:
- Max disadvantage (most negative eval)
- Time spent losing (fraction & longest span) w.r.t. threshold --tedge
- Severe losing counts (≤ -severe) & longest severe span
- Winning-side mirrors (num/frac/longest winning spans)
- Comeback magnitude to final eval
- Swinginess (max |Δ| and counts over thresholds)
- Lead changes
- Stability (mean, stdev)
- Forced-move context (total & how many occur within/adjacent to losing spans)
- Rescue Index (RI) and Squander Index (SI)
- Label with blunder-aware logic (BlunderLoss/BlunderWin, Chaotic*, etc.)
- Banding for RI/SI: Low/Medium/High written to CSV

Usage:
  python pdn_luck_analyzer.py <file.pdn> [--tdraw 10] [--tedge 20] [--severe 40] \
      [--swing 10,20,30,40,50] [--summary-csv out.csv] \
      [--high-threshold 1.0] [--low-threshold 0.4] \
      [--blunder-threshold 100] [--decisive-span 5]

Notes:
- Metrics are calculated over *scored* plies only (braces with a numeric score).
- "only move" plies are counted separately; for tunnel/adjacency we look at the
  nearest scored entries on either side.
"""

from __future__ import annotations
import argparse
import csv
import re
import statistics
from typing import List, Optional, Tuple, Dict, Any


def parse_headers(pdn: str) -> dict:
    return dict(re.findall(r'\[(\w+)\s+"([^"]*)"\]', pdn))


def parse_entries(pdn: str) -> List[dict]:
    """
    Returns a list of entries in chronological order. Each entry is a dict with:
      - 'score': Optional[int]  (None if "only move")
      - 'forced_only': bool     (True if "{only move}" seen)
    """
    entries: List[Dict[str, Any]] = []
    for raw in re.findall(r'\{([^}]*)\}', pdn, flags=re.DOTALL):
        content = raw.strip()
        low = content.lower()
        if "only move" in low:
            entries.append({"score": None, "forced_only": True})
            continue

        # Expect format like: "<move> <score> <depth>"
        # Be robust: read last two integer-like tokens as score and depth.
        toks = content.split()
        score: Optional[int] = None
        if len(toks) >= 2:
            int_like: List[str] = []
            for tok in reversed(toks):
                try:
                    _ = int(tok)
                    int_like.append(tok)
                    if len(int_like) == 2:
                        break
                except ValueError:
                    continue
            if len(int_like) >= 1:
                try:
                    score = int(int_like[-1])  # nearer to the move should be the score
                except ValueError:
                    score = None

        entries.append({"score": score, "forced_only": False})
    return entries


def consecutive_spans(flags: List[bool]) -> Tuple[int, List[int]]:
    """Return (longest_span, all_spans_lengths) for True-runs in boolean list."""
    longest = 0
    spans: List[int] = []
    cur = 0
    for f in flags:
        if f:
            cur += 1
        else:
            if cur > 0:
                longest = max(longest, cur)
                spans.append(cur)
                cur = 0
    if cur > 0:
        longest = max(longest, cur)
        spans.append(cur)
    return longest, spans


def analyze(entries: List[dict], tdraw: int, tedge: int, severe: int, swing_thresholds: List[int]) -> dict:
    scores: List[int] = [e["score"] for e in entries if e["score"] is not None]
    n_scored = len(scores)
    n_forced = sum(1 for e in entries if e["forced_only"])

    if n_scored == 0:
        return {
            "n_scored": 0,
            "n_forced": n_forced,
            "error": "No numeric scores found in PDN braces."
        }

    # Core aggregates
    min_eval = min(scores)  # most negative = max disadvantage
    max_eval = max(scores)  # most positive
    final_eval = scores[-1]
    mean_eval = statistics.mean(scores)
    stdev_eval = statistics.pstdev(scores) if n_scored > 1 else 0.0

    # Losing / winning / severe spans over scored plies
    losing_flags = [s <= -tedge for s in scores]
    winning_flags = [s >= +tedge for s in scores]
    severe_flags  = [s <= -severe for s in scores]

    num_losing = sum(losing_flags)
    frac_losing = num_losing / n_scored
    longest_losing_span, losing_spans = consecutive_spans(losing_flags)

    num_winning = sum(winning_flags)
    frac_winning = num_winning / n_scored
    longest_winning_span, winning_spans = consecutive_spans(winning_flags)

    num_severe = sum(severe_flags)
    frac_severe = num_severe / n_scored
    longest_severe_span, severe_spans = consecutive_spans(severe_flags)

    # Comeback magnitude (how far from min to the final eval)
    comeback_magnitude = max(0, final_eval - min_eval)

    # Swinginess on scored plies (ignore "only move" gaps)
    deltas = [scores[i+1] - scores[i] for i in range(n_scored - 1)]
    max_swing = max((abs(d) for d in deltas), default=0)
    max_swing_idx = max(range(len(deltas)), key=lambda i: abs(deltas[i])) if deltas else None
    max_pos_swing = max((d for d in deltas if d > 0), default=0)
    max_neg_swing = min((d for d in deltas if d < 0), default=0)
    swing_counts = {thr: sum(1 for d in deltas if abs(d) > thr) for thr in swing_thresholds}

    # Lead changes: sign flips ignoring zeros
    def sgn(x: int) -> int:
        return -1 if x < 0 else (1 if x > 0 else 0)

    lead_changes = 0
    prev_sign = None
    for s in scores:
        cur_sign = sgn(s)
        if prev_sign in (-1, 1) and cur_sign in (-1, 1) and cur_sign != prev_sign:
            lead_changes += 1
        if cur_sign != 0:
            prev_sign = cur_sign

    # Forced move context
    idx_scored = [i for i, e in enumerate(entries) if e["score"] is not None]
    idx_to_scored_pos = {idx: pos for pos, idx in enumerate(idx_scored)}  # entry-index -> position in scores[]

    forced_total = 0
    forced_adjacent_losing = 0
    forced_in_losing_tunnel = 0

    for i, e in enumerate(entries):
        if not e["forced_only"]:
            continue
        forced_total += 1

        # nearest previous/next scored positions
        left_pos = None
        right_pos = None

        for j in range(i - 1, -1, -1):
            if entries[j]["score"] is not None:
                left_entry_index = j
                left_pos = idx_to_scored_pos[left_entry_index]
                break

        for j in range(i + 1, len(entries)):
            if entries[j]["score"] is not None:
                right_entry_index = j
                right_pos = idx_to_scored_pos[right_entry_index]
                break

        left_losing = (left_pos is not None and losing_flags[left_pos])
        right_losing = (right_pos is not None and losing_flags[right_pos])

        if left_losing or right_losing:
            forced_adjacent_losing += 1
        if left_losing and right_losing:
            forced_in_losing_tunnel += 1

    return {
        "n_scored": n_scored,
        "n_forced": n_forced,
        "scores": scores,
        "min_eval": min_eval,
        "max_eval": max_eval,
        "final_eval": final_eval,
        "mean_eval": mean_eval,
        "stdev_eval": stdev_eval,
        "losing_flags": losing_flags,
        "winning_flags": winning_flags,
        "num_losing": num_losing,
        "frac_losing": frac_losing,
        "longest_losing_span": longest_losing_span,
        "num_winning": num_winning,
        "frac_winning": frac_winning,
        "longest_winning_span": longest_winning_span,
        "num_severe": num_severe,
        "frac_severe": frac_severe,
        "longest_severe_span": longest_severe_span,
        "comeback_magnitude": comeback_magnitude,
        "deltas": deltas,
        "max_swing": max_swing,
        "max_swing_idx": max_swing_idx,
        "max_pos_swing": max_pos_swing,
        "max_neg_swing": max_neg_swing,
        "swing_counts": swing_counts,
        "lead_changes": lead_changes,
        "forced_total": forced_total,
        "forced_adjacent_losing": forced_adjacent_losing,
        "forced_in_losing_tunnel": forced_in_losing_tunnel,
    }


def compute_rescue_index(metrics: dict, tedge: int,
                         A: int = 60, B: int = 8, C: int = 50,
                         w1: float = 0.4, w2: float = 0.25,
                         w3: float = 0.2, w4: float = 0.15) -> float:
    """
    Rescue Index (RI): how far you fell behind and fought back.
      - w1 * max(0, -min_eval) / A
      - w2 * frac_losing
      - w3 * (longest_losing_span / B)
      - w4 * (max_swing / C)
    """
    min_eval = metrics.get("min_eval", 0)
    frac_losing = metrics.get("frac_losing", 0.0)
    longest_losing_span = metrics.get("longest_losing_span", 0)
    max_swing = metrics.get("max_swing", 0)

    term1 = max(0, -min_eval) / float(A)
    term2 = float(frac_losing)
    term3 = (longest_losing_span / float(B)) if B else 0.0
    term4 = (max_swing / float(C)) if C else 0.0
    return w1*term1 + w2*term2 + w3*term3 + w4*term4


def compute_squander_index(metrics: dict,
                           A: int = 60, B: int = 8, C: int = 50,
                           w1: float = 0.4, w2: float = 0.25,
                           w3: float = 0.2, w4: float = 0.15) -> float:
    """
    Squander Index (SI): mirror metric for throwing away winning positions.
      - w1 * max(0, +max_eval) / A
      - w2 * frac_winning
      - w3 * (longest_winning_span / B)
      - w4 * (max_swing / C)
    """
    max_eval = metrics.get("max_eval", 0)
    frac_winning = metrics.get("frac_winning", 0.0)
    longest_winning_span = metrics.get("longest_winning_span", 0)
    max_swing = metrics.get("max_swing", 0)

    term1 = max(0, +max_eval) / float(A)
    term2 = float(frac_winning)
    term3 = (longest_winning_span / float(B)) if B else 0.0
    term4 = (max_swing / float(C)) if C else 0.0
    return w1*term1 + w2*term2 + w3*term3 + w4*term4


def band(value: float, lo: float, hi: float) -> str:
    """Return 'Low', 'Medium', 'High' for a numeric value given lo/hi thresholds."""
    try:
        v = float(value)
    except Exception:
        return "NA"
    if v <= lo:
        return "Low"
    if v >= hi:
        return "High"
    return "Medium"


def label_game(result: str,
               metrics: dict,
               hi: float, lo: float,
               blunder_threshold: int, decisive_span: int,
               tedge: int) -> Tuple[str, str, str, str]:
    """
    Return (label, reason, RI_band, SI_band) using blunder-aware logic and RI/SI context.
    """
    scores = metrics["scores"]
    deltas = metrics["deltas"]
    losing_flags = metrics["losing_flags"]
    winning_flags = metrics["winning_flags"]
    lead_changes = metrics["lead_changes"]

    # Helper: longest consecutive run of True starting at index start (inclusive)
    def run_length(flags: List[bool], start: int) -> int:
        r = 0
        for k in range(start, len(flags)):
            if flags[k]:
                r += 1
            else:
                break
        return r

    ri = compute_rescue_index(metrics, tedge)
    si = compute_squander_index(metrics)
    ri_band = band(ri, lo, hi)
    si_band = band(si, lo, hi)

    # Largest negative and positive single-PLY swings and indices
    max_neg = metrics.get("max_neg_swing", 0)
    max_pos = metrics.get("max_pos_swing", 0)
    neg_idx = None
    pos_idx = None
    for i, d in enumerate(deltas):
        if d == max_neg and max_neg < 0:
            neg_idx = i
            break
    for i, d in enumerate(deltas):
        if d == max_pos and max_pos > 0:
            pos_idx = i
            break

    res = (result or "").strip()

    # Detect decisive blunders first
    if neg_idx is not None and -max_neg >= blunder_threshold:
        losing_after = run_length(losing_flags, neg_idx + 1)
        if losing_after >= decisive_span and res != "1-1":
            return "BlunderLoss", f"-{abs(max_neg)} swing at ply {neg_idx+1} -> {losing_after}-ply losing span", ri_band, si_band
    if pos_idx is not None and max_pos >= blunder_threshold:
        winning_after = run_length(winning_flags, pos_idx + 1)
        if winning_after >= decisive_span and res != "1-1":
            return "BlunderWin", f"+{max_pos} swing at ply {pos_idx+1} -> {winning_after}-ply winning span", ri_band, si_band

    # Chaotic requires BOTH bands High, substantial spans both sides, and multiple lead changes
    if (ri_band == "High" and si_band == "High" and
        metrics["longest_losing_span"] >= decisive_span and
        metrics["longest_winning_span"] >= decisive_span and
        lead_changes >= 2):
        if res == "1-1":
            return "ChaoticDraw", "Alternating winning/losing spans with multiple lead changes", ri_band, si_band
        else:
            return "ChaoticGame", "Alternating winning/losing spans with multiple lead changes", ri_band, si_band

    # Result-specific interpretations
    if res == "1-1":  # Draw
        if ri_band == "High" and si_band == "Low":
            return "SavedDraw", "High RI, low SI", ri_band, si_band
        if si_band == "High" and ri_band == "Low":
            return "MissedWin", "High SI, low RI", ri_band, si_band
        return "QuietDraw", "Both RI and SI not extreme", ri_band, si_band

    # Non-draws (loss/win)
    # For losses, high RI is expected; choose between HadChances or BehindMostOfGame
    if res in ("0-2", "2-0"):
        # If had significant winning spans (SI high), note HadChances (for losses) or ControlledWin (for wins)
        if res == "0-2":  # loss
            if si_band == "High" and ri_band != "High":
                return "HadChances", "Significant winning spans before result slipped", ri_band, si_band
            if ri_band == "High":
                return "BehindMostOfGame", "Consistently behind; high RI expected in losses", ri_band, si_band
            return "ControlledGame", "No big alternations or decisive blunder detected", ri_band, si_band
        else:  # win
            if ri_band == "High":
                return "ComebackWin", "High RI suggests comeback", ri_band, si_band
            if si_band == "High":
                return "BlunderWin", "Significant winning spans, no big setbacks", ri_band, si_band
            return "ControlledWin", "Stable win without large swings", ri_band, si_band

    # Fallback
    return "ControlledGame", "Default classification", ri_band, si_band


def pretty_print(headers: dict, params: dict, metrics: dict, label: str, reason: str, ri_band: str, si_band: str) -> None:
    print("=== PDN Luck Analysis ===")
    if headers:
        print(f"White : {headers.get('White', 'Unknown')}")
        print(f"Black : {headers.get('Black', 'Unknown')}")
        print(f"Result: {headers.get('Result', 'Unknown')}")
        print()
    print(f"Parameters: tdraw={params['tdraw']} | tedge={params['tedge']} | severe={params['severe']} | swing={params['swing_thresholds']}")
    print()

    if metrics.get("error"):
        print(f"ERROR: {metrics['error']}")
        return

    print(f"Scored plies: {metrics['n_scored']}   |   'Only move' plies: {metrics['n_forced']}")
    print()
    print(f"Max disadvantage (most negative eval): {metrics['min_eval']}")
    print(f"Max advantage (most positive eval):    {metrics['max_eval']}")
    print(f"Final eval: {metrics['final_eval']}   |   Comeback magnitude: {metrics['comeback_magnitude']}")
    print(f"Mean eval: {metrics['mean_eval']:.2f}   |   Stdev: {metrics['stdev_eval']:.2f}")
    print()
    print(f"Time losing (≤ -tedge):   {metrics['num_losing']}/{metrics['n_scored']} "
          f"({metrics['frac_losing']*100:.1f}%)   |   Longest losing span: {metrics['longest_losing_span']}")
    print(f"Time winning (≥ +tedge):  {metrics['num_winning']}/{metrics['n_scored']} "
          f"({metrics['frac_winning']*100:.1f}%)   |   Longest winning span: {metrics['longest_winning_span']}")
    print(f"Severe losing (≤ -severe): {metrics['num_severe']}/{metrics['n_scored']} "
          f"({metrics['frac_severe']*100:.1f}%)   |   Longest severe span: {metrics['longest_severe_span']}")
    print()
    print(f"Max single‑ply swing |Δ|: {metrics['max_swing']}")
    for thr in sorted(metrics["swing_counts"].keys()):
        print(f"  Swings > {thr:2d}: {metrics['swing_counts'][thr]}")
    print(f"Lead changes (sign flips, zeros ignored): {metrics['lead_changes']}")
    print()
    # RI / SI
    ri = compute_rescue_index(metrics, params['tedge'])
    si = compute_squander_index(metrics)
    print(f"Rescue Index (RI):   {ri:.3f} [{ri_band}]")
    print(f"Squander Index (SI): {si:.3f} [{si_band}]")
    print("  RI = 0.40*max(0, -min_eval)/60 + 0.25*frac_losing + 0.20*(longest_losing_span/8) + 0.15*(max_swing/50)")
    print("  SI = 0.40*max(0, +max_eval)/60 + 0.25*frac_winning + 0.20*(longest_winning_span/8) + 0.15*(max_swing/50)")
    print()
    print(f"Label: {label}")
    if reason:
        print(f"Reason: {reason}")
    print()
    print("Forced-move context:")
    print(f"  Total 'only move' plies: {metrics['forced_total']}")
    print(f"  Adjacent to losing (prev OR next scored ≤ -tedge): {metrics['forced_adjacent_losing']}")
    print(f"  In losing tunnel (prev AND next scored ≤ -tedge):  {metrics['forced_in_losing_tunnel']}")


def split_games(pdn_text: str) -> List[str]:
    """Split a PDN file into individual game texts by [Event "..."] headers."""
    parts = re.split(r'(?=^\[Event\s+")', pdn_text, flags=re.MULTILINE)
    return [p.strip() for p in parts if p.strip()]


def write_summary_csv(path: str, summaries: List[dict], fieldnames: List[str]) -> None:
    with open(path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for row in summaries:
            row = row.copy()
            # Round floats to 3 decimals and stringify
            for k, v in list(row.items()):
                if isinstance(v, float):
                    row[k] = f"{v:.3f}"
            if isinstance(row.get("swing_counts"), dict):
                row["swing_counts"] = "; ".join(f">{k}:{v}" for k, v in sorted(row["swing_counts"].items()))
            # Drop bulky arrays if present
            for k in ("scores", "losing_flags", "winning_flags", "deltas"):
                row.pop(k, None)
            writer.writerow(row)


def main():
    ap = argparse.ArgumentParser(description="Analyze PDN games for 'lucky draw' signals using engine evals in braces.")
    ap.add_argument("pdn_file", help="Path to PDN file (may contain multiple games)")
    ap.add_argument("--tdraw", type=int, default=10, help="Drawing band threshold (|eval| ≤ tdraw) [default: 10]")
    ap.add_argument("--tedge", type=int, default=20, help="Losing threshold (eval ≤ -tedge) [default: 20]")
    ap.add_argument("--severe", type=int, default=40, help="Severe losing threshold (eval ≤ -severe) [default: 40]")
    ap.add_argument("--swing", type=str, default="10,20,30,40,50", help="Comma-separated swing thresholds [default: 10,20,30,40,50]")
    ap.add_argument("--summary-csv", type=str, default=None, help="Write per-game summary to CSV file")
    ap.add_argument("--high-threshold", type=float, default=1.0, help="High RI/SI threshold for labeling [default: 1.0]")
    ap.add_argument("--low-threshold", type=float, default=0.4, help="Low RI/SI threshold for labeling [default: 0.4]")
    ap.add_argument("--blunder-threshold", type=int, default=100, help="One‑ply swing magnitude to consider a blunder [default: 100]")
    ap.add_argument("--decisive-span", type=int, default=5, help="Min consecutive plies to count as a decisive span [default: 5]")
    args = ap.parse_args()

    swing_thresholds: List[int] = []
    for p in args.swing.split(","):
        p = p.strip()
        if p:
            try:
                swing_thresholds.append(int(p))
            except ValueError:
                raise SystemExit(f"Invalid --swing token: {p!r} (must be integers)")

    with open(args.pdn_file, "r", encoding="utf-8", errors="ignore") as f:
        pdn_text = f.read()

    games = split_games(pdn_text)
    print(f"Found {len(games)} game(s) in {args.pdn_file}\n")

    summaries: List[dict] = []
    for i, game_text in enumerate(games, start=1):
        headers = parse_headers(game_text)
        entries = parse_entries(game_text)
        metrics = analyze(entries, args.tdraw, args.tedge, args.severe, swing_thresholds)

        print("="*80)
        print(f"Game {i}")
        print("="*80)

        # Determine label
        label, reason, ri_band, si_band = label_game(headers.get("Result", ""), metrics,
                                                     args.high_threshold, args.low_threshold,
                                                     args.blunder_threshold, args.decisive_span,
                                                     args.tedge)

        pretty_print(headers, {
            "tdraw": args.tdraw,
            "tedge": args.tedge,
            "severe": args.severe,
            "swing_thresholds": swing_thresholds,
        }, metrics, label, reason, ri_band, si_band)

        # Build summary row
        ri = compute_rescue_index(metrics, args.tedge)
        si = compute_squander_index(metrics)
        row = {
            "Game": i,
            "White": headers.get("White", ""),
            "Black": headers.get("Black", ""),
            "Result": headers.get("Result", ""),
            **{k: v for k, v in metrics.items() if k in (
                "n_scored","n_forced",
                "min_eval","max_eval","final_eval","mean_eval","stdev_eval",
                "num_losing","frac_losing","longest_losing_span",
                "num_winning","frac_winning","longest_winning_span",
                "num_severe","frac_severe","longest_severe_span",
                "comeback_magnitude","max_swing","lead_changes",
                "forced_total","forced_adjacent_losing","forced_in_losing_tunnel",
                "swing_counts"
            )},
            "rescue_index": ri,
            "squander_index": si,
            "RI_band": ri_band,
            "SI_band": si_band,
            "label": label
        }
        summaries.append(row)

    if args.summary_csv:
        fieldnames = ["Game", "White", "Black", "Result",
                      "n_scored", "n_forced",
                      "min_eval", "max_eval", "final_eval", "mean_eval", "stdev_eval",
                      "num_losing", "frac_losing", "longest_losing_span",
                      "num_winning", "frac_winning", "longest_winning_span",
                      "num_severe", "frac_severe", "longest_severe_span",
                      "comeback_magnitude", "max_swing", "lead_changes",
                      "forced_total", "forced_adjacent_losing", "forced_in_losing_tunnel",
                      "swing_counts", "rescue_index", "squander_index", "RI_band", "SI_band", "label"]
        write_summary_csv(args.summary_csv, summaries, fieldnames)
        print(f"\nWrote summary CSV: {args.summary_csv}")


if __name__ == "__main__":
    main()
