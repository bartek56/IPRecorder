import shutil
import tempfile
from pathlib import Path
from unittest import TestCase
from unittest.mock import MagicMock
from types import SimpleNamespace

import cv2
import numpy as np

import MonitoringManager.DetectObjects as detect_objects_module
from MonitoringManager.CameraAnalyzer import CameraAnalyzer


detect_objects_module.cv2 = cv2
detect_objects_module.np = np


class DayNightImageClassificationTests(TestCase):
    @classmethod
    def setUpClass(cls):
        cls.files_dir = Path(__file__).parent / "files"
        print("setUpClass")

    def _classify(self, image_path: Path) -> str:
        img = cv2.imread(str(image_path))
        self.assertIsNotNone(img, f"Nie udało się wczytać obrazu: {image_path}")
        analyzer = detect_objects_module.DetectObjectsAnalyzer()
        return analyzer.classifyImageLighting(img)

    def test_day_images_are_classified_as_day(self):
        for image_path in sorted(self.files_dir.glob("*day.jpg")):
            with self.subTest(image=image_path.name):
                self.assertEqual(self._classify(image_path), "day", f"Zdjęcie {image_path.name} powinno być rozpoznane jako dzień")

    def test_night_images_are_classified_as_night(self):
        for image_path in sorted(self.files_dir.glob("*night.jpg")):
            with self.subTest(image=image_path.name):
                self.assertEqual(self._classify(image_path), "night", f"Zdjęcie {image_path.name} powinno być rozpoznane jako noc")


class CameraAnalyzerTests(TestCase):
    @classmethod
    def setUpClass(cls):
        print("setUpClass")

    def setUp(self):
        self.base = Path(tempfile.mkdtemp(prefix="camera-analyzer-tests-"))
        self.log_file = self.base / "alarm.log"
        self.log_file.write_text("")
        self.day = self.base / "2026-08-15"
        self.day.mkdir()
        self.nested_dir = self.day / "001" / "jpg" / "001" / "001"
        self.nested_dir.mkdir(parents=True)

        self.mock_analyze_minute_dir = MagicMock()
        self.mock_analyze_minute_dir.analyzeMinuteDir.return_value = []
        print("setUp")

    def tearDown(self):
        shutil.rmtree(self.base, ignore_errors=True)
        print("tearDown")

    @classmethod
    def tearDownClass(cls):
        print("tearDownClass")

    def _touch(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"x")

    def test_CountNewFiles_SameDirTriggersDetection(self):
        for i in range(2):
            self._touch(self.day / f"{i:02d}_a.jpg")

        ca = CameraAnalyzer(
            str(self.base),
            "CAM",
            str(self.log_file),
            minNewFilesToDetect=1,
            notificationBlockDuration=60,
            detect_objects=self.mock_analyze_minute_dir,
        )

        self._touch(self.day / "59_new.jpg")

        result = ca.analyzeMoving()

        self.assertIsNotNone(result)
        self.assertEqual(ca.countFiles, 3)
        self.mock_analyze_minute_dir.analyzeMinuteDir.assert_called_once()

    def test_CountNewFiles_AcrossPrevAndNewDir(self):
        day1 = self.base / "2026-08-15"
        for i in range(3):
            self._touch(day1 / f"{i:02d}_a.jpg")

        ca = CameraAnalyzer(
            str(self.base),
            "CAM",
            str(self.log_file),
            minNewFilesToDetect=2,
            notificationBlockDuration=60,
            detect_objects=self.mock_analyze_minute_dir,
        )

        self._touch(day1 / "10_new1.jpg")
        self._touch(day1 / "11_new2.jpg")

        day2 = self.base / "2026-08-16"
        day2.mkdir()
        for i in range(4):
            self._touch(day2 / f"{i:02d}_b.jpg")

        nested_day2 = day2 / "001" / "jpg" / "001" / "001"
        nested_day2.mkdir(parents=True)
        self._touch(nested_day2 / "frame.jpg")

        result = ca.analyzeMoving()

        self.assertIsNotNone(result)
        self.assertEqual(ca.theNewestDir, "2026-08-16")
        self.assertEqual(ca.countFiles, 5)
        self.mock_analyze_minute_dir.analyzeMinuteDir.assert_called_once()

    def test_compute_scaled_movement_level_basic_cases(self):
        ca = CameraAnalyzer("/tmp", "CAM", "log.txt", minNewFilesToDetect=1, notificationBlockDuration=60, detect_objects=self.mock_analyze_minute_dir)

        scaled, max_possible = ca.compute_scaled_movement_level(0)

        self.assertEqual(max_possible, 10)
        self.assertEqual(scaled, 0)

        scaled2, _ = ca.compute_scaled_movement_level(80)
        self.assertEqual(scaled2, 10)

    def test_compute_scaled_movement_level_with_higher_threshold(self):
        ca = CameraAnalyzer("/tmp", "CAM", "log.txt", minNewFilesToDetect=30, notificationBlockDuration=60, detect_objects=self.mock_analyze_minute_dir)

        scaled, max_possible = ca.compute_scaled_movement_level(1)

        self.assertEqual(max_possible, 10)
        self.assertGreaterEqual(scaled, 0)
        self.assertLessEqual(scaled, 2)

    def test_analyzeMoving_NoAlarm_WhenBelowThreshold(self):
        self._touch(self.day / "01.jpg")
        ca = CameraAnalyzer(
            str(self.base),
            "CAM",
            str(self.log_file),
            minNewFilesToDetect=3,
            notificationBlockDuration=60,
            detect_objects=self.mock_analyze_minute_dir,
        )

        result = ca.analyzeMoving()

        self.assertIsNone(result)
        self.assertEqual(ca.alarmLevel, 0)
        self.assertFalse(ca.readyToNotify)
        self.mock_analyze_minute_dir.analyzeMinuteDir.assert_not_called()

    def test_analyzeMoving_Alarm_WhenThresholdReached(self):
        self._touch(self.day / "01.jpg")
        self._touch(self.day / "02.jpg")
        ca = CameraAnalyzer(
            str(self.base),
            "CAM",
            str(self.log_file),
            minNewFilesToDetect=2,
            notificationBlockDuration=60,
            detect_objects=self.mock_analyze_minute_dir,
        )

        self._touch(self.day / "03.jpg")
        self._touch(self.day / "04.jpg")

        result = ca.analyzeMoving()

        self.assertIsNotNone(result)
        self.assertTrue(result.message.startswith("ALARM CAM"))
        self.assertEqual(result.level, 2)
        self.assertFalse(result.hasReasons)
        self.assertEqual(ca.alarmLevel, 0)
        self.assertFalse(ca.readyToNotify)
        self.mock_analyze_minute_dir.analyzeMinuteDir.assert_called_once()

    def test_analyzeMoving_WithDetection_ReasonsIncluded(self):
        # prepare a mock detector that returns one detection with a reason
        mock_detect = MagicMock()
        mock_detect.analyzeMinuteDir.return_value = [SimpleNamespace(reasons=['person']), SimpleNamespace(reasons=[])]

        ca = CameraAnalyzer(
            str(self.base),
            "CAM",
            str(self.log_file),
            minNewFilesToDetect=1,
            notificationBlockDuration=60,
            detect_objects=mock_detect,
        )

        # create files to reach threshold
        self._touch(self.day / "01.jpg")
        self._touch(self.day / "02.jpg")

        result = ca.analyzeMoving()

        self.assertIsNotNone(result)
        self.assertTrue(result.hasReasons)
        self.assertIn('person', result.reasons)
        mock_detect.analyzeMinuteDir.assert_called_once()

    def test_analyzeMoving_ReturnsErrorWhenDiskMissing(self):
        # point analyzer to a non-existing directory
        missing_dir = str(self.base / "nonexistent")
        mock_detect = MagicMock()

        ca = CameraAnalyzer(
            missing_dir,
            "CAM",
            str(self.log_file),
            minNewFilesToDetect=1,
            notificationBlockDuration=60,
            detect_objects=mock_detect,
        )

        result = ca.analyzeMoving()

        self.assertIsNotNone(result)
        self.assertEqual(result.message, "Error with Disk")
        self.assertTrue(result.hasReasons)
