import unittest
from types import SimpleNamespace
from unittest.mock import MagicMock, patch

from MonitoringManager.StatusManager import IpRecorderStatus


class IpRecorderStatusTests(unittest.TestCase):
    def test_checkNetwork_returns_true_when_http_works(self):
        with patch('MonitoringManager.StatusManager.urllib.request.urlopen', return_value=object()):
            status = IpRecorderStatus()
            self.assertTrue(status.checkNetwork())

    def test_checkNetwork_returns_false_when_exception_occurs(self):
        with patch('MonitoringManager.StatusManager.urllib.request.urlopen', side_effect=Exception('boom')):
            status = IpRecorderStatus()
            self.assertFalse(status.checkNetwork())

    def test_checkStatus_returns_ok_when_all_checks_pass(self):
        status = IpRecorderStatus()

        with patch.object(status, 'checkNetwork', return_value=True), \
             patch.object(status, 'checkFtpActiveClients', return_value=''), \
             patch('MonitoringManager.StatusManager.os.path.isdir', return_value=True), \
             patch.object(status, 'checkMemory', return_value='Internal Memory: 10/20\nExternal Memory: 5/10'):
            result = status.checkStatus()

        self.assertIn('everything okay', result)
        self.assertIn('Internal Memory', result)

    def test_checkStatus_reports_disk_not_mounted(self):
        status = IpRecorderStatus()

        with patch.object(status, 'checkNetwork', return_value=True), \
             patch.object(status, 'checkFtpActiveClients', return_value=''), \
             patch('MonitoringManager.StatusManager.os.path.isdir', return_value=False):
            result = status.checkStatus()

        self.assertIn('disk is not mounted', result)
        self.assertNotIn('everything okay', result)

    def test_checkStatus_reports_network_failure(self):
        status = IpRecorderStatus()

        with patch.object(status, 'checkNetwork', return_value=False), \
             patch.object(status, 'checkFtpActiveClients', return_value=''), \
             patch('MonitoringManager.StatusManager.os.path.isdir', return_value=True), \
             patch.object(status, 'checkMemory', return_value='Internal Memory: 1/2\nExternal Memory: 3/4'):
            result = status.checkStatus()

        self.assertIn("I'm not connected to network", result)

    def test_checkStatus_reports_ftp_clients_missing(self):
        status = IpRecorderStatus()

        with patch.object(status, 'checkNetwork', return_value=True), \
             patch.object(status, 'checkFtpActiveClients', return_value='camera1 not available\ncamera2 not available'), \
             patch('MonitoringManager.StatusManager.os.path.isdir', return_value=True), \
             patch.object(status, 'checkMemory', return_value='Internal Memory: 1/2\nExternal Memory: 3/4'):
            result = status.checkStatus()

        self.assertIn('camera1 not available', result)
        self.assertIn('camera2 not available', result)


if __name__ == '__main__':
    unittest.main()
