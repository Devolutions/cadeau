from conans import ConanFile
import os

class Importer(ConanFile):
    name = 'importer'
    settings = 'os', 'build_type'
    generators = 'cmake'

    def build_requirements(self):
        self.build_requires('halide/12.0.1-1@devolutions/stable')
        self.build_requires('libyuv/1661-3@devolutions/stable')
