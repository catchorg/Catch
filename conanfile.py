#!/usr/bin/env python
from conans import ConanFile, CMake, tools

class CatchConan(ConanFile):
    name = "catch2"
    description = "A modern, C++-native, framework for unit-tests, TDD and BDD"
    topics = ("conan", "catch2", "unit-test", "tdd", "bdd")
    url = "https://github.com/catchorg/Catch2"
    homepage = url
    license = "BSL-1.0"

    exports = "LICENSE.txt"
    exports_sources = ("src/*", "CMakeLists.txt", "CMake/*", "extras/*")

    settings = "os", "compiler", "build_type", "arch"
    
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    
    generators = "cmake"

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_TESTING"] = "OFF"
        cmake.definitions["CATCH_INSTALL_DOCS"] = "OFF"
        cmake.definitions["CATCH_INSTALL_HELPERS"] = "ON"
        cmake.definitions['CATCH_EXPERIMENTAL_SHARED'] = self.options.shared
        cmake.configure(build_folder="build")
        return cmake

    def build(self):
        # We need this workaround until the toolchains feature
        # to inject stuff like MD/MT
        line_to_replace = 'list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")'
        tools.replace_in_file("CMakeLists.txt", line_to_replace,
                              '''{}
include("{}/conanbuildinfo.cmake")
conan_basic_setup()'''.format(line_to_replace, self.install_folder.replace("\\", "/")))

        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy(pattern="LICENSE.txt", dst="licenses")
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = "Catch2"
        self.cpp_info.names["cmake_find_package_multi"] = "Catch2"
        # Catch2
        self.cpp_info.components["catch2base"].names["cmake_find_package"] = "Catch2"
        self.cpp_info.components["catch2base"].names["cmake_find_package_multi"] = "Catch2"
        self.cpp_info.components["catch2base"].names["pkg_config"] = "Catch2"
        self.cpp_info.components["catch2base"].libs = ["Catch2"]
        # Catch2WithMain
        self.cpp_info.components["catch2main"].names["cmake_find_package"] = "Catch2WithMain"
        self.cpp_info.components["catch2main"].names["cmake_find_package_multi"] = "Catch2WithMain"
        self.cpp_info.components["catch2main"].names["pkg_config"] = "Catch2WithMain"
        self.cpp_info.components["catch2main"].libs = ["Catch2Main"]
        self.cpp_info.components["catch2main"].requires = ["catch2base"]
