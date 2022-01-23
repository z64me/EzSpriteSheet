#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    Ui::MainWindow *ui;

private slots:
    void setExistingDirectory();
    void on_selectFolderButton_clicked();

    void on_widthValue_valueChanged(int arg1);

    void on_heightValue_valueChanged(int arg1);

    void on_methodValue_currentIndexChanged(int index);

    void on_cbTrim_stateChanged(int arg1);

    void on_cbDoubles_stateChanged(int arg1);

    void on_cbRotate_stateChanged(int arg1);

    void on_cbExhaust_stateChanged(int arg1);

    void on_cbPivot_stateChanged(int arg1);

    void on_cbPad_stateChanged(int arg1);

    void on_valuePad_valueChanged(int arg1);

    void on_cbExhaust_8_stateChanged(int arg1);

    void on_cbExhaust_9_stateChanged(int arg1);

    void on_cbExhaust_10_stateChanged(int arg1);

    void on_cbExhaust_12_stateChanged(int arg1);

    void on_cbExhaust_7_stateChanged(int arg1);

    void on_lineEdit_textChanged(const QString &arg1);

    void on_cbExhaust_6_stateChanged(int arg1);

    int on_buttonRefresh_clicked();

    void on_buttonAbout_clicked();

    void on_buttonExport_clicked();

    void on_cbPrefix_stateChanged(int arg1);

    void on_textPrefix_textChanged(const QString &arg1);

    void on_buttonPivot_clicked();

    void on_pageCombo_currentIndexChanged(int index);

    void on_cbLongname_stateChanged(int arg1);

    void on_textPrefix_textEdited(const QString &arg1);

    void on_textRegex_textEdited(const QString &arg1);

private:
    void SettingsChange(void);
    void PropagatePageCombo(int num);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void setFolderText(QString str);

    struct
    {
        QColor pivot;
    } settings;

    struct
    {
        int sprites;
        int duplicates;
    } stats;
};
#endif // MAINWINDOW_H
