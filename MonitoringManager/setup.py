import setuptools

setuptools.setup(
    name="MonitoringManager",
    version="0.2",
    license="MIT",
    description="Analyze pictures created by monitoring camera and send notification by GSM",
    url="https://github.com/bartek56/IPRecorder",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    install_requires=['GSMEngine, sh'],
    python_requires='>=3.6',
)
