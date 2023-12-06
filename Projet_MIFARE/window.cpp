#include "window.h"
#include "ui_window.h"
#include "MfErrNo.h"
#include "Sw_Device.h"
#include "Sw_Mf_Classic.h"
#include "Sw_ISO14443A-3.h"
#include "TypeDefs.h"
#include "Tools.h"
#include <QtGui>

int16_t card_read(uint8_t sect_count);
void updateCount();
std::string nomCarte = "";
std::string prenomCarte = "";
int16_t status = MI_OK;
uint8_t data[240] = {0};
uint8_t bloc_count, bloc, sect;
uint8_t offset;
uint8_t atq[2];
uint8_t sak[1];
uint8_t uid[12];
uint16_t uid_len;

window::window(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::window)
{
    ui->setupUi(this);
}

window::~window()
{
    delete ui;
}

ReaderName monLecteur;

/**
 * @brief Permet de se connecter au lecteur
 */
void window::on_Connect_clicked()
{
    int16_t status = MI_OK;
    monLecteur.Type = ReaderCDC;
    monLecteur.device = 0;

    //Ouvre la communication avec le lecteur
    status = OpenCOM(&monLecteur);
    qDebug() << "OpenCOM" << status;

    status = Version(&monLecteur);
    ui->Affichage->setText(monLecteur.version);
    ui->Affichage->update();
    status = LEDBuzzer(&monLecteur, LED_ON);
    status = LEDBuzzer(&monLecteur, LED_YELLOW_ON);
}

/**
 * @brief Permet de lire la carte MIFARE
 */
void window::on_LireCarte_clicked()
{
    //Prise de contact avec la carte
    ISO14443_3_A_PollCard(&monLecteur, atq, sak, uid, &uid_len);

    uint8_t prenom[16];
    uint8_t nom[16];
    //card_read(uint8_t(16));

    Mf_Classic_Read_Block(&monLecteur, TRUE, 10, prenom, AuthKeyA, 2);
    Mf_Classic_Read_Block(&monLecteur, TRUE, 9, nom, AuthKeyA, 2);

    ui->Nom->setText((char*)nom);
    ui->Prenom->setText((char*)prenom);
    ui->Prenom->update();
    ui->Nom->update();

    updateCount();

    status = LEDBuzzer(&monLecteur, LED_YELLOW_ON);
}

/**
 * @brief Met à jour le compteur de crédit
 */
void window::updateCount()
{
    uint32_t compte = 0;
    Mf_Classic_Read_Value(&monLecteur, TRUE, 14, &compte, AuthKeyA, 3);
    ui->CreditDispo->setText(QString::number(compte));
    ui->CreditDispo->update();
}

/**
 * @brief Déconnecte le lecteur
 */
void window::on_Disconnect_clicked()
{
    int16_t status = MI_OK;
    status =CloseCOM(&monLecteur);
    qDebug() << "OpenCOM" << status;
}

/**
 * @brief Quitte l'application
 */
void window::on_Quitter_clicked()
{
    int16_t status = MI_OK;
    status = CloseCOM(&monLecteur);
    //RF_Power_Control(&monLecteur, FALSE, 0);
    status = LEDBuzzer(&monLecteur, LED_OFF);
    qApp->quit();
}

/**
 * @brief Permet de  rajouter des crédits dans la carte
 */
void window::on_Payer_clicked()
{
    uint32_t tickets = ui->RajoutCredit->value();
    uint32_t compte = (uint32_t)ui->CreditDispo->toPlainText().toInt();
    //Limitation à 5000 tickets max
    if(compte + tickets <= 5000)
    {
        Mf_Classic_Increment_Value(&monLecteur, TRUE, 14, tickets, 14, AuthKeyB, 3);
        status = LEDBuzzer(&monLecteur, LED_RED_ON);
        updateCount();
    }
    else
    {
        status = LEDBuzzer(&monLecteur, LED_GREEN_ON);
    }
    ui->RajoutCredit->setValue(0);

}

/**
 * @brief Permet de retirer des crédits de la carte
 */
void window::on_RetirerCredit_clicked()
{
    uint32_t tickets = ui->spinBox_2->value();
    qDebug() << ui->CreditDispo->toPlainText().toInt();
    uint32_t compte = (uint32_t)ui->CreditDispo->toPlainText().toInt();
    qDebug() << compte;
    qDebug() << tickets;
    qDebug() << compte - tickets;
    //Limitation à 5000 tickets max
    if(compte - tickets <= 5000)
    {
        Mf_Classic_Decrement_Value(&monLecteur, TRUE, 14, tickets, 14, AuthKeyA, 3);
        status = LEDBuzzer(&monLecteur, LED_RED_ON);
        updateCount();
    }
    else
    {
        status = LEDBuzzer(&monLecteur, LED_GREEN_ON);
    }
    ui->spinBox_2->setValue(0);

}

/**
 * @brief Met à jour le nom et prénom sur la carte
 */
void window::on_Maj_clicked()
{
    char newNom[16];
    char newPrenom[16];
    strncpy(newNom, ui->Nom->toPlainText().toUtf8().data(), 16);
    strncpy(newPrenom, ui->Prenom->toPlainText().toUtf8().data(), 16);

    status = Mf_Classic_Write_Block(&monLecteur, TRUE, 9, (uint8_t*)newNom, AuthKeyB, 2);
    status = Mf_Classic_Write_Block(&monLecteur, TRUE, 10, (uint8_t*)newPrenom, AuthKeyB, 2);
}

/**
 * @brief Permet de debug (pas utilisé dans l'appli final)
 * @param sect_count
 * @return
 */
int16_t card_read(uint8_t sect_count)
{
    bloc = 0;
    uid_len = 0;
    sect = 3;

    qDebug() << "Reading sector " << sect;
    //printf("Reading sector %02d : ", sect);

    memset(data, 0x00, 240);

    status = Mf_Classic_Read_Sector(&monLecteur, TRUE, sect, data, AuthKeyA, 3);
    //status = Mf_Classic_Read_Sector(&monLecteur, TRUE, sect, data, AuthKeyB, 2);

    if (status != MI_OK)
    {
        qDebug() << "[Failed]\n" ;
        //qDebug() << GetErrorMessage(status) + " " + status;
        status = ISO14443_3_A_PollCard(&monLecteur, atq, sak, uid, &uid_len);
        if (status != MI_OK)
        {
            qDebug() << "No available tag in RF field\n";
        }
    }
    else
    {
        qDebug() << "[OK]\n";
        // Display sector's data
        if (sect < 32)
            bloc_count = 3;
        else
            bloc_count = 15;
        for (bloc = 0; bloc < bloc_count; bloc++)
        {
            QDebug deb = qDebug();
            deb << bloc;
            std::string s = "";
            //qDebug() << bloc;
            // Each blocks is 16-bytes wide
            for (offset = 0; offset < 16; offset++)
            {
                //qDebug() << data[16 * bloc + offset];
                //deb << data[16 * bloc + offset];
                deb << QString::number(data[16 * bloc + offset], 16);
            }
            for (offset = 0; offset < 16; offset++)
            {
                if (data[16 * bloc + offset] >= ' ')
                {
                    //qDebug() << data[16 * bloc + offset];
                    //deb << data[16 * bloc + offset];
                    //deb << QString::number(data[16 * bloc + offset], 16);
                    //deb << QString::fromUtf8(data[16 * bloc + offset]);
                    deb << char(data[16 * bloc + offset]);
                    s += char(data[16 * bloc + offset]);
                    if(bloc == 1){
                        nomCarte = s;
                    }
                    if(bloc == 2){
                        prenomCarte = s;
                    }


                }
                else
                {
                    //qDebug() << ".";
                    deb << ".";
                }
            }
            //this->ui->Prenom->setText(s);
            //qDebug() <<"\n";
            deb << "\n";
        }
    }
    return status;
}






