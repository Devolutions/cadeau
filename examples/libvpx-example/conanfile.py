from conans import ConanFile

class Importer(ConanFile):
    name = 'importer'
    settings = {
        'os': ['Windows'],
        'build_type': ['Release', 'Debug'],
        'distro': ['ANY']
    }
    generators = 'cmake'

    def requirements(self):
        self.requires('libvpx/1.10.0@devolutions/stable')
