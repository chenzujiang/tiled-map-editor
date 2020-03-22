import qbs 1.0
//Product {
//    type: "staticlibrary"
//}
StaticLibrary {
    name: "qtsingleapplication"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["widgets", "network"]; versionAtLeast: "5.4" }

    cpp.includePaths: ["src"]
    cpp.cxxLanguageVersion: "c++11"

    files: [
        "src/qtlocalpeer.cpp",
        "src/qtlocalpeer.h",
        "src/qtsingleapplication.cpp",
        "src/qtsingleapplication.h",
    ]

    Export {//导出静态库
        Depends { name: "cpp" }
        Depends { name: "Qt.network" }
        cpp.includePaths: "src"
    }
}