from conans import ConanFile, CMake, tools


class H5jpeglsConan(ConanFile):
    name = "h5jpegls"
    version = "1.0.0-alpha"
    license = "Apache-2.0"
    author = "Andrew Marshall <planetmarshalluk@gmail.com>"
    url = "https://github.com/planetmarshall/jpegls-hdf-filter"
    description = "HDF5 JPEG-LS Compression Filter"
    topics = ("hdf5", "compression")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "fPIC": [True, False],
        "static_plugin": [True, False],
        "shared": [True, False],
        "enable_tests": [True, False]
    }
    default_options = {
        "fPIC": True,
        "static_plugin": True,
        "shared": False,
        "enable_tests": False
    }
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

    def configure(self):
        if not self.options.static_plugin:
            self.options["hdf5"].shared = True

    def requirements(self):
        self.requires("hdf5/1.12.0")
        self.requires("charls/2.1.0")

    def build_requirements(self):
        if self.options.enable_tests:
            self.build_requires("catch2/2.13.7")
        if self.settings.get_safe("compiler.toolset") is None:
            self.build_requires("ninja/1.10.2")

    def _configure_cmake(self):
        if self._cmake is not None:
            return self._cmake

        self._cmake = CMake(self)
        self._cmake.definitions.update({
            "H5JPEGLS_STATIC_PLUGIN": self.options.static_plugin,
            "H5JPEGLS_ENABLE_TESTS": self.options.enable_tests,
        })
        self._cmake.configure()
        return self._cmake

    def build(self):
        with tools.run_environment(self):
            cmake = self._configure_cmake()
            cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("LICENSE.txt", src=self.source_folder, dst="licenses")

    def package_info(self):
        self.cpp_info.libs = ["h5jpegls"]
