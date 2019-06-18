from conans import ConanFile
import os

class Importer(ConanFile):
    name = 'importer'
    settings = 'os', 'build_type'
    generators = 'cmake'

    def build_requirements(self):
        if self.settings.os == 'Windows':
            self.build_requires('clang-llvm/8.0.0-2@devolutions/stable')
        else:
            self.build_requires('clang-llvm/6.0.1-1@devolutions/stable')
        self.build_requires('halide/1335f0219-2@devolutions/stable')
        self.build_requires('libyuv/1661-3@devolutions/stable')