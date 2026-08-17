
import os
# Make inner 'MonitoringManager/MonitoringManager' folder available as
# the package path so imports like `from MonitoringManager.DetectObjects ...`
# resolve to files in the nested directory used by the repository layout.
__path__.insert(0, os.path.join(os.path.dirname(__file__), "MonitoringManager"))
