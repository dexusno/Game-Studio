"""Encode the rendered study, then verify container/stream dimensions and duration."""
import hashlib
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parent
frames = root / "raw" / "frames"
paths = [frames / f"frame-{i:04}.png" for i in range(1, 193)]
assert all(p.is_file() for p in paths), "Render all 192 frames first"
movie = root / "camera-motion.mp4"
subprocess.run([
    "ffmpeg", "-hide_banner", "-y", "-framerate", "24", "-start_number", "1",
    "-i", str(frames / "frame-%04d.png"), "-c:v", "libx264", "-preset", "medium",
    "-crf", "20", "-pix_fmt", "yuv420p", "-movflags", "+faststart", "-an", str(movie),
], check=True)
result = subprocess.run([
    "ffprobe", "-v", "error", "-show_entries",
    "stream=codec_name,width,height,r_frame_rate,nb_frames:format=duration,size",
    "-of", "json", str(movie),
], check=True, capture_output=True, text=True)
data = json.loads(result.stdout)
stream = data["streams"][0]
assert (stream["width"], stream["height"], stream["nb_frames"]) == (960, 540, "192")
assert data["format"]["duration"] == "8.000000"
data["sha256"] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest()
                  for name in ["camera-motion.mp4", "preparation.png", "action-impact.png", "build_scene.py"]}
data["interpretation"] = "Scripted art study; no gameplay, physical simulation, engine import or owner acceptance established."
(root / "review-metadata.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
print(json.dumps(data, indent=2))
