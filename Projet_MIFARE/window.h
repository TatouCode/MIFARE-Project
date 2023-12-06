#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class window; }
QT_END_NAMESPACE

class window : public QWidget
{
    Q_OBJECT

public:
    window(QWidget *parent = nullptr);
    ~window();
    int16_t read_data1(uint8_t sect_count);

private slots:
    void on_Connect_clicked();
    void on_Disconnect_clicked();
    void on_Quitter_clicked();
    void on_LireCarte_clicked();
    void on_Maj_clicked();
    void on_Payer_clicked();
    void on_RetirerCredit_clicked();

private:
    Ui::window *ui;
    void updateCount();
};
#endif // WINDOW_H
