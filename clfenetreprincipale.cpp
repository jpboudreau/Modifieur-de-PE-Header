#include "clfenetreprincipale.h"
#include "ui_clfenetreprincipale.h"

ClFenetrePrincipale::ClFenetrePrincipale(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClFenetrePrincipale), m_modele(nullptr)
{
    ui->setupUi(this);
}

ClFenetrePrincipale::~ClFenetrePrincipale()
{
    m_modele = nullptr;
    delete ui;
}

void ClFenetrePrincipale::on_action_Ouvrir_triggered()
{

    QString fichier = QFileDialog::getOpenFileName(this, "Sélectionner le fichier à ouvrir", QString(), "Exécutable (*.exe *.dll)");

    if (!fichier.isEmpty())
    {
        if (!ClDocument::Instance().OuvrirFichier(fichier))
        {
            QMessageBox::critical(this, "Erreur", "Erreur lors de l'ouverture du fichier");
            ClDocument::Instance().FermerFichier();
        }
        else
        {
            m_modele = ClDocument::Instance().ObtenirModeleArbre();
            ui->treeView->setModel(m_modele);
            ui->treeView->expandAll();
        }
    }
}

void ClFenetrePrincipale::on_treeView_doubleClicked(const QModelIndex &index)
{
    if (m_modele != nullptr)
    {
        //QString texte = m_modele->itemFromIndex(index)->text();
        QSharedPointer<ClType>* type = static_cast<QSharedPointer<ClType>*>(index.data(ClDocument::Type).value<QSharedPointer<ClType>*>());
        if (type == nullptr)
        {
            //QMessageBox::information(this, "Test", "Null");
            return;
        }

        std::ostringstream oss;

        QStandardItem* item = m_modele->itemFromIndex(index);

        //QMessageBox::information(this,"test", QString(oss.str().c_str()));

        if (type->data()->Type() == "unsigned short")
        {
            unsigned short entier = QInputDialog::getInteger(this, "Modification d'un entier", "Veuillez entrer un entier", *(static_cast<short*>(type->data()->Valeur())), 0, 65535);

            oss.str("");
            oss << std::hex << "0x" << entier << " (" << static_cast<char>(entier & 255)
                << static_cast<char>(entier >> 8) << ')';

            item->setText(oss.str().c_str());

            *(static_cast<short*>(type->data()->Valeur())) = entier;
        }
        else if (type->data()->Type() == "unsigned long")
        {
            struct tm tm;
            gmtime_s(&tm, (time_t *)type->data()->Valeur());
            int jour = tm.tm_mday;
            int mois = tm.tm_mon+1;
            int annee = tm.tm_year + 1900;

            struct tm dateEpoch;

            dateEpoch.tm_sec = 0;
            dateEpoch.tm_min = 0;
            dateEpoch.tm_hour = 0;

            oss.str("");

            int entier = QInputDialog::getInteger(this, "Modification d'une date", "Veuillez entrer le jour", jour, 1, 31);
            dateEpoch.tm_mday = entier;
            oss << std::dec << entier;

            entier = QInputDialog::getInteger(this, "Modification d'une date", "Veuillez entrer le mois", mois, 1, 12);
            dateEpoch.tm_mon = entier - 1;
            oss << '/' << entier;

            entier = QInputDialog::getInteger(this, "Modification d'une date", "Veuillez entrer l'année", annee, 1970, 2032);
            dateEpoch.tm_year = entier-1900;
            oss << '/' << entier;

            item->setText(oss.str().c_str());

            *(static_cast<unsigned long*>(type->data()->Valeur())) = _mkgmtime(&dateEpoch);
        }
        else
            QMessageBox::information(this, "Test", type->data()->Type());


    }
}
