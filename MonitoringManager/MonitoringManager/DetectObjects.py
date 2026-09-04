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
    def __init__(
        self,
        model_path: str = "yolov8n.pt",
        day_confidence_threshold: float = 0.55,
        night_confidence_threshold: float = 0.35,
        interesting_labels: set[str] | None = None,
        monitored_root: str = "/mnt/intenso/MONITORING",
        gap_seconds: int = 2,
        max_inference_width: int = 640,
    ):
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
        self.model_path = model_path
        self.day_confidence_threshold = day_confidence_threshold
        self.night_confidence_threshold = night_confidence_threshold
        self.interesting_labels = interesting_labels or {
            "person",
            "car", "motorcycle", "bicycle", "bus", "truck",
            "dog", "cat", "bird", "horse", "sheep", "cow",
        }
        self.monitored_root = monitored_root
        self.gap_seconds = gap_seconds
        self.max_inference_width = max_inference_width

        globals()["cv2"] = cv2
        globals()["np"] = np
        globals()["torch"] = torch
        globals()["YOLO"] = YOLO

    @staticmethod
    def parseSecondFromFilename(filename: str) -> int | None:
        m = re.match(r"^(\d{2})\D", filename)
        if not m:
            return None
        sec = int(m.group(1))
        return sec if 0 <= sec <= 59 else None

    def groupImagesIntoEvents(self, minuteDir: str, gapSeconds: int = 2) -> List[List[str]]:
        files = [
            f for f in os.listdir(minuteDir)
            if f.lower().endswith((".jpg", ".jpeg", ".png")) and "_det" not in f.lower()
        ]
        items: List[Tuple[int, str]] = []

        for f in files:
            sec = self.parseSecondFromFilename(f)
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

    def analyzeMinuteDir(self, minuteDir: str, gapSeconds: int | None = None) -> List[EventAnalysisRecord]:
        events = self.groupImagesIntoEvents(minuteDir, gapSeconds=self.gap_seconds if gapSeconds is None else gapSeconds)

        model = self.YOLO(self.model_path)  # init once

        out: List[EventAnalysisRecord] = []
        for eventIndex, eventPaths in enumerate(events, start=1):
            Logger.DEBUG(eventPaths)
            result = self.classifyEvent(eventPaths, yoloModel=model)
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

        Logger.DEBUG(json.dumps([asdict(record) for record in out], ensure_ascii=False, indent=2))
        return out

    @staticmethod
    def cropImage(frame, topPx=0, bottomPx=0, leftPx=0, rightPx=0):
        """
        Crop image by removing given number of pixels from each side.
        """

        h, w = frame.shape[:2]

        y1 = max(0, topPx)
        y2 = max(0, h - bottomPx)
        x1 = max(0, leftPx)
        x2 = max(0, w - rightPx)

        if y1 >= y2 or x1 >= x2:
            return frame

        return frame[y1:y2, x1:x2]

    @staticmethod
    def cropImagePercent(frame, topPct=0.0, bottomPct=0.0, leftPct=0.0, rightPct=0.0):
        """
        Crop image by percentage (0.0 - 1.0).
        Example: topPct=0.2 removes top 20% of image.
        """

        h, w = frame.shape[:2]

        topPx = int(h * topPct)
        bottomPx = int(h * bottomPct)
        leftPx = int(w * leftPct)
        rightPx = int(w * rightPct)

        return DetectObjectsAnalyzer.cropImage(
            frame,
            topPx=topPx,
            bottomPx=bottomPx,
            leftPx=leftPx,
            rightPx=rightPx,
        )

    def diffMask(self, prevFrame, currFrame, diffThresh=25):
        prevGray = self.cv2.cvtColor(prevFrame, self.cv2.COLOR_BGR2GRAY)
        currGray = self.cv2.cvtColor(currFrame, self.cv2.COLOR_BGR2GRAY)

        diff = self.cv2.absdiff(prevGray, currGray)
        diff = self.cv2.GaussianBlur(diff, (5, 5), 0)
        _, mask = self.cv2.threshold(diff, diffThresh, 255, self.cv2.THRESH_BINARY)
        return mask, prevGray, currGray

    def resizeForInference(self, frame, maxWidth: int | None = None):
        maxWidth = self.max_inference_width if maxWidth is None else maxWidth
        h, w = frame.shape[:2]
        if w <= maxWidth:
            return frame
        scale = maxWidth / float(w)
        newW = int(w * scale)
        newH = int(h * scale)
        return self.cv2.resize(frame, (newW, newH), interpolation=self.cv2.INTER_LINEAR)

    def classifyImageLighting(self, frame) -> str:
        gray = self.cv2.cvtColor(frame, self.cv2.COLOR_BGR2GRAY)
        mean = float(gray.mean())
        std = float(gray.std())
        darkPixelsRatio = float(self.np.mean(gray < 40))
        brightPixelsRatio = float(self.np.mean(gray > 180))

        try:
            hsv = self.cv2.cvtColor(frame, self.cv2.COLOR_BGR2HSV)
            sat_mean = float(self.np.mean(hsv[:, :, 1]))
        except Exception:
            sat_mean = 0.0

        w_mean = -0.01056557
        w_std = 0.01809076
        w_dark = -0.63433674
        w_bright = 1.57713505
        w_sat = -0.02090464
        b = 1.41355037

        score = (
            (w_mean * mean)
            + (w_std * std)
            + (w_dark * darkPixelsRatio)
            + (w_bright * brightPixelsRatio)
            + (w_sat * sat_mean)
            + b
        )

        return "night" if score > 0.15 else "day"

    def getDetectionConfidenceThreshold(self, frame) -> float:
        sceneType = self.classifyImageLighting(frame)
        if sceneType == "day":
            return self.day_confidence_threshold
        return self.night_confidence_threshold

    def detectObjects(self, model, frame, conf=0.35) -> List[DetectionInfo]:
        res = model(frame, conf=conf, verbose=False)
        dets: List[DetectionInfo] = []
        for r in res:
            for b in r.boxes:
                clsId = int(b.cls[0])
                label = model.names[clsId]
                c = float(b.conf[0])
                x1, y1, x2, y2 = map(int, b.xyxy[0].tolist())
                objectData = DetectionInfo(label=label, conf=c, box=(x1, y1, x2, y2))
                Logger.DEBUG(objectData)
                dets.append(objectData)

        return dets

    def buildPreviewPath(self, imgPath: str) -> str:
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

    def savePreview(self, frame, detections, outputPath):
        root, ext = os.path.splitext(outputPath)
        if ext.lower() not in [".jpg", ".jpeg", ".png"]:
            outputPath = root + ".jpg"

        os.makedirs(os.path.dirname(outputPath), exist_ok=True)

        preview = self.drawDetectionsRed(frame, detections)
        ok = self.cv2.imwrite(outputPath, preview)
        if not ok:
            raise RuntimeError(f"cv2.imwrite returned False for: {outputPath}")
        return outputPath

    def chooseBestDetectionFrame(self, candidateFrames, interestingLabels):
        if not candidateFrames:
            return None

        def frameScore(item):
            dets = [d for d in item.detections if d.label in interestingLabels]
            count = len(dets)
            bestConf = max((d.conf for d in dets), default=-1.0)
            sumConf = sum(d.conf for d in dets)
            return (count, bestConf, sumConf)

        return max(candidateFrames, key=frameScore)

    def summarizeDetectedLabels(self, detections: List[DetectionInfo]) -> Tuple[List[str], List[str], Dict[str, int]]:
        labelsFound = []
        for detection in detections:
            label = detection.get("label") if isinstance(detection, dict) else getattr(detection, "label", None)
            if label is not None:
                labelsFound.append(label)

        labelCounts = dict(sorted(Counter(labelsFound).items()))
        uniqueLabels = sorted(labelCounts.keys())
        return labelsFound, uniqueLabels, labelCounts

    def buildLabelConfidenceSuffix(self, detections: List[DetectionInfo]) -> str:
        bestByLabel: Dict[str, float] = {}
        for det in detections:
            label = det.label
            previous = bestByLabel.get(label, 0.0)
            bestByLabel[label] = max(previous, float(det.conf))

        if not bestByLabel:
            return ""

        parts: List[str] = []
        for label, conf in sorted(bestByLabel.items()):
            confPct = max(0, min(100, int(round(conf * 100))))
            slug = re.sub(r"[^a-z0-9]+", "_", label.lower()).strip("_")
            if slug:
                parts.append(f"{slug}-{confPct}")

        return "".join(f"-{part}" for part in parts)

    def classifyEvent(self, imagePaths, yoloModel, savePreviewOnDetect=True) -> EventClassificationResult:
        frames = []
        validPaths = []
        for p in imagePaths:
            img = self.cv2.imread(p)
            if img is not None:
                frames.append(img)
                validPaths.append(p)

        if len(frames) == 0:
            return EventClassificationResult(
                reasons=[],
                confidence=0.0,
                details=EventDetails(labelsFound=[], uniqueLabels=[], labelCounts={}, bestConf=0.0, numDetections=0),
            )

        interestingLabels = self.interesting_labels

        idxs = sorted(set([0, len(frames) // 2, len(frames) - 1]))

        candidateFrames: List[CandidateFrame] = []
        allDets: List[DetectionInfo] = []
        nameOfCamera = ""

        for i in idxs:
            original = frames[i]
            if "brama" in validPaths[i]:
                original = self.cropImagePercent(original, topPct=0.25, leftPct=0.20, rightPct=0.10)
                nameOfCamera = "brama"
            elif "altanka" in validPaths[i]:
                original = self.cropImagePercent(original, topPct=0.30, leftPct=0.15)
                nameOfCamera = "altanka"

            resized = self.resizeForInference(original, maxWidth=640)
            threshold = self.getDetectionConfidenceThreshold(resized)
            Logger.INFO(f"camera: {nameOfCamera}, conf threshold: {threshold:.2f})")

            dets = self.detectObjects(yoloModel, resized, conf=threshold)
            detsInteresting = [d for d in dets if d.label in interestingLabels and d.conf >= threshold]

            if detsInteresting:
                candidateFrames.append(CandidateFrame(
                    imgPath=validPaths[i],
                    frame=resized,
                    detections=detsInteresting,
                ))

            allDets.extend(detsInteresting)

        if not allDets:
            return EventClassificationResult(
                reasons=[],
                confidence=0.0,
                details=EventDetails(labelsFound=[], uniqueLabels=[], labelCounts={}, bestConf=0.0, numDetections=0),
            )

        labelsFound, uniqueLabels, labelCounts = self.summarizeDetectedLabels(allDets)
        bestConf = float(max(d.conf for d in allDets))

        details = EventDetails(
            labelsFound=labelsFound,
            uniqueLabels=uniqueLabels,
            labelCounts=labelCounts,
            bestConf=bestConf,
            numDetections=len(allDets),
        )

        saved = None
        if savePreviewOnDetect and candidateFrames:
            bestFrame = self.chooseBestDetectionFrame(candidateFrames, interestingLabels)
            outPath = self.buildPreviewPath(bestFrame.imgPath)

            saved = self.savePreview(bestFrame.frame, bestFrame.detections, outPath)

            monitored_root = self.monitored_root
            category = "other_det"
            if "brama" in bestFrame.imgPath:
                category = "brama_det"
            elif "altanka" in bestFrame.imgPath:
                category = "altanka_det"

            mtime = os.path.getmtime(bestFrame.imgPath)
            dt = datetime.fromtimestamp(mtime)
            date_dir = dt.strftime("%Y-%m-%d")
            mon_dir = os.path.join(monitored_root, category, date_dir)
            os.makedirs(mon_dir, exist_ok=True)

            full_dt = dt.strftime("%Y-%m-%d-%H:%M:%S")
            label_suffix = self.buildLabelConfidenceSuffix(bestFrame.detections)
            mon_filename = f"{full_dt}{label_suffix}.jpg"
            mon_path = os.path.join(mon_dir, mon_filename)

            details.monitoring_preview = mon_path
            Logger.DEBUG(f"monitoring preview path: {mon_path}")

            try:
                shutil.copy2(bestFrame.imgPath, mon_path)
                details.monitoring_original = mon_path
            except Exception as e2:
                Logger.ERROR(f"monitoring original copy failed: {e2}")

        details.preview = saved
        return EventClassificationResult(reasons=uniqueLabels, confidence=bestConf, details=details)


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


# -------------------------
# 5) Uruchomienie na folderze minuty (np. 18/45/)
# -------------------------

if __name__ == "__main__":
    Logger.settings(saveToFile=False, showFilename=True, logLevel=LogLevel.DEBUG, print=True)
    minuteDir = "/mnt/intenso/MONITORING/brama_cam/2026-08-15/001/jpg/17/07"
    DetectObjectsAnalyzer().analyzeMinuteDir(minuteDir, gapSeconds=3)
