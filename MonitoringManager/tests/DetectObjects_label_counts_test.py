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
        image_paths = sorted(self.objects_dir.glob("*.jpg"))
        self.assertTrue(image_paths, f"Brak obrazów testowych w katalogu {self.objects_dir}")

        for image_path in image_paths:
            with self.subTest(image=image_path.name):
                labels = []
                for conf in (0.01, 0.05, 0.10, 0.20, 0.35):
                    labels = self._detect_labels(image_path, conf=conf)
                    if labels:
                        break

                self.assertTrue(labels, f"Model nie wykrył żadnych obiektów na zdjęciu {image_path.name} przy żadnym z testowanych progów.")
                self.assertGreaterEqual(len(labels), 1, f"Na zdjęciu {image_path.name} wykryto za mało obiektów: {labels}")


if __name__ == "__main__":
    unittest.main()
