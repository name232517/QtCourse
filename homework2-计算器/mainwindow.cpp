#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPushButton>
#include <cmath>

namespace {
// 输入长度上限：超过 16 位有效数字后 double 已无法精确表示，直接忽略多余按键
constexpr int MaxInputDigits = 16;

int digitCount(const QString &text)
{
    int count = 0;
    for (const QChar &c : text) {
        if (c >= '0' && c <= '9')
            ++count;
    }
    return count;
}
}   // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    applyStyleSheet();      // 界面美化：样式表

    // 显示区不接收焦点：键盘输入统一交给 keyPressEvent，避免被 QLineEdit 吃掉
    ui->display->setReadOnly(true);
    ui->display->setFocusPolicy(Qt::NoFocus);

    // 数字 0~9：按控件名统一连接，避免十行重复代码
    for (int i = 0; i <= 9; ++i) {
        connect(findChild<QPushButton *>(QString("btnNum%1").arg(i)), SIGNAL(clicked()),
                this, SLOT(btnNumClicked()));
    }

    // 二元运算符：+ - × /
    const char *binaryOperators[] = {"btnAdd", "btnMinus", "btnMultiply", "btnDivide"};
    for (const char *name : binaryOperators) {
        connect(findChild<QPushButton *>(name), SIGNAL(clicked()),
                this, SLOT(btnBinaryOperatorClicked()));
    }

    // 单目运算符：± √ x^2 1/x %
    const char *unaryOperators[] = {"btnPlus", "btnSqrt", "btnSquare", "btnInverse", "btnPercentage"};
    for (const char *name : unaryOperators) {
        connect(findChild<QPushButton *>(name), SIGNAL(clicked()),
                this, SLOT(btnUnaryOperatorClicked()));
    }

    resetAll();     // 初始显示 0，状态清空
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 界面美化：用样式表按"数字键 / 功能键 / 运算符"三组统一配色、圆角与字号
void MainWindow::applyStyleSheet()
{
    setStyleSheet(R"QSS(
        QMainWindow, QWidget#centralwidget {
            background-color: #1f2226;
            font-family: "Microsoft YaHei UI", "Segoe UI";
        }

        /* 显示区：深色底 + 浅色大字（右对齐在 .ui 中设置） */
        QLineEdit#display {
            background-color: #12141a;
            color: #f5f7fa;
            border: 1px solid #343b47;
            border-radius: 10px;
            padding: 8px 14px;
            font-size: 28px;
        }

        /* 数字键 */
        QPushButton {
            background-color: #333a45;
            color: #f5f7fa;
            border: none;
            border-radius: 10px;
            font-size: 20px;
            padding: 10px 0;
            min-height: 42px;
            min-width: 60px;
        }
        QPushButton:hover   { background-color: #3f4754; }
        QPushButton:pressed { background-color: #2a303a; }

        /* 功能键：CE C ⌫ % 1/x x^2 √ ± */
        QPushButton#btnClearAll, QPushButton#btnClear, QPushButton#btnDel,
        QPushButton#btnPercentage, QPushButton#btnInverse, QPushButton#btnSquare,
        QPushButton#btnSqrt, QPushButton#btnPlus {
            background-color: #4a5361;
        }
        QPushButton#btnClearAll:hover, QPushButton#btnClear:hover, QPushButton#btnDel:hover,
        QPushButton#btnPercentage:hover, QPushButton#btnInverse:hover, QPushButton#btnSquare:hover,
        QPushButton#btnSqrt:hover, QPushButton#btnPlus:hover {
            background-color: #586274;
        }

        /* 运算符：+ - × / = */
        QPushButton#btnAdd, QPushButton#btnMinus, QPushButton#btnMultiply,
        QPushButton#btnDivide, QPushButton#btnEqual {
            background-color: #f59e0b;
            color: #1f2226;
            font-weight: bold;
        }
        QPushButton#btnAdd:hover, QPushButton#btnMinus:hover, QPushButton#btnMultiply:hover,
        QPushButton#btnDivide:hover, QPushButton#btnEqual:hover {
            background-color: #fbb43a;
        }

        /* 状态栏：与窗口同色，仅在有提示信息时显示文字 */
        QStatusBar {
            background-color: #1f2226;
            color: #c9a227;
            border: none;
        }
    )QSS");
}

// 键盘输入：所有按键都转发到与鼠标相同的处理函数，保证两种输入方式行为一致
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    const QString text = event->text();

    // 1. 数字键（主键盘与小键盘都有效）
    if (text.size() == 1 && text.at(0) >= '0' && text.at(0) <= '9') {
        appendDigit(text);
        return;
    }

    // 2. 字符键
    if (text == "." || text == ",") {
        appendDigit(".");
        return;
    }
    if (text == "+") {
        applyBinaryOperator("+");
        return;
    }
    if (text == "-") {
        applyBinaryOperator("-");
        return;
    }
    if (text == "*" || text == "x" || text == "X" || text == "×") {
        applyBinaryOperator("×");
        return;
    }
    if (text == "/" || text == "÷") {
        applyBinaryOperator("/");
        return;
    }
    if (text == "%") {
        applyUnary("%");
        return;
    }
    if (text == "=") {
        on_btnEqual_clicked();
        return;
    }

    // 3. 功能键与单目运算快捷键（与 Windows 计算器保持一致）
    switch (event->key()) {
    case Qt::Key_Return:            // 主键盘回车
    case Qt::Key_Enter:             // 小键盘回车
    case Qt::Key_Equal:
        on_btnEqual_clicked();
        return;
    case Qt::Key_Period:
    case Qt::Key_Comma:
        appendDigit(".");
        return;
    case Qt::Key_Backspace:         // 退格
        on_btnDel_clicked();
        return;
    case Qt::Key_Delete:            // Delete 等于 C：只清当前输入的操作数
        on_btnClear_clicked();
        return;
    case Qt::Key_Escape:            // Esc 等于 CE：清空整个计算
        on_btnClearAll_clicked();
        return;
    case Qt::Key_At:                // @ ：开平方 √
        applyUnary("√");
        return;
    case Qt::Key_Q:                 // q ：平方 x^2
        applyUnary("x^2");
        return;
    case Qt::Key_R:                 // r ：倒数 1/x
        applyUnary("1/x");
        return;
    case Qt::Key_F9:                // F9：正负号 ±
        applyUnary("±");
        return;
    default:
        break;
    }

    QMainWindow::keyPressEvent(event);      // 其它按键交回父类处理
}

// 结算栈顶的一次二元运算：取出最上面的两个操作数和一个运算符
QString MainWindow::calculation(bool *ok)
{
    if (ok)
        *ok = false;
    if (operands.size() < 2 || opcodes.isEmpty())
        return operand;

    const QString opcode = opcodes.back();
    opcodes.pop_back();
    const double operand2 = operands.back().toDouble();
    operands.pop_back();
    const double operand1 = operands.back().toDouble();
    operands.pop_back();

    double result = 0;
    if (opcode == "+") {
        result = operand1 + operand2;
    }
    else if (opcode == "-") {
        result = operand1 - operand2;
    }
    else if (opcode == "×" || opcode == "*") {
        result = operand1 * operand2;
    }
    else if (opcode == "/") {
        if (qFuzzyIsNull(operand2)) {
            showError("除数不能为0");        // 除数为 0：提示后等待下一次输入自动清零
            return QString();
        }
        result = operand1 / operand2;
    }

    if (ok)
        *ok = true;
    return storeNumber(result);             // 中间结果保留完整精度，显示时再格式化
}

// 数字和小数点的统一入口（按钮与键盘都能复用）
void MainWindow::appendDigit(const QString &text)
{
    if (errorState)
        resetAll();                         // 上一次的错误提示：先清零再继续输入
    const bool freshStart = hasResult || !operandEntered;   // 结果后/运算符后/C 之后

    if (text == ".") {
        if (freshStart) {
            operand = "0.";
        }
        else if (!operand.contains(".")) {
            operand += ".";                 // 同一个操作数内只允许一个小数点
        }
        else {
            return;                         // 已经有点：忽略这次输入
        }
    }
    else if (freshStart || operand == "0") {
        operand = text;                     // 开始新操作数，同时去掉前导零
    }
    else {
        if (digitCount(operand) >= MaxInputDigits)
            return;                         // 超过 16 位：忽略（再长 double 也存不下）
        operand += text;
    }

    operandEntered = true;
    hasResult = false;
    typingDigits = true;
    refreshDisplay();
}

void MainWindow::btnNumClicked()
{
    appendDigit(qobject_cast<QPushButton *>(sender())->text());
}

// 单目运算统一入口：± √ x^2 1/x %
void MainWindow::applyUnary(const QString &opcode)
{
    if (errorState)
        resetAll();

    const double value = operand.toDouble();
    double result = value;

    if (opcode == "±") {
        result = -value;
    }
    else if (opcode == "√") {
        if (value < 0) {
            showError("负数不能开平方");
            return;
        }
        result = std::sqrt(value);
    }
    else if (opcode == "x²" || opcode == "x^2") {
        result = value * value;
    }
    else if (opcode == "1/x") {
        if (qFuzzyIsNull(value)) {
            showError("除数不能为0");
            return;
        }
        result = 1.0 / value;
    }
    else if (opcode == "%") {
        // 与常用计算器一致：50 + 10% 得到 55，50 × 10% 得到 5，单独按 10% 得到 0.1
        if (!opcodes.isEmpty() && !operands.isEmpty()
            && (opcodes.back() == "+" || opcodes.back() == "-")) {
            result = operands.back().toDouble() * value / 100.0;
        }
        else {
            result = value / 100.0;
        }
    }

    operand = storeNumber(result);
    operandEntered = true;      // 算出来的值可以直接参加后续运算
    hasResult = true;           // 之后再按数字键则开始输入新的操作数
    typingDigits = false;       // 结果由计算得到，显示时统一格式化
    refreshDisplay();
}

void MainWindow::btnUnaryOperatorClicked()
{
    applyUnary(qobject_cast<QPushButton *>(sender())->text());
}

void MainWindow::btnBinaryOperatorClicked()
{
    applyBinaryOperator(qobject_cast<QPushButton *>(sender())->text());
}

// 二元运算符：向栈中压入操作数，并保证 × ÷ 优先于 + -
void MainWindow::applyBinaryOperator(const QString &opcode)
{
    if (errorState)
        resetAll();

    // 1. 把"等待处理的操作数"入栈（刚输入的数，或者上一个计算结果）
    if (operandEntered || hasResult) {
        operands.push_back(operand);
        operandEntered = false;
        hasResult = false;
    }
    else if (!opcodes.isEmpty()) {
        opcodes.pop_back();             // 连按运算符（如 1 + × 2）：新运算符替换上一个
    }

    // 2. 栈顶优先级不低于新运算符的先结算，这样 1 + 2 × 3 能得到 7
    while (operands.size() >= 2 && !opcodes.isEmpty()
           && priority(opcodes.back()) >= priority(opcode)) {
        bool ok = false;
        const QString result = calculation(&ok);
        if (!ok)
            return;                     // 除零等错误：保持提示信息
        operands.push_back(result);
    }

    // 3. 新运算符入栈，屏幕继续显示第一个操作数（或中间结果）
    opcodes.push_back(opcode);
    if (!operands.isEmpty())
        operand = operands.back();
    typingDigits = false;
    refreshDisplay();
}

void MainWindow::on_btnPeriod_clicked()
{
    appendDigit(".");
}

void MainWindow::on_btnDel_clicked()
{
    if (errorState) {
        resetAll();
        return;
    }

    if (hasResult || !operandEntered) {
        operand = "0";                  // 结果是算出来的，不能退格：归零
        operandEntered = false;
        hasResult = false;
        typingDigits = false;
    }
    else {
        operand.chop(1);
        if (operand.isEmpty() || operand == "-") {
            operand = "0";              // 退到空：下一个数字重新开始
            operandEntered = false;
        }
        typingDigits = operandEntered;  // 还在逐位编辑就保留原样显示
    }
    refreshDisplay();
}

void MainWindow::on_btnClear_clicked()          // C：只清当前输入的操作数
{
    if (errorState)
        resetAll();
    operand = "0";
    operandEntered = false;
    hasResult = false;
    typingDigits = false;
    refreshDisplay();
}

void MainWindow::on_btnClearAll_clicked()       // CE：清空整个计算
{
    resetAll();
}

void MainWindow::on_btnEqual_clicked()
{
    if (errorState)
        resetAll();

    if (operandEntered) {
        operands.push_back(operand);
        operandEntered = false;
    }

    // 末尾悬挂的运算符（如 1 + 2 × =）没有第二个操作数，直接丢掉
    if (opcodes.size() >= operands.size() && !opcodes.isEmpty())
        opcodes.pop_back();

    while (operands.size() >= 2 && !opcodes.isEmpty()) {    // 依次结算所有挂起的运算
        bool ok = false;
        const QString result = calculation(&ok);
        if (!ok)
            return;                     // 除零等错误：保持提示信息
        operands.push_back(result);
    }

    opcodes.clear();
    if (!operands.isEmpty()) {
        operand = operands.back();
        operands.clear();
    }
    hasResult = true;                   // 结果可以接着参与运算，也可以直接开始新计算
    typingDigits = false;
    refreshDisplay();
}

void MainWindow::resetAll()
{
    operand = "0";
    operandEntered = false;
    hasResult = false;
    errorState = false;
    typingDigits = false;
    operands.clear();
    opcodes.clear();
    ui->display->setText(operand);
    statusBar()->clearMessage();
}

void MainWindow::showError(const QString &message)
{
    resetAll();
    errorState = true;
    ui->display->setText(message);
    statusBar()->showMessage(message, 5000);
}

void MainWindow::refreshDisplay()
{
    // 逐位输入时原样显示（保留 "0." 这样的中间状态），计算结果统一格式化
    ui->display->setText(typingDigits ? operand : formatNumber(operand.toDouble()));
}

int MainWindow::priority(const QString &opcode)
{
    if (opcode == "+" || opcode == "-")
        return 1;
    if (opcode == "×" || opcode == "*" || opcode == "/")
        return 2;
    return 0;
}

// 显示用：12 位有效数字，避免 0.1+0.2 显示成一长串
QString MainWindow::formatNumber(double value)
{
    if (std::isinf(value) || std::isnan(value))
        return QStringLiteral("错误");
    QString text = QString::number(value, 'g', 12);
    if (text == "-0")
        text = "0";
    return text;
}

// 运算用：17 位有效数字（double 的往返精度），保证连续运算不被中间结果截断
QString MainWindow::storeNumber(double value)
{
    if (std::isinf(value) || std::isnan(value))
        return QStringLiteral("0");
    return QString::number(value, 'g', 17);
}
