"""Measure the opt-in actual-AI capture and encode its consecutive frames.

These measurements expose contact/coverage errors; they do not grade animation
quality. Keep raw frames in ignored local storage and visually review sequences.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import shutil
import struct
import subprocess
from collections import Counter
from pathlib import Path
from PIL import Image


def distribution(values):
    values = sorted(values)
    if not values:
        return {"samples": 0}
    return {"samples": len(values), "median": values[len(values) // 2],
            "p95": values[min(len(values) - 1, int(len(values) * .95))], "max": values[-1]}


def vector(row, prefix):
    return tuple(float(row[prefix + axis]) for axis in ("x", "y", "z"))


def distance(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--video", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    root = args.capture.resolve()
    capture = json.loads((root / "Capture.json").read_text(encoding="utf-8-sig"))
    with (root / "Frames.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.DictReader(stream))
    frames = sorted(root.glob("Frame_*.png"))
    if not frames or not rows:
        raise SystemExit("The capture has no frames or trace samples to review.")
    with frames[0].open("rb") as stream:
        dimensions = list(struct.unpack(">II", stream.read(24)[16:24]))
    gaps = [i for i, row in enumerate(rows) if int(row["frame"]) != i or not (root / f"Frame_{i:05}.png").is_file()]
    deltas = [float(b["simulation_seconds"]) - float(a["simulation_seconds"]) for a, b in zip(rows, rows[1:])]
    cadence_ok = bool(deltas) and all(abs(dt - 1 / 30) < .0001 for dt in deltas)
    report = {"capture": capture, "source_frames_csv_sha256": hashlib.sha256((root / "Frames.csv").read_bytes()).hexdigest(),
              "actual_pngs": len(frames), "actual_dimensions": dimensions, "csv_samples": len(rows), "missing_or_misnumbered_frames": gaps,
              "consecutive_30hz_simulation": cadence_ok, "sample_interval_seconds": distribution(deltas),
              "scope": "Measured bone/target positions and coverage, not a visual quality grade or hardware FPS benchmark."}
    report["phase_samples"] = dict(Counter(row["phase"] for row in rows))
    # A complete CSV still cannot establish visual coverage if the camera was
    # inside a wall. Report blank renders separately from contact measurements.
    report["near_black_frames"] = []
    for frame in frames:
        with Image.open(frame) as picture:
            if max(channel[1] for channel in picture.convert("RGB").getextrema()) < 12:
                report["near_black_frames"].append(int(frame.stem.split("_")[-1]))
    report["camera_obstruction_adjusted_samples"] = sum(row.get("camera_adjusted") == "1" for row in rows)
    transitions = []
    for a, b in zip(rows, rows[1:]):
        if b["phase"] != a["phase"]:
            transitions.append({"time": float(b["simulation_seconds"]), "from": a["phase"], "to": b["phase"], "frame": int(b["frame"])})
    report["phase_transitions"] = transitions
    plants = []
    for previous, row in zip(rows, rows[1:]):
        for side in ("left", "right"):
            if row[side + "_planted"] == "1" and previous[side + "_planted"] == "0" and row["phase"] == "Approach":
                plants.append({"frame": int(row["frame"]), "side": side,
                               "time": float(row["simulation_seconds"]), "speed_cm_s": float(row["speed_cm_s"])})
    report["approach_foot_plants"] = plants
    steady_intervals = []
    for previous, event in zip(plants, plants[1:]):
        between = rows[previous["frame"]:event["frame"] + 1]
        if previous["side"] != event["side"] and min(float(row["speed_cm_s"]) for row in between) > 100:
            speeds = [float(row["speed_cm_s"]) for row in between]
            if all(row["phase"] == "Approach" for row in between) and max(speeds) < min(speeds) * 1.1:
                steady_intervals.append({"frame": event["frame"], "seconds": event["time"] - previous["time"],
                                         "speed_cm_s": sum(speeds) / len(speeds)})
    report["steady_alternating_plant_intervals"] = steady_intervals
    for side in ("left", "right"):
        prefix = side + "_"
        stance = [row for row in rows if row[prefix + "planted"] == "1" and row["phase"] != "Dead"]
        slip, anchor_moves, peaks = [], [], []
        for a, b in zip(rows, rows[1:]):
            if b["phase"] == "Dead" or a[prefix + "planted"] != "1" or b[prefix + "planted"] != "1":
                continue
            anchor_delta = distance(vector(a, prefix + "target_"), vector(b, prefix + "target_"))
            if anchor_delta > .1:
                anchor_moves.append({"time": float(b["simulation_seconds"]), "cm": anchor_delta, "frame": int(b["frame"])})
                continue
            moved = distance(vector(a, prefix + "foot_"), vector(b, prefix + "foot_"))
            slip.append(moved)
            if moved > 1:
                peaks.append({"time": float(b["simulation_seconds"]), "cm": moved, "phase": b["phase"], "frame": int(b["frame"])})
        report[side + "_contact"] = {
            "planted_reach_error_cm": distribution([float(row[prefix + "error_cm"]) for row in stance]),
            "planted_frame_drift_cm": distribution(slip),
            "anchor_changes_while_marked_planted": anchor_moves,
            "largest_drift_frames": sorted(peaks, key=lambda item: item["cm"], reverse=True)[:12]}
    report["speed_cm_s"] = distribution([float(row["speed_cm_s"]) for row in rows if row["phase"] != "Dead"])
    report["moving_samples_over_40_cm_s"] = sum(float(row["speed_cm_s"]) > 40 for row in rows if row["phase"] != "Dead")
    report["stationary_samples_under_5_cm_s"] = sum(float(row["speed_cm_s"]) < 5 for row in rows if row["phase"] != "Dead")
    report["player_damaged_samples"] = [{"time": float(row["simulation_seconds"]), "phase": row["phase"], "health": float(row["player_health"])} for row in rows if float(row["player_health"]) < 100]
    report["creature_health_events"] = [{"time": float(b["simulation_seconds"]), "health": float(b["health"]), "frame": int(b["frame"])} for a, b in zip(rows, rows[1:]) if b["health"] != a["health"]]
    if "right_hand_x" in rows[0]:
        contacts = [row for row in rows if row["attack"] == "Swing" and row["phase"] == "Attack" and .14 <= float(row["attack_elapsed"]) <= .28]
        def forward_distance(row, start):
            return sum((end - begin) * facing for end, begin, facing in zip(vector(row, "right_hand_"), start, vector(row, "facing_")))
        report["melee_active_window_right_wrist"] = [{"frame": int(row["frame"]), "time": float(row["simulation_seconds"]),
            "ahead_of_actor_cm": forward_distance(row, vector(row, "")),
            "ahead_of_shoulder_cm": forward_distance(row, vector(row, "right_shoulder_"))} for row in contacts]
        report["initial_pelvis_height_above_actor_cm"] = [{"frame": int(row["frame"]), "time": float(row["simulation_seconds"]),
            "height": float(row["pelvis_z"]) - float(row["z"])} for row in rows[:60]]
        pelvis_changes = []
        for previous, row in zip(rows, rows[1:]):
            if previous["phase"] == row["phase"] == "Approach" and float(row["speed_cm_s"]) > 100:
                change = (float(row["pelvis_z"]) - float(row["z"])) - (float(previous["pelvis_z"]) - float(previous["z"]))
                pelvis_changes.append({"frame": int(row["frame"]), "relative_height_change_cm": change})
        report["largest_moving_pelvis_height_changes"] = sorted(pelvis_changes, key=lambda event: abs(event["relative_height_change_cm"]), reverse=True)[:12]
    if "left_hand_error_cm" in rows[0]:
        slam_contact = [row for row in rows if row["attack"] == "Slam" and row["phase"] == "Attack" and float(row["attack_elapsed"]) >= .155]
        report["slam_committed_wrist_target_error_cm"] = {
            side: distribution([float(row[side + "_hand_error_cm"]) for row in slam_contact])
            for side in ("left", "right")}
    report["projectile_count_increases"] = [{"time": float(b["simulation_seconds"]), "phase": b["phase"], "count": int(b["projectiles"])} for a, b in zip(rows, rows[1:]) if int(b["projectiles"]) > int(a["projectiles"])]
    complete_frames = not gaps and len(frames) == len(rows) == capture["frames"] and cadence_ok and capture["complete"] and not capture["missed_readbacks"]
    report["complete_consecutive_capture"] = complete_frames
    if args.video:
        if not complete_frames:
            raise SystemExit("Refusing a 30fps encoding: capture is incomplete or does not contain consecutive actual simulation frames.")
        encoder = shutil.which("ffmpeg")
        if not encoder:
            raise SystemExit("ffmpeg is required to encode the review clip.")
        args.video.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run([encoder, "-hide_banner", "-loglevel", "error", "-y", "-framerate", "30", "-i", str(root / "Frame_%05d.png"),
                        "-c:v", "libx264", "-crf", "18", "-preset", "fast", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(args.video)], check=True)
        report["video"] = {"file": args.video.name, "sha256": hashlib.sha256(args.video.read_bytes()).hexdigest(),
                           "frames_per_second": 30, "source_frames_repeated": False, "audio": False}
    output = args.report or root / "Review.json"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("complete_consecutive_capture", "actual_pngs", "phase_samples", "speed_cm_s", "left_contact", "right_contact")}, indent=2))


if __name__ == "__main__":
    main()
