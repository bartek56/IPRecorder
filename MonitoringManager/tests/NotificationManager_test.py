import os
import tempfile
import unittest
from types import ModuleType, SimpleNamespace
from unittest.mock import MagicMock, patch


def make_fake_gsm_module():
    fake_module = ModuleType("GSMEngine")
    gsm_mock = MagicMock()
    gsm_mock.initialize.return_value = True
    gsm_mock.is_alive.return_value = True
    gsm_mock.sendSms = MagicMock()
    gsm_mock.getSms = MagicMock(return_value=None)
    gsm_mock.getCall = MagicMock(return_value=None)
    gsm_mock.shutdown = MagicMock()

    def GSMManager(serial):
        return gsm_mock

    fake_module.GSMManager = GSMManager
    return fake_module, gsm_mock


class NotificationManagerTests(unittest.TestCase):
    def setUp(self):
        # prepare temporary active users file
        fd, self.active_users_path = tempfile.mkstemp(prefix="active-users-", text=True)
        os.close(fd)

    def tearDown(self):
        try:
            os.remove(self.active_users_path)
        except OSError:
            pass

    def test_sendSMSNotification_calls_sendSms_for_each_active_user(self):
        with patch.dict('sys.modules') as modules:
            fake_mod, gsm_mock = make_fake_gsm_module()
            modules['GSMEngine'] = fake_mod

            # import module under test after injecting fake GSMEngine
            from MonitoringManager.NotificationManager import NotificationManager

            # prepare active users file with two users
            with open(self.active_users_path, 'w') as f:
                f.write("John Doe\nJane Smith\n")

            # mock Contacts to return default numbers
            fake_contact_instance = SimpleNamespace()
            fake_contact_instance.GetDefaultNumber = MagicMock(side_effect=lambda name: '111222333')

            with patch('MonitoringManager.NotificationManager.Contacts', return_value=fake_contact_instance):
                nm = NotificationManager(self.active_users_path, 'TTY', '999', gsm_manager=gsm_mock)

            # call sendSMSNotification
            nm.sendSMSNotification('Hello')

            # expect sendSms called twice
            self.assertEqual(gsm_mock.sendSms.call_count, 2)

    def test_checkNewMessage_unknown_sender_sends_admin_sms(self):
        with patch.dict('sys.modules') as modules:
            fake_mod, gsm_mock = make_fake_gsm_module()
            modules['GSMEngine'] = fake_mod
            from MonitoringManager.NotificationManager import NotificationManager

            # create empty active users file
            with open(self.active_users_path, 'w') as f:
                f.write("")

            fake_contact_instance = SimpleNamespace()
            fake_contact_instance.LookingForContactByNumber = MagicMock(return_value=None)

            with patch('MonitoringManager.NotificationManager.Contacts', return_value=fake_contact_instance):
                nm = NotificationManager(self.active_users_path, 'TTY', 'ADMIN123', gsm_manager=gsm_mock)

                # simulate incoming sms from unknown number
                gsm_mock.getSms.return_value = SimpleNamespace(number='+48123456789', msg='HELLO')

                nm.checkNewMessage()

                # sendSms should be called to admin number
                gsm_mock.sendSms.assert_called()
                called_args = gsm_mock.sendSms.call_args[0]
                self.assertEqual(called_args[0], '+48' + 'ADMIN123')

    def test_checkNewMessage_status_from_known_contact_sends_status(self):
        with patch.dict('sys.modules') as modules:
            fake_mod, gsm_mock = make_fake_gsm_module()
            modules['GSMEngine'] = fake_mod
            from MonitoringManager.NotificationManager import NotificationManager

            with open(self.active_users_path, 'w') as f:
                f.write("")

            contact = SimpleNamespace(name='A', surname='B', numbers=[SimpleNamespace(number='600600600')])
            fake_contact_instance = SimpleNamespace()
            fake_contact_instance.LookingForContactByNumber = MagicMock(return_value=contact)
            fake_contact_instance.GetDefaultNumber = MagicMock(return_value='600600600')

            fake_status = SimpleNamespace()
            fake_status.checkStatus = MagicMock(return_value='STATUS OK')

            # patch Contacts and IpRecorderStatus
            with patch('MonitoringManager.NotificationManager.Contacts', return_value=fake_contact_instance), \
                 patch('MonitoringManager.NotificationManager.IpRecorderStatus', return_value=fake_status):
                nm = NotificationManager(self.active_users_path, 'TTY', 'ADMIN123', gsm_manager=gsm_mock)

            # simulate sms with +48 prefix
            gsm_mock.getSms.return_value = SimpleNamespace(number='+48600600600', msg='STATUS')

            nm.checkNewMessage()

            # expect sendSms called with contact number and status message
            gsm_mock.sendSms.assert_called()
            called_args = gsm_mock.sendSms.call_args[0]
            self.assertEqual(called_args[0], '+48' + '600600600')
            self.assertIn('STATUS OK', called_args[1])


if __name__ == '__main__':
    unittest.main()
