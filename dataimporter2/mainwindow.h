#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class Manager;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

private slots:
    void on_ibHostLineEdit_textChanged(const QString &arg1);

    void on_ibPortSpinBox_valueChanged(int arg1);

    void on_loginButton_clicked();

    void on_logoutButton_clicked();

    void on_reconnectCheckbox_toggled(bool checked);

    void on_mySqlCheckBox_toggled(bool checked);

    void on_hdf5CheckBox_toggled(bool checked);

    void on_sqlHostNameLineEdit_textChanged(const QString &arg1);

    void on_sqlPortSpinBox_valueChanged(int arg1);

    void on_sqlUserNameLineEdit_textChanged(const QString &arg1);

    void on_sqlPasswordLineEdit_textChanged(const QString &arg1);

    void on_sqlDatabaseNameLineEdit_textChanged(const QString &arg1);

    void on_sqlSymbolTableNameLineEdit_textChanged(const QString &arg1);

    void on_sqlOutputDatabaseLineEdit_textChanged(const QString &arg1);

    void on_loadSymbolTablePushButton_clicked();

//    void on_hdf5OutputFolderLineEdit_textChanged(const QString &arg1);

    void on_hdf5OutputFileToolButton_clicked();

    void on_timeFrameComboBox_currentIndexChanged(int index);

    void on_numberOfMonthsSpinBox_valueChanged(int arg1);

    void on_startButton_clicked();

    void on_stopButton_clicked();

    void on_autoDownloadCheckbox_toggled(bool checked);

    void onConnected();

    void on_sqlSymbolTableNameLineEdit_editingFinished();

    void on_hdf5OutputFolderLineEdit_editingFinished();

    void onDownloading(const QString & name);

private:
    Ui::MainWindow *ui;
    Manager* m_mgr;
    QString m_sqlSymbolTableName;
    bool m_logoutButtonClicked;
    bool m_reconnectOnFailure;
    bool m_autoDownload;


    void readSettings();
    void writeSettings();
    bool userReallyWantsToQuit();

};

#endif // MAINWINDOW_H
