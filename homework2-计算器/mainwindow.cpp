#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPushButton>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    return formatNumber(result);
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
        operand += text;
    }

    operandEntered = true;
    hasResult = false;
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
    else if (opcode == "x^2") {
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

    operand = formatNumber(result);
    operandEntered = true;      // 算出来的值可以直接参加后续运算
    hasResult = true;           // 之后再按数字键则开始输入新的操作数
    refreshDisplay();
}

void MainWindow::btnUnaryOperatorClicked()
{
    applyUnary(qobject_cast<QPushButton *>(sender())->text());
}

// 二元运算符：向栈中压入操作数，并保证 × ÷ 优先于 + -
void MainWindow::btnBinaryOperatorClicked()
{
    const QString opcode = qobject_cast<QPushButton *>(sender())->text();
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
    }
    else {
        operand.chop(1);
        if (operand.isEmpty() || operand == "-") {
            operand = "0";              // 退到空：下一个数字重新开始
            operandEntered = false;
        }
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
    refreshDisplay();
}

void MainWindow::resetAll()
{
    operand = "0";
    operandEntered = false;
    hasResult = false;
    errorState = false;
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
    ui->display->setText(operand);
}

int MainWindow::priority(const QString &opcode)
{
    if (opcode == "+" || opcode == "-")
        return 1;
    if (opcode == "×" || opcode == "*" || opcode == "/")
        return 2;
    return 0;
}

QString MainWindow::formatNumber(double value)
{
    if (std::isinf(value) || std::isnan(value))
        return QStringLiteral("错误");
    QString text = QString::number(value, 'g', 12);     // 避免 0.1+0.2 显示成一长串
    if (text == "-0")
        text = "0";
    return text;
}
