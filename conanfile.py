from conans import ConanFile
import os

class Importer(ConanFile):
    name = 'importer'
    settings = 'os', 'build_type'
    generators = 'cmake'

    def requirements(self):
        self.requires('libjpeg/2.1.0@devolutions/stable')
        self.requires('libpng/1.6.36@devolutions/stable')
        self.requires('zlib/1.2.11@devolutions/stable')

        self.requires('winpr/3.0.0@devolutions/stable')
        self.requires('mbedtls/2.16.0@devolutions/stable')

        if self.settings.os in ['Windows', 'Macos', 'Linux']:
            self.requires('libvpx/1.10.0@devolutions/stable')
            self.requires('libwebm/bc32e3c@devolutions/stable')
