#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QStack>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // 计算器状态：正在输入的数 operand + 操作数栈 operands + 运算符栈 opcodes
    QString operand;                        // 当前输入/显示的数值串（计算结果按 17 位有效数字保存）
    QStack<QString> operands;               // 操作数栈
    QStack<QString> opcodes;                // 运算符栈
    bool operandEntered = false;            // operand 是否是等待参与运算的操作数
    bool hasResult = false;                 // 刚刚得到计算结果（用于"结果后继续输入"）
    bool errorState = false;                // 处于错误提示状态（如除数为 0）
    bool typingDigits = false;              // 正在逐位输入（原样显示，保留 "0." 这类中间状态）

    QString calculation(bool *ok = nullptr);        // 结算栈顶的一次二元运算

protected:
    void keyPressEvent(QKeyEvent *event) override;  // 键盘输入（与鼠标复用同一套处理函数）

private slots:
    void btnNumClicked();                   // 数字 0~9
    void btnBinaryOperatorClicked();        // + - × /
    void btnUnaryOperatorClicked();         // ± √ x^2 1/x %

    void on_btnPeriod_clicked();            // 小数点
    void on_btnDel_clicked();               // 退格 ⌫
    void on_btnClear_clicked();             // C：只清当前输入的操作数
    void on_btnClearAll_clicked();          // CE：清空整个计算
    void on_btnEqual_clicked();             // =

private:
    void applyStyleSheet();                         // 界面美化：样式表
    void appendDigit(const QString &text);          // 数字/小数点统一入口
    void applyBinaryOperator(const QString &opcode); // 二元运算统一入口
    void applyUnary(const QString &opcode);         // 单目运算统一入口
    void resetAll();                                // 回到初始状态（CE）
    void showError(const QString &message);         // 进入错误状态并提示
    void refreshDisplay();                          // 把当前值显示到屏幕
    static int priority(const QString &opcode);     // 运算符优先级
    static QString formatNumber(double value);      // 用于显示的格式（12 位有效数字）
    static QString storeNumber(double value);       // 用于继续运算的格式（17 位有效数字）

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
