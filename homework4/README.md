# Qt 应用开发实验三

诊疗工作台使用 Qt Widgets 和 SQLite 实现用户登录及患者档案的查询、新增、修改和删除。患者列表采用 Qt 的 Model/View 结构，性别列使用自定义代理。医生管理、科室管理和操作记录属于指导书的选做内容，本项目只建立了相应的数据库结构。

## 构建和启动

需要 Qt 6.7.3 或更高版本、CMake 和支持 C++17 的编译器。在 Qt Creator 中打开 [CMakeLists.txt](CMakeLists.txt)，选择 Desktop Qt 套件并运行 `QT3Ui`。在 Qt Creator 的运行参数中加入 `--demo-data`，首次启动时导入虚构患者和演示账号 `admin` / `123456`。

也可以在具备 Qt 工具链的终端构建。若 CMake 未找到 Qt，在配置命令中用 `-DCMAKE_PREFIX_PATH` 指定对应套件目录。

```sh
cmake -S . -B build
cmake --build build --config Release
```

| 程序 | 用途 | 数据库路径 |
| --- | --- | --- |
| `QT3Ui` | 启动图形界面 | 默认使用 Qt 应用数据目录下的 `hospital.sqlite`，也可指定 `--database` |
| `QT3` | 从命令行初始化或验证数据库 | 可用 `--database` 明确指定，默认使用当前目录下的 `hospital.sqlite` |

例如，在可执行文件所在目录运行以下命令，两个程序将打开同一个演示库。在其他目录运行时，可将文件名换成绝对路径。

```text
QT3 --database hospital-demo.sqlite --demo-data
QT3Ui --database hospital-demo.sqlite
```

`--demo-data` 可以重复执行，已存在的示例档案和账号会保留。只使用 `--database` 打开新文件时，程序初始化表结构，账号和患者表为空。

## 源码入口

| 文件 | 职责 |
| --- | --- |
| [main.cpp](main.cpp)、[preview/main.cpp](preview/main.cpp) | 解析参数，打开数据库。图形入口还会加载主题、构造患者模型和主窗口。 |
| [database.cpp](database.cpp)、[sql/schema.sql](sql/schema.sql) | 负责 SQLite 连接、结构初始化、查询、数据校验和写入。SQL 脚本由 CMake 打包为 Qt 资源。 |
| [passworddigest.cpp](passworddigest.cpp) | 创建和核对采用独立随机盐的 PBKDF2-SHA256 密码摘要。 |
| [patientrecord.h](patientrecord.h)、[preview/patienttablemodel.cpp](preview/patienttablemodel.cpp) | 定义档案数据及空值表示，查询 Patient 表，给视图提供原始值和显示值。 |
| [preview/patientsexdelegate.cpp](preview/patientsexdelegate.cpp) | 创建性别下拉框，将选中的整数代码交给模型更新。 |
| [preview/navigationcontroller.cpp](preview/navigationcontroller.cpp) | 记录当前页面，限制切换路径，发送 `pageChanged`。 |
| [preview/previewwindow.cpp](preview/previewwindow.cpp)、[preview/theme.qss](preview/theme.qss) | 负责布局、交互、表单状态和操作反馈。 |

## 操作与数据流

### 打开数据库和登录

`Database::open()` 创建命名的 QSQLITE 连接，启用外键，并检查 `PRAGMA user_version`。新库在事务中执行内嵌建表脚本。已有数据库继续使用已保存的表和数据。数据库打不开或版本无法处理时，图形入口会显示错误并退出。

`PreviewWindow::attemptLogin()` 先检查账号和密码是否为空，再让 `Database::passwordDigestForUser()` 通过绑定参数读取账号摘要。SQLite 查询在界面线程完成，摘要核对交给 Qt Concurrent。核对成功后，`NavigationController` 发出页面切换信号，`applyPage()` 更新 `QStackedWidget`、导航状态和键盘焦点。退出登录会清理筛选、选择和表单状态。

### 查询与列表

`PatientTableModel::load()` 使用 `QSqlQuery` 读取 Patient 表，并通过派生自 `QSqlQueryModel` 的模型向视图提供数据。中间的 `QSortFilterProxyModel` 按输入的字面关键词进行不区分大小写的筛选，`QTableView` 显示匹配结果。点击查询或按 Enter 会重新读取 SQLite，再显示当前筛选的结果。列表区分查询无匹配、数据库没有记录和读取失败。

`Qt::EditRole` 提供原始性别代码和 UTC 创建时间，`Qt::DisplayRole` 提供中文性别及本地时间。双击表格打开档案时，窗口把代理索引映射到源模型索引，再从对应的原始行读取记录。复制病历编号时同样使用原始值，可以取得窄窗口中被省略显示的完整 UUID。

### 新增、修改和删除

1. 新增和编辑共用表单。窗口读取字段，解析可空身高和体重，`Database::savePatient()` 检查内容并通过绑定参数执行 INSERT 或 UPDATE。
2. 新增时生成 32 位无连字符的 UUID v4 和 UTC 创建时间，并在同一条 INSERT 中写入。保存成功后才将新编号交还给窗口。列表和编辑页按本地时间展示创建时间。
3. 编辑只更新可编辑字段。UPDATE 的条件同时核对病历编号和打开表单时的原始字段。若记录已被其他连接修改或删除，会提示重新查询。性别代理调用模型的 `setData()`，复用同一写入路径。窗口在代理编辑器关闭后重新读取模型。
4. 删除前显示患者姓名和病历编号，默认按钮为取消。确认后，`Database::removePatient()` 按病历主键删除记录。完成后重新读取 SQLite，继续使用当前筛选并更新记录数。
5. 单击保存后，符合当前筛选的记录会在刷新后的列表中自动选中。编辑过程中返回列表、退出登录或关闭窗口时，若表单有变化，先询问是否放弃。

## 最终验收路径

用单独的演示库运行 `QT3Ui`。先输入错误密码，再正常登录。进入患者管理后搜索一个不存在的关键词并恢复完整列表。新增一条虚构档案，按生成的病历编号查找，修改手机号，检查编号和创建时间，取消一次删除，再确认删除。表格记录数应随数据库中的新增和删除变化。

快捷键使用系统定义。Windows 下在患者列表中可按 Ctrl+F 定位查询框，选中一行后按 Ctrl+C 复制完整病历编号。也可使用行右键菜单。选中性别单元格后按 F2 可打开性别下拉框。

可以用 SQLite 客户端复查以下查询。新记录在插入与编辑之后应能用同一个 ID 查到。确认删除后，以该 ID 查询应返回零行。

```sql
SELECT ID, NAME, MOBILEPHONE, CREATEDTIMESTAMP FROM Patient ORDER BY ID;
PRAGMA integrity_check;
PRAGMA foreign_key_check;
```
