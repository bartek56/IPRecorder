import unittest
from pathlib import Path

import cv2

from MonitoringManager.DetectObjects import DetectObjectsAnalyzer


class DetectObjectsLabelCountsTests(unittest.TestCase):
    def test_summarize_detected_labels_keeps_duplicates_in_details_but_unique_in_reasons(self):
        analyzer = DetectObjectsAnalyzer()
        detections = [
            {"label": "person"},
            {"label": "person"},
            {"label": "car"},
            {"label": "car"},
            {"label": "car"},
        ]

        labels_found, unique_labels, label_counts = analyzer.summarizeDetectedLabels(detections)
        reasons = unique_labels

        self.assertEqual(labels_found, ["person", "person", "car", "car", "car"])
        self.assertEqual(reasons, ["car", "person"])
        self.assertEqual(unique_labels, ["car", "person"])
        self.assertEqual(label_counts, {"car": 3, "person": 2})


class DetectObjectsFixtureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.model = DetectObjectsAnalyzer().YOLO("yolov8n.pt")
        cls.objects_dir = Path(__file__).parent / "objects"

    def _detect_labels(self, image_path: Path, conf: float) -> list[str]:
        img = cv2.imread(str(image_path))
        self.assertIsNotNone(img, f"Nie udało się wczytać obrazu testowego: {image_path}")

        analyzer = DetectObjectsAnalyzer()
        detections = analyzer.detectObjects(self.model, img, conf=conf)
        return [d.label for d in detections]

    def test_object_fixture_images_are_detected_at_the_expected_threshold(self):
        fixture_checks = [
            ("42[M][0@0][0].jpg", 0.01, ["person", "motorcycle"]),
            ("44[M][0@0][0].jpg", 0.20, ["person"]),
            ("45[M][0@0][0].jpg", 0.05, ["person"]),
        ]

        for image_name, conf, expected_labels in fixture_checks:
            with self.subTest(image=image_name, threshold=conf):
                image_path = self.objects_dir / image_name
                labels = self._detect_labels(image_path, conf)

                self.assertTrue(labels, f"Model nie wykrył żadnych obiektów na zdjęciu {image_name} przy progu {conf}.")
                for expected_label in expected_labels:
                    self.assertIn(
                        expected_label,
                        labels,
                        f"Model nie wykrył {expected_label!r} na zdjęciu {image_name} przy progu {conf}. Wykryte etykiety: {labels}",
                    )


if __name__ == "__main__":
    unittest.main()
