from conans import ConanFile, CMake, tools


class H5jpeglsConan(ConanFile):
    name = "h5jpegls"
    version = "1.0.0"
    license = "Apache-2.0"
    author = "Andrew Marshall <planetmarshalluk@gmail.com>"
    url = "https://github.com/planetmarshall/jpegls-hdf-filter"
    description = "HDF5 JPEG-LS Compression Filter"
    topics = ("hdf5", "compression")
    settings = "os", "compiler", "build_type", "arch"
    options = {"fPIC": [True, False]}
    default_options = {"fPIC": True}
    generators = "cmake", "cmake_find_package"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }
    _cmake = None

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def requirements(self):
        self.requires("hdf5/1.12.0")
        self.requires("charls/2.1.0")

    def _configure_cmake(self):
        if self._cmake is not None:
            return self._cmake

        self._cmake = CMake(self)
        self._cmake.configure()
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["h5jpegls"]
