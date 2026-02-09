import os
os.environ["TORCH_CPP_LOG_LEVEL"] = "ERROR"
import re
import json
from typing import List, Dict, Any, Tuple

import cv2
import numpy as np

import torch
torch.backends.nnpack.enabled = False
from ultralytics import YOLO

from Logger import Logger, LogLevel

# -------------------------
# 1) Grupowanie zdjęć w eventy (z Twojej struktury)
# -------------------------


global minuteDir
minuteDir = ""

def parseSecondFromFilename(filename: str) -> int | None:
    m = re.match(r"^(\d{2})\D", filename)
    if not m:
        return None
    sec = int(m.group(1))
    return sec if 0 <= sec <= 59 else None

def groupImagesIntoEvents(minuteDir: str, gapSeconds: int = 2) -> List[List[str]]:
    files = [f for f in os.listdir(minuteDir) if f.lower().endswith((".jpg", ".jpeg", ".png"))]
    items: List[Tuple[int, str]] = []

    for f in files:
        sec = parseSecondFromFilename(f)
        if sec is not None:
            items.append((sec, f))

    items.sort(key=lambda x: x[0])

    events: List[List[str]] = []
    current: List[str] = []
    lastSec: int | None = None

    for sec, f in items:
        path = os.path.join(minuteDir, f)
        if lastSec is None:
            current = [path]
        else:
            if sec - lastSec > gapSeconds:
                events.append(current)
                current = [path]
            else:
                current.append(path)
        lastSec = sec

    if current:
        events.append(current)

    return events

# -------------------------
# 2) ANALIZA OBRAZU (OpenCV heurystyki)
# -------------------------

def diffMask(prevFrame, currFrame, diffThresh=25):
    prevGray = cv2.cvtColor(prevFrame, cv2.COLOR_BGR2GRAY)
    currGray = cv2.cvtColor(currFrame, cv2.COLOR_BGR2GRAY)

    diff = cv2.absdiff(prevGray, currGray)
    diff = cv2.GaussianBlur(diff, (5, 5), 0)
    _, mask = cv2.threshold(diff, diffThresh, 255, cv2.THRESH_BINARY)
    return mask, prevGray, currGray

def rainFeatures(frames):
    """
    Deszcz/śnieg = dużo drobnych, rozproszonych zmian.
    """
    if len(frames) < 2:
        return 0.0, 0.0, 0.0, 0.0

    scores = []
    changeRatios = []
    smallRatios = []
    bigCounts = []

    for i in range(1, len(frames)):
        mask, _, _ = diffMask(frames[i - 1], frames[i], diffThresh=25)
        changeRatio = float(mask.mean() / 255.0)

        numLabels, _, stats, _ = cv2.connectedComponentsWithStats(mask, connectivity=8)
        areas = stats[1:, cv2.CC_STAT_AREA]

        if len(areas) == 0:
            scores.append(0.0)
            changeRatios.append(changeRatio)
            smallRatios.append(0.0)
            bigCounts.append(0.0)
            continue

        smallCount = int(np.sum((areas >= 2) & (areas <= 80)))
        bigCount = int(np.sum(areas >= 300))
        smallRatio = float(smallCount / max(1, len(areas)))

        score = (changeRatio * 2.0) + (smallRatio * 1.5) - (bigCount * 0.7)

        scores.append(float(score))
        changeRatios.append(changeRatio)
        smallRatios.append(smallRatio)
        bigCounts.append(float(bigCount))

    return float(np.mean(scores)), float(np.mean(changeRatios)), float(np.mean(smallRatios)), float(np.mean(bigCounts))

def insectSpiderScore(frames):
    """
    Owady/pająk przy IR: zwykle 1-2 duże plamy, ale nie globalna zmiana jasności.
    """
    if len(frames) < 2:
        return 0.0

    blobScores = []
    for i in range(1, len(frames)):
        mask, prevGray, currGray = diffMask(frames[i - 1], frames[i], diffThresh=25)

        numLabels, _, stats, _ = cv2.connectedComponentsWithStats(mask, connectivity=8)
        areas = stats[1:, cv2.CC_STAT_AREA]

        if len(areas) == 0:
            blobScores.append(0.0)
            continue

        h, w = mask.shape
        biggest = float(np.max(areas) / (h * w))
        meanDelta = float(abs(currGray.mean() - prevGray.mean()) / 255.0)

        score = (biggest * 8.0) - (meanDelta * 2.0)
        blobScores.append(score)

    return float(np.mean(blobScores))

def lightChangeScore(frames):
    """
    Światło/cień/autoekspozycja: zmienia się średnia jasność.
    """
    if len(frames) < 2:
        return 0.0

    deltas = []
    for i in range(1, len(frames)):
        prevGray = cv2.cvtColor(frames[i - 1], cv2.COLOR_BGR2GRAY)
        currGray = cv2.cvtColor(frames[i], cv2.COLOR_BGR2GRAY)
        deltas.append(float(abs(currGray.mean() - prevGray.mean()) / 255.0))

    return float(np.mean(deltas))

# -------------------------
# 3) ANALIZA OBRAZU (YOLO: osoba/zwierzę)
# -------------------------

def resizeForInference(frame, maxWidth=640):
    h, w = frame.shape[:2]
    if w <= maxWidth:
        return frame
    scale = maxWidth / float(w)
    newW = int(w * scale)
    newH = int(h * scale)
    return cv2.resize(frame, (newW, newH), interpolation=cv2.INTER_LINEAR)

def drawDetectionsRed(frame, detections):
    out = frame.copy()

    for d in detections:
        x1, y1, x2, y2 = d["box"]
        label = d["label"]
        conf = d["conf"]

        # czerwony prostokąt (BGR)
        cv2.rectangle(out, (x1, y1), (x2, y2), (0, 0, 255), 2)

        text = f"{label} {conf:.2f}"
        cv2.putText(
            out,
            text,
            (x1, max(20, y1 - 8)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (0, 0, 255),
            2,
            cv2.LINE_AA
        )

    return out

def savePreviewIfDetected(frame, detections, outputPath):
    if len(detections) == 0:
        return False

    preview = drawDetectionsRed(frame, detections)

    os.makedirs(os.path.dirname(outputPath), exist_ok=True)
    cv2.imwrite(outputPath, preview)
    return True

def detectObjects(model, frame, conf=0.35) -> List[Dict[str, Any]]:
    res = model(frame, conf=conf, verbose=False)
    dets = []
    for r in res:
        for b in r.boxes:
            clsId = int(b.cls[0])
            label = model.names[clsId]
            c = float(b.conf[0])
            x1, y1, x2, y2 = map(int, b.xyxy[0].tolist())
            objectData = {"label": label, "conf": c, "box": (x1, y1, x2, y2)}
            Logger.INFO(objectData)
            dets.append(objectData)
            
    return dets

def filterTinyBoxes(dets, frameShape, minAreaRatio=0.012):
    h, w = frameShape[:2]
    minArea = (h * w) * minAreaRatio
    out = []
    for d in dets:
        x1, y1, x2, y2 = d["box"]
        area = max(0, x2 - x1) * max(0, y2 - y1)
        if area >= minArea:
            out.append(d)
    return out

def drawDetectionsRed(frame, detections):
    out = frame.copy()
    for d in detections:
        x1, y1, x2, y2 = d["box"]
        label = d["label"]
        conf = d["conf"]

        cv2.rectangle(out, (x1, y1), (x2, y2), (0, 0, 255), 2)

        text = f"{label} {conf:.2f}"
        cv2.putText(
            out,
            text,
            (x1, max(20, y1 - 8)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.6,
            (0, 0, 255),
            2,
            cv2.LINE_AA
        )
    return out

def buildPreviewPath(imgPath: str) -> str:
    # .../jpg/18/45/file.jpg -> .../preview/18/45/file_det.jpg
    parts = imgPath.split("/jpg/")
    if len(parts) == 2:
        base = parts[0]
        rest = parts[1]
        root, ext = os.path.splitext(rest)
        if ext.lower() not in [".jpg", ".jpeg", ".png"]:
            ext = ".jpg"
        out = os.path.join(base, "jpg", root + "_det" + ext)
        return out

    root, ext = os.path.splitext(imgPath)
    if ext.lower() not in [".jpg", ".jpeg", ".png"]:
        ext = ".jpg"
    return root + "_det" + ext

def savePreview(frame, detections, outputPath):
    root, ext = os.path.splitext(outputPath)
    if ext.lower() not in [".jpg", ".jpeg", ".png"]:
        outputPath = root + ".jpg"

    os.makedirs(os.path.dirname(outputPath), exist_ok=True)

    preview = drawDetectionsRed(frame, detections)
    ok = cv2.imwrite(outputPath, preview)
    if not ok:
        raise RuntimeError(f"cv2.imwrite returned False for: {outputPath}")
    return outputPath

def chooseBestDetectionFrame(candidateFrames, interestingLabels):
    """
    Wybiera klatkę do zapisu podglądu:
    - najwięcej interesujących detekcji
    - potem najwyższe conf
    - potem suma conf
    """
    if not candidateFrames:
        return None

    def frameScore(item):
        dets = [d for d in item["detections"] if d["label"] in interestingLabels]
        count = len(dets)
        bestConf = max((d["conf"] for d in dets), default=-1.0)
        sumConf = sum(d["conf"] for d in dets)
        return (count, bestConf, sumConf)

    return max(candidateFrames, key=frameScore)

# -------------------------
# 4) Decyzja: "dlaczego event powstał"
# -------------------------

def classifyEvent(imagePaths, yoloModel, savePreviewOnDetect=True):
    # Load frames
    frames = []
    validPaths = []
    for p in imagePaths:
        img = cv2.imread(p)
        if img is not None:
            frames.append(img)
            validPaths.append(p)

    if len(frames) == 0:
        return {"reasons": [], "confidence": 0.0, "details": {"error": "no_frames"}}

    # Klasy, które chcesz raportować i rysować (dopisz/usuń wg potrzeb)
    interestingLabels = {
        "person",
        "car", "motorcycle", "bicycle", "bus", "truck",
        "dog", "cat", "bird", "horse", "sheep", "cow"
    }

    # Analizuj 3 klatki dla szybkości: pierwsza/środek/ostatnia
    idxs = sorted(set([0, len(frames) // 2, len(frames) - 1]))

    candidateFrames = []
    allDets = []

    for i in idxs:
        original = frames[i]
        resized = resizeForInference(original, maxWidth=640)

        dets = detectObjects(yoloModel, resized, conf=0.35)
        #dets = filterTinyBoxes(dets, resized.shape, minAreaRatio=0.012)

        detsInteresting = [d for d in dets if d["label"] in interestingLabels and d["conf"] >= 0.35]

        if detsInteresting:
            candidateFrames.append({
                "imgPath": validPaths[i],
                "frame": resized,
                "detections": detsInteresting
            })

        allDets.extend(detsInteresting)

    # Nic nie wykryto
    if not allDets:
        return {"reasons": ["none"], "confidence": 0.0, "details": {"labelsFound": []}}

    # reasons = unikalne etykiety YOLO z eventu (bez priorytetów)
    labelsFound = sorted(set(d["label"] for d in allDets))
    bestConf = float(max(d["conf"] for d in allDets))

    details = {
        "labelsFound": labelsFound,
        "bestConf": bestConf,
        "numDetections": len(allDets)
    }

    # Zapisz preview (jedna najlepsza klatka) z WSZYSTKIMI obiektami
    if savePreviewOnDetect and candidateFrames:
        bestFrame = chooseBestDetectionFrame(candidateFrames, interestingLabels)
        outPath = buildPreviewPath(bestFrame["imgPath"])
        saved = savePreview(bestFrame["frame"], bestFrame["detections"], outPath)
        details["preview"] = saved

    return {"reasons": labelsFound, "confidence": bestConf, "details": details}


# -------------------------
# 5) Uruchomienie na folderze minuty (np. 18/45/)
# -------------------------

def analyzeMinuteDir(minuteDir: str, gapSeconds: int = 2):
    events = groupImagesIntoEvents(minuteDir, gapSeconds=gapSeconds)

    model = YOLO("yolov8n.pt")  # init once

    out = []
    for eventIndex, eventPaths in enumerate(events, start=1):
        result = classifyEvent(eventPaths, yoloModel=model)
        out.append({
            "minuteDir": minuteDir,
            "eventIndex": eventIndex,
            "numImages": len(eventPaths),
            "firstImage": os.path.basename(eventPaths[0]),
            "lastImage": os.path.basename(eventPaths[-1]),
            **result
        })

    Logger.INFO(json.dumps(out, ensure_ascii=False, indent=2))
    return out

if __name__ == "__main__":
    Logger.settings(saveToFile=False, showFilename=True, logLevel=LogLevel.INFO, print=True)
    minuteDir = "/mnt/intenso/MONITORING/brama_cam/2026-02-05/001/jpg/17/29/"
    analyzeMinuteDir(minuteDir, gapSeconds=3)
