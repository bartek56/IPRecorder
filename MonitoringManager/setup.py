import setuptools
from setuptools.command.install import install
import subprocess

class CustomInstallCommand(install):
    """Customized setuptools install command - builds with PyInstaller."""
    def run(self):
        install.run(self)
        subprocess.call(['python3', 'build.py'])

setuptools.setup(
    name="MonitoringManager",
    version="1.0",
    license="MIT",
    description="Analyze pictures created by monitoring camera and send notification by GSM",
    url="https://github.com/bartek56/IPRecorder",
    packages=setuptools.find_packages(),
    entry_points={
        'console_scripts': [
            'MonitoringManager=MonitoringManager.MonitoringManager:main',
        ],
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    install_requires=['GSMEngine==1.0', 'sh'],
    python_requires='>=3.6',
#    cmdclass={
#        'install': CustomInstallCommand,
#    }
)
