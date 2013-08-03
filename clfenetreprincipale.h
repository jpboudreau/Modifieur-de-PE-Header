#ifndef CLFENETREPRINCIPALE_H
#define CLFENETREPRINCIPALE_H

#include <QMainWindow>
#include <QFileDialog>
#include <QInputDialog>
#include "cldocument.h"

namespace Ui {
class ClFenetrePrincipale;
}

class ClFenetrePrincipale : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit ClFenetrePrincipale(QWidget *parent = 0);
    ~ClFenetrePrincipale();
    
private slots:
    void on_action_Ouvrir_triggered();

    void on_treeView_doubleClicked(const QModelIndex &index);

private:
    Ui::ClFenetrePrincipale *ui;
    QStandardItemModel* m_modele;   // Le modèle qui sera utiliser dans le QTreeView
};

#endif // CLFENETREPRINCIPALE_H
