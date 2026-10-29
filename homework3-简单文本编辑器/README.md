# 实验二：简单文本编辑器（Qt 应用程序开发）

东莞理工学院  计算机科学与技术学院  Qt 应用程序开发课程  实验二

## 一、当前进度

### 第一次提交：主窗口界面设计

- 用 Qt Designer 设计 `mainwindow.ui`：菜单栏、工具栏、编辑区、状态栏布局与组件属性；
- 用 Action 编辑器建立全部 Action，并按「文件 / 编辑 / 格式 / 查看 / 帮助」分组，
  工具栏用分隔符按功能分段；
- 图标统一取自 Material Design Icons 一套图标库，编译进 `res.qrc` 资源；
- 状态栏右侧信息区标签：`length`、`lines`、`Ln`、`Col`、姓名。

### 第二次提交：对话框界面设计

- 查找对话框 `finddialog.ui`：查找目标输入框、「查找下一个」、方向（向上 / 向下）、
  区分大小写、取消；
- 替换对话框 `replacedialog.ui`：查找目标、替换为、「查找下一个」「替换」「全部替换」、
  方向、区分大小写、取消；
- 关于对话框 `aboutdialog.ui`：软件名称、图标、开发者 / 学号 / 邮箱、版本号、OK；
- 三个对话框由「编辑 → 查找 / 替换」「帮助 → 关于」打开：查找与替换为非模态窗口
  （只创建一次，关掉后再点菜单会重新显示），关于为模态窗口。

三个对话框先搭界面，查找替换的实际算法在后续提交中接入。

### 第三次提交：新建 / 打开 / 保存 / 另存为

- **新建**（Ctrl+N）：清空编辑区，标题回到「新建文本文件」；
- **打开**（Ctrl+O）：文件对话框选择文本文件，按 UTF-8 读入，标题变成「文件名 - 编辑器」；
- **保存**（Ctrl+S）：已有路径直接覆盖写；还没有路径（新建的文档）自动转到另存为；
- **另存为**：选择路径后写入，并把后续保存的目标切换到新文件；
- **退出**（文件菜单）与关闭窗口：文档有未保存修改时弹「保存 / 不保存 / 取消」，
  选「取消」则放弃本次关闭；
- **标题随修改状态变化**：编辑区一改动，标题前出现 `*`；保存或打开后 `*` 消失；
- 读写统一用 UTF-8；打开失败（文件不存在、被占用、无权限）与写入失败都会弹提示说明原因，
  不会清空当前编辑区。

### 第四次提交：撤销 / 恢复 / 剪切 / 复制 / 粘贴 / 全选

- 六个 Action 分别接到编辑区的 `undo / redo / cut / copy / paste / selectAll`，
  菜单项、工具栏按钮、快捷键（Ctrl+Z、Ctrl+Shift+Z、Ctrl+X、Ctrl+C、Ctrl+V、Ctrl+A）共用同一个 Action；
- **使能状态与编辑状态联动**（函数 `updateEditActions()`）：
  - 撤销 / 恢复：跟着文档的 undo/redo 栈是否为空；
  - 剪切 / 复制：只有选中了文字才可用；
  - 粘贴：剪贴板里没有可粘贴的文本时变灰；
  - 全选：始终可用。
- 联动信号来自 `QTextDocument::undoAvailable / redoAvailable`、`QPlainTextEdit::copyAvailable /
  selectionChanged` 与 `QClipboard::dataChanged`，改内容、撤销重做、改变选区、复制别处内容都会刷新。

### 第五次提交：格式菜单（字体、颜色、背景、自动换行、行号）

- **字体**（`on_actionFont_triggered`）：`QFontDialog` 选字体字号，整篇编辑区跟着换，
  行号栏宽度按新字体重算；
- **字体颜色 / 字体背景色**：`QColorDialog` 取色后写进字符格式（`QTextCharFormat` 的
  `foreground` / `background`）。有选中就只改选中部分，没选中就整篇改，之后输入的字符沿用；
- **编辑器背景色**：改编辑区调色板的 `QPalette::Base`，与文字本身的背景色互不影响；
- **自动换行**（`actionWrap`，可勾选）：勾选按窗口宽度换行，取消后长行不折行、出现横向滚动条；
- **显示行号**（`actionLineNumbers`，Ctrl+L，可勾选）：编辑区由 `CodeEditor` 继承 `QPlainTextEdit`
  实现——`LineNumberArea` 画在 viewport 左侧，`blockCountChanged` 时按行数位数重算宽度，
  `updateRequest` 时跟随滚动重绘；在 `mainwindow.ui` 里把 `QPlainTextEdit` 提升（promote）为
  `CodeEditor`；
- 顺手修掉的问题：状态栏标签的行号宽度、提升控件后 `ui_mainwindow.h` 找不到 `codeeditor.h`
  （`CMakeLists.txt` 里加 `target_include_directories`）。

尚未实现：查看菜单（工具栏 / 状态栏的显示隐藏）、状态栏 `length / lines / Ln / Col` 的实时更新、
查找替换的实际算法、定时自动保存（选做）。

## 二、目录结构

```
QT2/
├── CMakeLists.txt
├── main.cpp
├── mainwindow.h
├── mainwindow.cpp
├── mainwindow.ui        # 界面：菜单、工具栏、编辑区、状态栏
├── codeeditor.h
├── codeeditor.cpp       # 派生自 QPlainTextEdit，带行号栏
├── finddialog.h
├── finddialog.cpp
├── finddialog.ui        # 查找对话框
├── replacedialog.h
├── replacedialog.cpp
├── replacedialog.ui     # 替换对话框
├── aboutdialog.h
├── aboutdialog.cpp
├── aboutdialog.ui       # 关于对话框
├── res.qrc              # 图标资源
├── images/              # SVG 图标（Material Design Icons）
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
