import unittest

from MonitoringManager.DetectObjects import summarizeDetectedLabels


class DetectObjectsLabelCountsTests(unittest.TestCase):
    def test_summarize_detected_labels_keeps_duplicates_in_details_but_unique_in_reasons(self):
        detections = [
            {"label": "person"},
            {"label": "person"},
            {"label": "car"},
            {"label": "car"},
            {"label": "car"},
        ]

        labels_found, unique_labels, label_counts = summarizeDetectedLabels(detections)
        reasons = unique_labels

        self.assertEqual(labels_found, ["person", "person", "car", "car", "car"])
        self.assertEqual(reasons, ["car", "person"])
        self.assertEqual(unique_labels, ["car", "person"])
        self.assertEqual(label_counts, {"car": 3, "person": 2})


if __name__ == "__main__":
    unittest.main()
