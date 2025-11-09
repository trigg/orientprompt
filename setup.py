from setuptools import setup, find_namespace_packages


def readme():
    return open("README.md", "r", encoding="utf-8").read()


setup(
    name="orientprompt",
    author="trigg",
    author_email="",
    version="0.1.0",
    description="Suggest changing screen orientation",
    long_description=readme(),
    long_description_content_type="text/markdown",
    packages=find_namespace_packages(),
    include_package_data=True,
    install_requires=["PyGObject>=3.22", "sh"],
    entry_points={
        "console_scripts": [
            "orientprompt = orientprompt.orientprompt:entrypoint",
        ]
    },
    classifiers=[
        "Intended Audience :: End Users/Desktop",
    ],
    keywords="",
    license="GPLv3+",
)
