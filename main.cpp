#include <QtGui/QApplication>
#include "clfenetreprincipale.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ClFenetrePrincipale w;
    w.show();
    
    return a.exec();
}
