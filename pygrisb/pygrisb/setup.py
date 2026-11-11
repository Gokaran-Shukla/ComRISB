# Build configuration lives in pyproject.toml (PEP 517/621).
# This shim keeps "python setup.py ..." based workflows working.
from setuptools import setup

setup()
