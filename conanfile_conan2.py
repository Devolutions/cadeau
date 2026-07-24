from conan import ConanFile


class Importer(ConanFile):
    name = "importer"
    settings = "os", "arch", "build_type"

    def requirements(self):
        self.requires("libpng/1.6.39@devolutions/stable")
        self.requires("zlib/1.3.1@devolutions/stable")
        self.requires("libvpx/1.10.0@devolutions/stable")
