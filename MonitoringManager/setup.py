import setuptools

setuptools.setup(
    name="phonecontacts",
    version="0.2",
    license="MIT",
    description="Phone Contacts managed in file",
    url="https://github.com/example",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    install_requires=['simplelogger'],
    python_requires='>=3.6',
)
