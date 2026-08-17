from __future__ import annotations

import os
import re
import json
from collections import Counter
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Any, Tuple
from datetime import datetime
import shutil

from .Logger import Logger, LogLevel


class DetectObjectsAnalyzer:
    def __init__(self):
        os.environ.setdefault("TORCH_CPP_LOG_LEVEL", "ERROR")

        import cv2
        import numpy as np
        import torch
        from ultralytics import YOLO

        torch.backends.nnpack.enabled = False

        self.cv2 = cv2
        self.np = np
        self.torch = torch
        self.YOLO = YOLO

        globals()["cv2"] = cv2
        globals()["np"] = np
        globals()["torch"] = torch
        globals()["YOLO"] = YOLO

    def analyzeMinuteDir(self, minuteDir: str, gapSeconds: int = 2) -> List[EventAnalysisRecord]:
        events = groupImagesIntoEvents(minuteDir, gapSeconds=gapSeconds)

        model = self.YOLO("yolov8n.pt")  # init once

        out: List[EventAnalysisRecord] = []
        for eventIndex, eventPaths in enumerate(events, start=1):
            Logger.DEBUG(eventPaths)
            result = classifyEvent(eventPaths, yoloModel=model)
            out.append(EventAnalysisRecord(
                minuteDir=minuteDir,
                eventIndex=eventIndex,
                numImages=len(eventPaths),
                firstImage=os.path.basename(eventPaths[0]),
                lastImage=os.path.basename(eventPaths[-1]),
                reasons=result.reasons,
                confidence=result.confidence,
                details=result.details,
            ))

        Logger.INFO(json.dumps([asdict(record) for record in out], ensure_ascii=False, indent=2))
        return out


@dataclass
class DetectionInfo:
    label: str
    conf: float
    box: Tuple[int, int, int, int]


@dataclass
class EventDetails:
    labelsFound: List[str] = field(default_factory=list)
    uniqueLabels: List[str] = field(default_factory=list)
    labelCounts: Dict[str, int] = field(default_factory=dict)
    bestConf: float = 0.0
    numDetections: int = 0
    preview: str | None = None
    monitoring_preview: str | None = None
    monitoring_original: str | None = None


@dataclass
class EventClassificationResult:
    reasons: List[str]
    confidence: float
    details: EventDetails


@dataclass
class EventAnalysisRecord:
    minuteDir: str
    eventIndex: int
    numImages: int
    firstImage: str
    lastImage: str
    reasons: List[str]
    confidence: float
    details: EventDetails


@dataclass
class CandidateFrame:
    imgPath: str
    frame: Any
    detections: List[DetectionInfo]


def cropImage(frame, 
              topPx=0, 
              bottomPx=0, 
              leftPx=0, 
              rightPx=0):
    """
    Crop image by removing given number of pixels from each side.
    """

    h, w = frame.shape[:2]

    y1 = max(0, topPx)
    y2 = max(0, h - bottomPx)
    x1 = max(0, leftPx)
    x2 = max(0, w - rightPx)

    # zabezpieczenie przed złymi wartościami
    if y1 >= y2 or x1 >= x2:
        return frame  # zwróć oryginał jeśli coś poszło źle

    return frame[y1:y2, x1:x2]

def cropImagePercent(frame,
                     topPct=0.0,
                     bottomPct=0.0,
                     leftPct=0.0,
                     rightPct=0.0):
    """
    Crop image by percentage (0.0 - 1.0).
    Example: topPct=0.2 removes top 20% of image.
    """

    h, w = frame.shape[:2]

    topPx = int(h * topPct)
    bottomPx = int(h * bottomPct)
    leftPx = int(w * leftPct)
    rightPx = int(w * rightPct)

    return cropImage(
        frame,
        topPx=topPx,
        bottomPx=bottomPx,
        leftPx=leftPx,
        rightPx=rightPx
    )
    
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
    files = [
        f for f in os.listdir(minuteDir)
        if f.lower().endswith((".jpg", ".jpeg", ".png")) and "_det" not in f.lower()
    ]
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
        x1, y1, x2, y2 = d.box
        label = d.label
        conf = d.conf

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

def detectObjects(model, frame, conf=0.35) -> List[DetectionInfo]:
    res = model(frame, conf=conf, verbose=False)
    dets: List[DetectionInfo] = []
    for r in res:
        for b in r.boxes:
            clsId = int(b.cls[0])
            label = model.names[clsId]
            c = float(b.conf[0])
            x1, y1, x2, y2 = map(int, b.xyxy[0].tolist())
            objectData = DetectionInfo(label=label, conf=c, box=(x1, y1, x2, y2))
            Logger.INFO(objectData)
            dets.append(objectData)

    return dets

def filterTinyBoxes(dets, frameShape, minAreaRatio=0.012):
    h, w = frameShape[:2]
    minArea = (h * w) * minAreaRatio
    out = []
    for d in dets:
        x1, y1, x2, y2 = d.box
        area = max(0, x2 - x1) * max(0, y2 - y1)
        if area >= minArea:
            out.append(d)
    return out

def drawDetectionsRed(frame, detections):
    out = frame.copy()
    for d in detections:
        x1, y1, x2, y2 = d.box
        label = d.label
        conf = d.conf

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
        dets = [d for d in item.detections if d.label in interestingLabels]
        count = len(dets)
        bestConf = max((d.conf for d in dets), default=-1.0)
        sumConf = sum(d.conf for d in dets)
        return (count, bestConf, sumConf)

    return max(candidateFrames, key=frameScore)


def summarizeDetectedLabels(detections: List[DetectionInfo]) -> Tuple[List[str], List[str], Dict[str, int]]:
    """
    Zwraca:
    - labelsFound: list z duplikacjami, np. ['person', 'person', 'car']
    - uniqueLabels: unikatowe etykiety posortowane alfabetycznie
    - labelCounts: liczba wystąpień dla każdej etykiety
    """
    labelsFound = []
    for detection in detections:
        label = detection.get("label") if isinstance(detection, dict) else getattr(detection, "label", None)
        if label is not None:
            labelsFound.append(label)

    labelCounts = dict(sorted(Counter(labelsFound).items()))
    uniqueLabels = sorted(labelCounts.keys())
    return labelsFound, uniqueLabels, labelCounts

# -------------------------
# 4) Decyzja: "dlaczego event powstał"
# -------------------------

def classifyEvent(imagePaths, yoloModel, savePreviewOnDetect=True) -> EventClassificationResult:
    # Load frames
    frames = []
    validPaths = []
    for p in imagePaths:
        img = cv2.imread(p)
        if img is not None:
            frames.append(img)
            validPaths.append(p)

    if len(frames) == 0:
        return EventClassificationResult(
            reasons=[],
            confidence=0.0,
            details=EventDetails(labelsFound=[], uniqueLabels=[], labelCounts={}, bestConf=0.0, numDetections=0),
        )

    # Klasy, które chcesz raportować i rysować (dopisz/usuń wg potrzeb)
    interestingLabels = {
        "person",
        "car", "motorcycle", "bicycle", "bus", "truck",
        "dog", "cat", "bird", "horse", "sheep", "cow"
    }

    # Analizuj 3 klatki dla szybkości: pierwsza/środek/ostatnia
    idxs = sorted(set([0, len(frames) // 2, len(frames) - 1]))

    candidateFrames: List[CandidateFrame] = []
    allDets: List[DetectionInfo] = []

    for i in idxs:
        original = frames[i]
        if "brama" in validPaths[i]:
            original = cropImagePercent(original, topPct=0.25, leftPct=0.20, rightPct=0.10)
        elif "altanka" in validPaths[i]:
            original = cropImagePercent(original, topPct=0.30, leftPct=0.15)
 
        resized = resizeForInference(original, maxWidth=640)

        dets = detectObjects(yoloModel, resized, conf=0.35)
        #dets = filterTinyBoxes(dets, resized.shape, minAreaRatio=0.012)

        detsInteresting = [d for d in dets if d.label in interestingLabels and d.conf >= 0.35]

        if detsInteresting:
            candidateFrames.append(CandidateFrame(
                imgPath=validPaths[i],
                frame=resized,
                detections=detsInteresting,
            ))

        allDets.extend(detsInteresting)

    # Nic nie wykryto
    if not allDets:
        return EventClassificationResult(
            reasons=[],
            confidence=0.0,
            details=EventDetails(labelsFound=[], uniqueLabels=[], labelCounts={}, bestConf=0.0, numDetections=0),
        )

    labelsFound, uniqueLabels, labelCounts = summarizeDetectedLabels(allDets)
    bestConf = float(max(d.conf for d in allDets))

    details = EventDetails(
        labelsFound=labelsFound,
        uniqueLabels=uniqueLabels,
        labelCounts=labelCounts,
        bestConf=bestConf,
        numDetections=len(allDets),
    )

    # Zapisz preview (jedna najlepsza klatka) z WSZYSTKIMI obiektami
    if savePreviewOnDetect and candidateFrames:
        bestFrame = chooseBestDetectionFrame(candidateFrames, interestingLabels)
        outPath = buildPreviewPath(bestFrame.imgPath)

        saved = savePreview(bestFrame.frame, bestFrame.detections, outPath)

        # Additionally save a copy in /mnt/intenso/MONITORING/{brama|altanka}
        try:
            monitored_root = "/mnt/intenso/MONITORING"
            category = "other_det"
            if "brama" in bestFrame.imgPath:
                category = "brama_det"
            elif "altanka" in bestFrame.imgPath:
                category = "altanka_det"

            mon_dir = os.path.join(monitored_root, category)
            os.makedirs(mon_dir, exist_ok=True)

            # modification time of the original image
            mtime = os.path.getmtime(bestFrame.imgPath)
            dt = datetime.fromtimestamp(mtime).strftime("%Y-%m-%d-%H:%M:%S")

            # filenames are unique (<=1 image/sec), use unique labels only
            label_suffix = "".join(
                f"-{re.sub(r'[^a-z0-9]+', '_', label.lower()).strip('_')}"
                for label in uniqueLabels
            )
            mon_filename = f"{dt}{label_suffix}.jpg"
            mon_path = os.path.join(mon_dir, mon_filename)

            # save preview copy
            savePreview(bestFrame.frame, bestFrame.detections, mon_path)
            details.monitoring_preview = mon_path

            # also save the original image (kopiuj plik źródłowy) with '-orig' suffix
            try:
                root_orig, ext_orig = os.path.splitext(mon_filename)
                orig_filename = f"{root_orig}-orig{ext_orig}"
                mon_orig_path = os.path.join(mon_dir, orig_filename)
                shutil.copy2(bestFrame.imgPath, mon_orig_path)
                details.monitoring_original = mon_orig_path
            except Exception as e2:
                Logger.DEBUG(f"monitoring original copy failed: {e2}")
        except Exception as e:
            Logger.ERROR(f"monitoring save failed: {e}")

        details.preview = saved

    return EventClassificationResult(reasons=uniqueLabels, confidence=bestConf, details=details)


# -------------------------
# 5) Uruchomienie na folderze minuty (np. 18/45/)
# -------------------------

def analyzeMinuteDir(minuteDir: str, gapSeconds: int = 2) -> List[EventAnalysisRecord]:
    engine = DetectObjectsAnalyzer()
    return engine.analyzeMinuteDir(minuteDir, gapSeconds=gapSeconds)


if __name__ == "__main__":
    Logger.settings(saveToFile=False, showFilename=True, logLevel=LogLevel.DEBUG, print=True)
    minuteDir = "/mnt/intenso/MONITORING/brama_cam/2026-08-15/001/jpg/17/07"
    analyzeMinuteDir(minuteDir, gapSeconds=3)
