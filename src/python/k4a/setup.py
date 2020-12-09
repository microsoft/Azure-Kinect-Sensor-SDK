from setuptools import setup, find_packages

setup(
    name='k4a',
    version='0.0.1',
    author='Jonathan Santos',
    author_email='jonsanto@microsoft.com',
    description='Python interface to Azure Kinect API.',
    keywords=['k4a', 'Azure Kinect', 'Kinect for Azure'],
    url='',
    license='Copyright (C) Microsoft Corporation. All rights reserved.',
    python_requires='>=3.6',
    packages=find_packages(exclude=['docs', 'data']),
    extras_require={
        'dev': [
        ],
    },
    zip_safe=False,
    tests_require=[
        'pytest',
    ],
    package_data={
        'k4a': [
                'lib/*',
        ]
    },
    install_requires=[
    ],
)
