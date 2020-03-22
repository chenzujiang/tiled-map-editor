import qbs 1.0
import qbs.Environment
//tiled.qbs:5中的project
Project {
    name: "Tiled"

    qbsSearchPaths: "qbs"
    minimumQbsVersion: "1.8"

    property string version: Environment.getEnv("TILED_VERSION") || "1.1.5";//返回环境变量的值
    property bool sparkleEnabled: Environment.getEnv("TILED_SPARKLE")       //返回环境变量的值
    property bool snapshot: Environment.getEnv("TILED_SNAPSHOT")            //返回环境变量的值
    property bool release: Environment.getEnv("TILED_RELEASE")              //debug-->return false

    property bool installHeaders: false                                     //将头文件安装进去
    property bool useRPaths: true                                           //如果为false，则阻止链接器将rpath写入二进制文件。
    property bool windowsInstaller: false

    references: [
        "src/libtiled",
        "src/plugins",
        "src/qtpropertybrowser",
        "src/qtsingleapplication",
        "src/tiled",
        "translations"
    ]
}
