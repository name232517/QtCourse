# 实验二：简单文本编辑器（Qt 应用程序开发）

东莞理工学院  计算机科学与技术学院  Qt 应用程序开发课程  实验二

## 一、当前进度（第一次提交）

本次提交完成**主窗口界面设计**：

- 用 Qt Designer 设计 `mainwindow.ui`：菜单栏、工具栏、编辑区、状态栏布局与组件属性；
- 用 Action 编辑器建立全部 Action，并按「文件 / 编辑 / 格式 / 查看 / 帮助」分组，
  工具栏用分隔符按功能分段；
- 图标统一取自 Material Design Icons 一套图标库，编译进 `res.qrc` 资源；
- 状态栏右侧信息区标签：`length`、`lines`、`Ln`、`Col`、姓名。

尚未实现：文件读写、编辑动作逻辑、查找/替换/关于对话框、行号、未保存提示等（后续提交完成）。

## 二、目录结构

```
QT2/
├── CMakeLists.txt
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── mainwindow.ui      # 界面：菜单、工具栏、编辑区、状态栏
├── res.qrc            # 图标资源
├── images/            # SVG 图标（Material Design Icons）
└── README.md
```

## 三、构建方法

### 命令行（MinGW + CMake）

```powershell
$env:PATH = "E:\Qt\Tools\mingw1310_64\bin;E:\Qt\6.11.2\mingw_64\bin;" + $env:PATH
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=E:/Qt/6.11.2/mingw_64
cmake --build build
```

### Qt Creator

用 Qt Creator 打开 `CMakeLists.txt`，选择 `Desktop Qt 6.11.2 MinGW 64-bit` 构建套件，
然后「构建 → 运行」。

> 工程使用 `find_package(QT NAMES Qt6 Qt5)`，源码同时兼容 Qt 5.12 与 Qt 6。
> 本机实际环境为 Qt 6.11.2 + MinGW 13.1 + CMake，故在本环境编译验证。

## 四、图标来源

图标为 [Material Design Icons](https://pictogrammers.com/library/mdi/)（Pictogrammers，v7.4.47）
中的一套线性图标，SVG 矢量格式，随 `res.qrc` 一起编译进程序。
许可证：Pictogrammers Free License（图标部分基于 Apache License 2.0），可免费用于个人与商业项目。
