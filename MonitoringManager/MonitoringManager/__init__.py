import sys
import os

path = os.path.realpath(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(path))+"/MonitoringManager"))