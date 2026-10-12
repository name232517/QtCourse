#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <math.h>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui -> btnNum0, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum1, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum2, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum3, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum4, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum5, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum6, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum7, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum8, SIGNAL(clicked()), this, SLOT(btnNumClicked()));
    connect(ui -> btnNum9, SIGNAL(clicked()), this, SLOT(btnNumClicked()));

    connect(ui->btnMultiply, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
    connect(ui->btnAdd, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
    connect(ui->btnDivide, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));
    connect(ui->btnMinus, SIGNAL(clicked()), this, SLOT(btnBinaryOperatorClicked()));

    connect(ui->btnPercentage, SIGNAL(clicked()), this, SLOT(btnUnaryOperatorClicked()));
    connect(ui->btnInverse, SIGNAL(clicked()), this, SLOT(btnUnaryOperatorClicked()));
    connect(ui->btnSquare, SIGNAL(clicked()), this, SLOT(btnUnaryOperatorClicked()));
    connect(ui->btnSqrt, SIGNAL(clicked()), this, SLOT(btnUnaryOperatorClicked()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::calculation(bool *ok)
{
    if(operands.size() >= 2 && opcodes.size() >= 1){
        double operand1 = operands.front().toDouble();
        operands.pop_front();
        double operand2 = operands.front().toDouble();
        operands.pop_front();
        QString op = opcodes.front();
        opcodes.pop_front();
        double result = 0;
        if(op == "+"){
            result = operand1 + operand2;
        }
        else if(op == "-"){
            result = operand1 - operand2;
        }
        else if(op == "×" || op == "*"){
            result = operand1 * operand2;
        }
        else if(op == "/"){
            result = operand2 != 0 ? operand1 / operand2 : 0;
        }
        operands.push_back(QString::number(result));
        if(ok)
            *ok = true;
        return QString::number(result);
    }
    if(ok)
        *ok = false;
    return operand;
}

void MainWindow::btnNumClicked()
{
    QString digit = qobject_cast<QPushButton *>(sender()) -> text();
    if(digit == "0" && operand == "0")
        operand += digit;
    if(operand == "0" && digit != "0")
        operand = "";
    operand += digit;
    ui -> display -> setText(operand);
}

void MainWindow::btnBinaryOperatorClicked()
{
    QString opcode = qobject_cast<QPushButton *>(sender()) -> text();
    if(operand != ""){
        operands.push_back(operand);
        operand = "";
    }
    if(operands.size() >= 2){
        bool ok = false;
        QString result = calculation(&ok);
        if(ok){
            operands.push_back(result);
            ui ->display->setText(result);
        }
    }
    opcodes.push_back(opcode);
}

void MainWindow::btnUnaryOperatorClicked()
{
    if(operand != ""){
        double result = operand.toDouble();
        operand = "";
        QString op = qobject_cast<QPushButton*>(sender())->text();
        if(op == "%") result /= 100.0;
        else if(op == "1/x") result = 1/result;
        else if(op == "x^2") result *= result;
        else if(op == "√") result = sqrt(result);
        ui->display->setText(QString::number(result));
    }
}


void MainWindow::on_btnPeriod_clicked()
{
    if(!operand.contains("."))
        operand += qobject_cast<QPushButton *>(sender())->text();
    ui -> display -> setText(operand);
}


void MainWindow::on_btnDel_clicked()
{
    operand = operand.left(operand.length() - 1);
    ui -> display -> setText(operand);
}


void MainWindow::on_btnClear_clicked()
{
    operand.clear();
    ui -> display -> setText(operand);
}


void MainWindow::on_btnEqual_clicked()
{
    if(operand != ""){
        operands.push_back(operand);
        operand = "";
    }
    bool ok = false;
    QString result = calculation(&ok);
    if(ok){
        ui ->display->setText(result);
        operands.push_back(result);
    }
}

