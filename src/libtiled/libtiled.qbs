import qbs 1.0

DynamicLibrary {
    targetName: "tiled"

    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: "gui"; versionAtLeast: "5.5" }
    /*The condition to be used for the other bindings in this item.*/
    Properties {
        condition: !qbs.toolchain.contains("msvc")//工具链为msvc
        cpp.dynamicLibraries: base.concat(["z"])
    }

    cpp.cxxLanguageVersion: "c++11"

    cpp.defines: [
        "TILED_LIBRARY",
        "QT_NO_CAST_FROM_ASCII",
        "QT_NO_CAST_TO_ASCII",
        "QT_NO_URL_CAST_FROM_STRING",
        "_USE_MATH_DEFINES"
    ]

    files: [
        "compression.cpp",
        "compression.h",
        "fileformat.cpp",
        "fileformat.h",
        "filesystemwatcher.cpp",
        "filesystemwatcher.h",
        "gidmapper.cpp",
        "gidmapper.h",
        "grouplayer.cpp",
        "grouplayer.h",
        "hex.cpp",
        "hex.h",
        "hexagonalrenderer.cpp",
        "hexagonalrenderer.h",
        "imagecache.cpp",
        "imagecache.h",
        "imagelayer.cpp",
        "imagelayer.h",
        "imagereference.cpp",
        "imagereference.h",
        "isometricrenderer.cpp",
        "isometricrenderer.h",
        "layer.cpp",
        "layer.h",
        "logginginterface.h",
        "map.cpp",
        "map.h",
        "mapformat.cpp",
        "mapformat.h",
        "mapobject.cpp",
        "mapobject.h",
        "mapreader.cpp",
        "mapreader.h",
        "maprenderer.cpp",
        "maprenderer.h",
        "maptovariantconverter.cpp",
        "maptovariantconverter.h",
        "mapwriter.cpp",
        "mapwriter.h",
        "object.cpp",
        "object.h",
        "objectgroup.cpp",
        "objectgroup.h",
        "objecttemplate.cpp",
        "objecttemplate.h",
        "objecttemplateformat.cpp",
        "objecttemplateformat.h",
        "objecttypes.cpp",
        "objecttypes.h",
        "orthogonalrenderer.cpp",
        "orthogonalrenderer.h",
        "plugin.cpp",
        "plugin.h",
        "pluginmanager.cpp",
        "pluginmanager.h",
        "properties.cpp",
        "properties.h",
        "savefile.cpp",
        "savefile.h",
        "staggeredrenderer.cpp",
        "staggeredrenderer.h",
        "templatemanager.cpp",
        "templatemanager.h",
        "tile.cpp",
        "tileanimationdriver.cpp",
        "tileanimationdriver.h",
        "tiled.cpp",
        "tiled_global.h",
        "tiled.h",
        "tile.h",
        "tilelayer.cpp",
        "tilelayer.h",
        "tileset.cpp",
        "tileset.h",
        "tilesetformat.cpp",
        "tilesetformat.h",
        "tilesetmanager.cpp",
        "tilesetmanager.h",
        "varianttomapconverter.cpp",
        "varianttomapconverter.h",
        "wangset.cpp",
        "wangset.h",
        "worldmanager.cpp",
        "worldmanager.h",
    ]

    Group {
        condition: project.installHeaders       //确定组中的文件是否实际被视为项目的一部分(defult true)
        qbs.install: true
        qbs.installDir: "include/tiled"         //前面是install_root目录
        fileTagsFilter: "hpp"                   //指明文件标记名,通过标记名来匹配文件
    }

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt";submodules: ["gui"] }

        cpp.includePaths: "."
    }

    Group {
        condition: true
        qbs.install: true
        qbs.installDir: {                               //默认安装目录为构建目录install_root目录下
            if (qbs.targetOS.contains("windows"))
                return ""
            else if (qbs.targetOS.contains("darwin"))
                return "Tiled.app/Contents/Frameworks"
            else
                return "lib"
        }
        fileTagsFilter: "dynamiclibrary"                //指文件标记名,通过标记名来匹配文件(动态库tiled.dll)
    }
}
