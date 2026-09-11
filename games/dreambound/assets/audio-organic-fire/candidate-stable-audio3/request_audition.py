"""Anonymous audition through the official public Gradio endpoint.

One --item per invocation, at most the three declared requests. Existing request
records are never resubmitted: --resume only reconnects to the same event handle.
No credentials, paid API, account operation, model download or gated agreement.
Raw returned WAV bytes are retained; nothing is imported or edited afterward.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import time
from urllib.parse import urlparse

import requests

ROOT = Path(__file__).resolve().parent
SPACE = "stabilityai/stable-audio-3"
BASE = "https://stabilityai-stable-audio-3.hf.space"
MODEL = "stabilityai/stable-audio-3-small-sfx"
ITEMS = {
    "internal_ignition": dict(seed=46711,duration=3,prompt=
        "A flame ignites inside a confined cavity: a low muffled puff of air and rough turbulent burning with tiny irregular crackles. Close dry sound effect. No music, voice, synthesizer, beeps or metallic ringing."),
    "erupting_flare": dict(seed=46712,duration=2,prompt=
        "A powerful burst of pressurized flame erupts: a sudden deep air punch followed by a coarse rushing fire roar and a brief crackling tail. Close dry sound effect. No music, voice, synthesizer, beeps or metallic ringing."),
    "fireball_whoosh": dict(seed=46713,duration=2,prompt=
        "A dense ball of fire rushes rapidly past the listener, with a sharp turbulent air whoosh and a ragged burning tail. One fast pass. Close dry sound effect. No music, voice, synthesizer, beeps or metallic ringing."),
}


def now():
    return datetime.now(timezone.utc).isoformat()


def write(path, record):
    path.write_text(json.dumps(record,indent=2)+"\n",encoding="utf-8")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--item",choices=list(ITEMS),required=True)
    parser.add_argument("--resume",action="store_true")
    args=parser.parse_args()
    path=ROOT/(args.item+"-request.json")
    session=requests.Session()
    # Do not read .netrc, cached Hugging Face credentials or environment tokens.
    session.trust_env=False
    session.headers.update({"User-Agent":"Dreambound-Private-Audio-Audition/1.0"})
    if args.resume:
        record=json.loads(path.read_text(encoding="utf-8"))
        if not record.get("event_id") or record["status"] in ("complete","failed"):
            raise RuntimeError("Only a currently pending known event can be resumed")
    else:
        if path.exists():raise RuntimeError("Request record exists; refusing duplicate generation")
        space=session.get("https://huggingface.co/api/spaces/"+SPACE,timeout=25)
        space.raise_for_status(); space_data=space.json()
        if space_data.get("private") or space_data.get("gated") or space_data.get("disabled"):
            raise RuntimeError("Official Space is not anonymously public")
        if space_data.get("runtime",{}).get("stage")!="RUNNING":
            raise RuntimeError("Official Space is not running")
        info=session.get(BASE+"/gradio_api/info",timeout=25)
        info.raise_for_status(); endpoint=info.json()["named_endpoints"]["/infer"]
        if endpoint.get("api_visibility")!="public":raise RuntimeError("Inference API is not public")
        model=session.get("https://huggingface.co/api/models/"+MODEL,timeout=25)
        model.raise_for_status(); model_data=model.json()
        settings=dict(variant_key="small-sfx",steps=8,cfg_scale=1.0,sampler_type="pingpong",**ITEMS[args.item])
        record=dict(item=args.item,created_utc=now(),status="submitting",space=SPACE,
            space_url="https://huggingface.co/spaces/"+SPACE,host=BASE,space_revision=space_data["sha"],
            space_hardware=space_data["runtime"]["hardware"],model=MODEL,model_metadata_revision=model_data.get("sha"),
            model_license_tag=model_data.get("cardData",{}).get("license"),
            actual_loaded_model_revision="Not reported by demo; model metadata revision is not proof of loaded weight identity.",
            endpoint="/gradio_api/call/v2/infer",endpoint_visibility="public",settings=settings,
            anonymous=True,credentials_used=False,paid_service=False,local_processing="none",
            acceptance="unaccepted private audition",commercial_rights="not cleared for release",listened=False)
        write(path,record)
        try:
            response=session.post(BASE+record["endpoint"],json=settings,timeout=25)
        except requests.RequestException as error:
            record.update(status="submission_unknown",error=str(error),updated_utc=now())
            write(path,record);raise
        record["submission_http_status"]=response.status_code
        try:record["submission_response"]=response.json()
        except ValueError:record["submission_response"]=response.text[:4000]
        if response.status_code!=200 or not isinstance(record["submission_response"],dict) or not record["submission_response"].get("event_id"):
            record.update(status="failed",updated_utc=now());write(path,record)
            print(json.dumps(record,indent=2),flush=True);return
        record.update(status="queued",event_id=record["submission_response"]["event_id"],events=[])
        write(path,record)
    print(json.dumps(dict(item=args.item,status=record["status"],event_id=record["event_id"])),flush=True)
    url=BASE+"/gradio_api/call/infer/"+record["event_id"]
    event_type="message"
    start=time.monotonic()
    try:
        with session.get(url,stream=True,timeout=(20,35)) as stream:
            record["stream_http_status"]=stream.status_code
            stream.raise_for_status()
            for line in stream.iter_lines(decode_unicode=True):
                if line.startswith("event:"):event_type=line[6:].strip()
                if not line.startswith("data:"):continue
                raw=line[5:].strip()
                try:data=json.loads(raw)
                except ValueError:data=raw
                event=dict(type=event_type,data=data,observed_utc=now())
                if event_type!="heartbeat":record["events"].append(event)
                record.update(updated_utc=now(),status="running")
                if event_type=="error":
                    record.update(status="failed",error=data);write(path,record)
                    print(json.dumps(event),flush=True);return
                if event_type=="complete":
                    files=data if isinstance(data,list) else [data]
                    result=next(x for x in files if isinstance(x,dict) and x.get("url"))
                    download_url=result["url"]
                    if urlparse(download_url).hostname!=urlparse(BASE).hostname:
                        raise RuntimeError("Returned download uses another host; inspect before proceeding")
                    output=session.get(download_url,timeout=35);output.raise_for_status()
                    output_path=ROOT/(args.item+"-raw.wav")
                    output_path.write_bytes(output.content)
                    record.update(status="complete",output_file=output_path.name,
                        output_url=download_url,output_bytes=len(output.content),
                        output_sha256=hashlib.sha256(output.content).hexdigest(),wall_seconds=time.monotonic()-start)
                    write(path,record)
                    print(json.dumps({k:record[k] for k in ["item","status","output_file","output_bytes","output_sha256"]}),flush=True);return
                write(path,record)
    except requests.RequestException as error:
        # An observation timeout is not a terminal generation result.
        record.update(status="observation_lost",observation_error=str(error),updated_utc=now())
        write(path,record);raise
    record.update(status="observation_ended_without_terminal_event",updated_utc=now())
    write(path,record)


if __name__=="__main__":main()
