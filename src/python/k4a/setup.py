from setuptools import setup, find_packages

setup(
    name='k4a',
    version='0.0.2',
    author='Jonathan Santos',
    author_email='jonsanto@microsoft.com',
    description='Python interface to Azure Kinect API.',
    keywords=['k4a', 'Azure Kinect', 'Kinect for Azure'],
    url='https://github.com/JonathanESantos/Azure-Kinect-Sensor-SDK/tree/python_ctypes_bindings',
    license='Copyright (C) Microsoft Corporation. All rights reserved.',
    python_requires='>=3.6',
    packages=find_packages('src'),
    package_dir={'': 'src'},
    package_data={"k4a":["_libs/*"]},
    zip_safe=False,
    tests_require=[
        'pytest',
        'numpy'
    ],
    install_requires=[
        'numpy',
    ],
    extras_require={
        'test': ['pytest', 'numpy'],
    },
)
