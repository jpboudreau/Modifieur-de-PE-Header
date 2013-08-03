#ifndef CLDOCUMENT_H
#define CLDOCUMENT_H

#include <Windows.h>
#include <QString>
#include <String>
#include <QMessageBox>
#include <QList>
#include <QStandardItemModel>
#include <sstream>
#include <ctime>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <QSharedPointer>
#include <iomanip>

#include "ClSection.h"

class ClDocument
{
private:
    ClDocument() :m_fichierEstOuvert(false)
    {};
    ~ClDocument();

public:
    static ClDocument& Instance()
    {
        static ClDocument instance;
        return instance;
    }

    bool OuvrirFichier(const QString&);

    // Sortir de ClDocument?
    QStandardItemModel* ObtenirModeleArbre();
    QStandardItem* ObtenirModeleDosHeader();
    QStandardItem* ObtenirModeleNtHeader();
    QStandardItem* ObtenirModeleFileHeader();
    QStandardItem* ObtenirModeleOptionalHeader();
    QStandardItem* ObtenirModeleSectionHeader();
    QStandardItem* ObtenirModeleDataDirectory();
    QList<QStandardItem*> ObtenirModeleImportTable();
    QStandardItem* ObtenirModeleExportTable();
    QList<QStandardItem*> ObtenirModeleRessource();
    QList<QStandardItem*> ObtenirModeleException();

    void FermerFichier();
    std::vector<ClSection*> Sections() const; // À remplacer par un itérateur...

    // Afin de pouvoir aller chercher les pointeurs des structure dans la QTreeView
    enum Roles
    {
        Type = Qt::UserRole + 1
    };

private:

    bool m_fichierEstOuvert;

    HANDLE m_hFichier;  // Handle du fichier
    HANDLE m_hFichierMapper;    // Handle du fichier mapper en mémoire
    LPVOID m_pBaseFichier;  // Pointeur pointant à la base du fichier
    PIMAGE_DOS_HEADER m_pDosHeader; // Pointeur pointant sur le DOS header
    PIMAGE_NT_HEADERS m_pNtHeader; // Pointeur pointant au début du header NT
    PIMAGE_FILE_HEADER m_pImageFileHeader;
    PIMAGE_OPTIONAL_HEADER m_pImageOptionalHeader;


    std::vector<ClSection*> m_sections;
};

class ClType
{
public:
    ClType(void* p_donnee, const std::type_index& p_type) : m_donnee(p_donnee), m_type(p_type)
    {
    }

    ClType(): m_donnee(nullptr), m_type(typeid(nullptr))
    {
    }

    ~ClType(){}

    QString Type() const
    {
        return m_type.name();
    }

    void* Valeur() const
    {
        return m_donnee;
    }

private:
    void* m_donnee;
    std::type_index m_type;
};

Q_DECLARE_METATYPE(ClType*)
Q_DECLARE_METATYPE(ClType)
Q_DECLARE_METATYPE(QSharedPointer<ClType*>)
Q_DECLARE_METATYPE(QSharedPointer<ClType>*)



#endif // CLDOCUMENT_H
